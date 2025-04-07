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

thread_local _STD set<size_t> ThreadPool::_ourThreadIndices;

ThreadPool::ThreadPool(size_t numThreads, size_t numAllocatedThreads)
	: _numThreads(numThreads)
	, _numAllocatedThreads(numAllocatedThreads)
{
	// In primary thread
	_numInUse = 0;
	_assigned.resize(_numAllocatedThreads, false);
	_stage.resize(_numAllocatedThreads, AT_NOT_CREATED);
	_threadFuncs.resize(_numAllocatedThreads, nullptr);
	_numThreadsForThisFunc.resize(_numAllocatedThreads, 0);
	_numSteps.resize(_numAllocatedThreads, 0);
	_startIndex.resize(_numAllocatedThreads, -1);
	start();
}

ThreadPool::~ThreadPool()
{
	// In primary thread
	int dbgBreak = 1;
	stop();
}

void ThreadPool::start() {
	// In primary thread

	for (size_t i = 0; i < _numAllocatedThreads; i++) {
		_allocatedThreads.push_back(move(_STD thread(runStat, this, i)));
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
			return atStage(AT_TERMINATED);
		});
	}

	for (auto& t : _allocatedThreads)
		t.join();
	_allocatedThreads.clear();
}

bool ThreadPool::atStage(Stage st) const
{
	for (size_t i : _ourThreadIndices) {
		if (_stage[i] != st)
			return false;
	}
	_cv.notify_all();
	return true;
}

bool ThreadPool::atStage(Stage st0, Stage st1) const
{
	for (size_t i : _ourThreadIndices) {
		if (_stage[i] != st0 && _stage[i] != st1)
			return false;
	}
	_cv.notify_all();
	return true;
}

void ThreadPool::setStageForAll(Stage st) const
{
	for (size_t i : _ourThreadIndices) {
		_stage[i] = st;
	}
	_cv.notify_all();
}

void ThreadPool::setStage(Stage st, size_t threadNum) const
{
	_stage[threadNum] = st;
	_cv.notify_all();
}

size_t ThreadPool::numThreadsAvailable() const
{
	return _numAllocatedThreads - _numInUse;
}

bool ThreadPool::acquireThreads(size_t numRequested, size_t numSteps, FuncType* f) const
{
	_ourThreadIndices.clear();

	size_t numRequired = numRequested / 8;
	if (numRequired < 1)
		numRequired = 1;
	for (size_t i = 0; i < _numAllocatedThreads; i++) {
		if (!_assigned[i]) {
			_ourThreadIndices.insert(i);
			if (_ourThreadIndices.size() == numRequested)
				break;
		}
	}
	if (_ourThreadIndices.size() >= numRequired) {
		size_t startIdx = 0;
		for (size_t i : _ourThreadIndices) {
			_numInUse++;
			_assigned[i] = true;
			_threadFuncs[i] = f;
			_numThreadsForThisFunc[i] = _ourThreadIndices.size();
			_numSteps[i] = numSteps;
			_startIndex[i] = startIdx++;
		}
		return true;
	} else {
		_ourThreadIndices.clear();
		return false;
	}

}

void ThreadPool::releaseThread(size_t threadNum) const
{
	_numInUse--;
	_assigned[threadNum] = false;

	_threadFuncs[threadNum] = nullptr;
	_numThreadsForThisFunc[threadNum] = 0;
	_numSteps[threadNum] = 0;
	_startIndex[threadNum] = -1;

	_ourThreadIndices.erase(threadNum);

	_cv.notify_all();
}

void ThreadPool::runFunc_private(size_t numThreads, size_t numSteps, FuncType* f) const
{
	// In primary thread
//	_cv.notify_all();
#if 1
	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this, numSteps, f, numThreads]()->bool {
			return acquireThreads(numThreads, numSteps, f);
		});
	}
#endif
	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this]()->bool {
			return atStage(AT_STOPPED);
		});

//		_numSteps = numSteps;
//		_pFunc = f;
		setStageForAll(AT_RUNNING);
	}

	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this]()->bool {
			return atStage(AT_STOPPED);
			});

//		_numSteps = 0;
//		_pFunc = nullptr;

		_cv.notify_all();
	}
#if 1
	{
		_STD unique_lock lk(_stageMutex);
		_cv.wait(lk, [this]()->bool {
			_ourThreadIndices.clear();
			return true;
		});
	}
#endif
}

void ThreadPool::runStat(ThreadPool* pSelf, size_t threadNum) {
	pSelf->run(threadNum);
}

void ThreadPool::run(size_t threadNum) {
	{
		_STD unique_lock lk(_stageMutex);
		setStage(AT_STOPPED, threadNum);
	}
	// In worker thread
	while (_running) {
		{
			_STD unique_lock lk(_stageMutex);
			_cv.wait(lk, [this, threadNum]()->bool {
				return (_stage[threadNum] == AT_RUNNING) || !_running;
			});
		}

		auto pFunc = _threadFuncs[threadNum];
		if (pFunc) {
			size_t numThreads = _numThreadsForThisFunc[threadNum];
			size_t numSteps = _numSteps[threadNum];
			size_t startIdx = _startIndex[threadNum];
			for (size_t i = startIdx; i < numSteps; i += numThreads) {
				if (!(*pFunc)(startIdx, i))
					break;
			}
		}

		{
			_STD unique_lock lk(_stageMutex);
			// This the reason shared threads exist.
			// When this function completes, put this thread back in the pool so it can be reassigned.
			releaseThread(threadNum);
			setStage(AT_STOPPED, threadNum);
		}
	}
	_STD unique_lock lk(_stageMutex);
	setStage(AT_TERMINATED, threadNum);
}
