//-----------------------------------------------------------------
//
// Copyright 2023 Dark Sky Innovative Solutions.
//
// File: MultiCoreUtil.h
//
// Author: Bob Tipton
//
// Created: 10/3/2023
//
// Use this instead of COneShotThreadManager were possible
//
// API to easily support multi threading on multiple cores. API is servo loop aware and uses a thread pool for
// each calling thread. Assuming that only the main and servo loops invoke the API this means there will be two
// thread pools. This is unavoidable since we are using mutexes to regulate the pool and the thread which locks a
// mutex is the only thread that can unlock it.
//
// Uses CriticalSection switching and no sleep calls, should be very speedy.
// Switching overhead measured as ~30 micro seconds on a 2.5GHZ quad core system. This compares with 1-3 
// milli seconds thread launch time.
//
// Note that the first launch will incur the 1-3 milli second delay because it has to start the threads.
// The interactive smoother shows that thread overhead is ~13%-15% of a 350 micro second run time which agrees well 
// with the 30 micro second number. Tests on the interactive smoother showed a best case improvement of 3.47 X
// on a quad core system compared with a single core.
//
// Further testing on a 6 core/2.8 GHz system resulted in an overhead of 9-20 micro 
// seconds and an improvement 5.75 X on interactive smooth. This is extremely good
// since the total time per call was only 64.4 micro seconds.

// Function calls are assigned to each thread except for the thread running on the caller's processor. That call 
// is made directly after the other threads are launched to save one thread switch.
//
// When called from the servo thread all cores are used. This based on the assumption that the servo thread would have
// been executing the calls anyway so it can share the load.
//
// When called from the main thread with the servo loop running, the the servo thread processor (number 0) is not used.
// If the servo loop is not running all cores are used. MultiCore and the ServoLoopAPI manages this for you.
//
// You should probably never call this from a thread other than Main/Servo, if you do be sure to call MultiCore::shutdown
// or you'll leave a dangling thread pool.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <assert.h>

#if defined(_WIN32)
#include <process.h>
#endif

#include <algorithm>
#include <MultiCoreUtil.h>

#if defined(_WIN32)
#include <tchar.h>
#endif

using namespace std;
using namespace MultiCore;

class ThreadPool::Thread {
public:
	template<class FUNC>
	inline Thread(FUNC f, ThreadPool* pPool)
		: _thread(f, pPool, this)
	{
	}

	inline void join() {
		_thread.join();
	}

	std::thread::id _ownerThreadId;
	Stage _stage = AT_NOT_CREATED;
	const FuncType* _pThreadFunc = nullptr;
	mutable size_t _numThreadsForThisFunc = 0;
	mutable size_t _numSteps = 0;
	mutable size_t _ourThreadIndex = -1;
private:
	_STD thread _thread;
};



ThreadPool::ThreadPool(size_t numThreads, size_t numSubThreads, size_t numAllocatedThreads)
	: _numThreads(numThreads)
	, _numSubThreads(numSubThreads)
{
	// In primary thread
	start(numAllocatedThreads);
}

ThreadPool::~ThreadPool()
{
	// In primary thread
	stop();
}

void ThreadPool::start(size_t numAllocatedThreads) {
	// In primary thread

	for (size_t i = 0; i < numAllocatedThreads; i++) {
		_allocatedThreads.push_back(new Thread(runSingleThreadStat, this));
		_availThreads.push_back(_allocatedThreads.back());
	}
}

void ThreadPool::stop()
{
	// In primary thread
	{
		_running = false;
		_cv.notify_all();
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this]()->bool {
			return atStage(_allocatedThreads, AT_TERMINATED);
		});
	}

	for (auto& t : _allocatedThreads) {
		t->join();
		delete t;
	}
	_allocatedThreads.clear();
	_availThreads.clear();
}

bool ThreadPool::atStage(const _STD vector<Thread*>& ourThreads, Stage st) const
{
	for (auto pThread : ourThreads) {
		if (pThread->_ownerThreadId == this_thread::get_id() && pThread->_stage != st)
			return false;
	}
	_cv.notify_all();
	return true;
}

bool ThreadPool::atStage(const _STD vector<Thread*>& ourThreads, Stage st0, Stage st1) const
{
	for (auto pThread : ourThreads) {
		if (pThread->_ownerThreadId == this_thread::get_id() && pThread->_stage != st0 && pThread->_stage != st1)
			return false;
	}
	_cv.notify_all();
	return true;
}

void ThreadPool::setStageForAll(const _STD vector<Thread*>& ourThreads, Stage st) const
{
	for (auto pThread : ourThreads) {
		assert(pThread->_ownerThreadId == this_thread::get_id());
		pThread->_stage = st;
	}
	_cv.notify_all();
}

void ThreadPool::setStageFromWorkerThread(Stage st, Thread* pThread) const
{
	pThread->_stage = st;
	_cv.notify_all();
}

void ThreadPool::acquireThreads(size_t numRequested, size_t numSteps, size_t minStepsToMultiThread, const FuncType& func, _STD vector<Thread*>& ourThreads) const
{
	ourThreads.clear();

	size_t num = 0;
	{
		lock_guard lg(_stackMutex);
		if (minStepsToMultiThread != -1) {
			size_t numStepsPerThread = minStepsToMultiThread / 3;
			size_t numRecommended = std::max((size_t)1, numSteps / numStepsPerThread);
			num = std::min(numRequested, numRecommended);
			num = std::min(num, _availThreads.size());
		} else {
			num = std::min(numRequested, _availThreads.size());
		}
		if (num < 1)
			num = 1;
		for (size_t i = 0; i < num - 1; i++) {
			auto pThread = _availThreads.back();
			pThread->_ownerThreadId = this_thread::get_id();
			_availThreads.pop_back();
			ourThreads.push_back(pThread);
		}
	}

	for (size_t i = 0; i < ourThreads.size(); i++) {
		auto pThread = ourThreads[i];

		pThread->_pThreadFunc = &func;
		pThread->_numThreadsForThisFunc = ourThreads.size() + 1;
		pThread->_numSteps = numSteps;
		pThread->_ourThreadIndex = i;
	}

}

void ThreadPool::releaseThread(Thread* pThread) const
{
	pThread->_ownerThreadId = {};
	pThread->_pThreadFunc = nullptr;
	pThread->_numThreadsForThisFunc = 0;
	pThread->_numSteps = 0;
	pThread->_ourThreadIndex = -1;

	{
		lock_guard lg(_stackMutex);
		_availThreads.push_back(pThread);
	}

	_cv.notify_all();
}

void ThreadPool::runFunc_private(size_t numThreads, size_t numSteps, size_t minStepsToMultiThread, const FuncType& func, _STD vector<Thread*>& ourThreads) const
{
	// In owner thread
	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this, numThreads, numSteps, minStepsToMultiThread, &func, &ourThreads]()->bool {
			acquireThreads(numThreads, numSteps, minStepsToMultiThread, func, ourThreads);
			return true;
		});
	}

	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this, &ourThreads]()->bool {
			return atStage(ourThreads, AT_STOPPED);
		});

		setStageForAll(ourThreads, AT_RUNNING);
	}

	size_t startIndex = 0;
	size_t stride = ourThreads.size() + 1;
	for (size_t i = startIndex; i < numSteps; i += stride) {
		if (!func(startIndex, i))
			break;
	}

	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this, &ourThreads]()->bool {
			return atStage(ourThreads, AT_STOPPED);
		});

		_cv.notify_all();
	}

	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this]()->bool {
			return true;
		});
	}
}

void ThreadPool::runSingleThreadStat(ThreadPool* pSelf, Thread* pThread) {
	pSelf->runSingleThread(pThread);
}

void ThreadPool::runSingleThread(Thread* pThread) {
	{
		_STD unique_lock lk(_stageMutex);
		setStageFromWorkerThread(AT_STOPPED, pThread);
	}
	// In worker thread
	while (_running) {
		{
			_STD unique_lock lk(_stageMutex);
			_cv.wait(lk, [this, pThread]()->bool {
				return (pThread->_stage == AT_RUNNING) || !_running;
			});
		}

		auto pFunc = pThread->_pThreadFunc;
		if (pFunc) {
			size_t startIndex = pThread->_ourThreadIndex + 1;
			size_t numSteps = pThread->_numSteps;
			size_t stride = pThread->_numThreadsForThisFunc;
			for (size_t i = startIndex; i < numSteps; i += stride) {
				if (!(*pFunc)(startIndex, i))
					break;
			}
		}

		{
			_STD unique_lock lk(_stageMutex);
			// This the reason shared threads exist.
			// When this function completes, put this thread back in the pool so it can be reassigned.
			releaseThread(pThread);
			setStageFromWorkerThread(AT_STOPPED, pThread);
		}
	}
	_STD unique_lock lk(_stageMutex);
	setStageFromWorkerThread(AT_TERMINATED, pThread);
}
