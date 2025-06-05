#pragma once
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

#pragma once

#include "defines.h"
#include <vector>
#include <set>
#include <algorithm>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

namespace MultiCore {

	inline int getNumCores()
	{
		static int numCores = -1;
		if (numCores == -1) {
			numCores = _STD thread::hardware_concurrency();
		}
		return numCores;
	}

	template<class L>
	void runLambda(L fLambda, bool multiCore)
	{
		if (multiCore) {
			size_t numCores = getNumCores();
			_STD vector<_STD thread> threads;
			threads.reserve(numCores);
			for (size_t i = 0; i < numCores; i++) {
				threads.push_back(_STD move(_STD thread(fLambda, i, numCores)));
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].join();
			}
		}
		else {
			fLambda(0, 1);
		}
	}

	template<class L>
	void runLambda(size_t numCores, L fLambda, bool multiCore)
	{
		if (multiCore) {
			_STD vector<_STD thread> threads;
			threads.reserve(numCores);
			for (size_t i = 0; i < numCores; i++) {
				threads.push_back(_STD move(_STD thread(fLambda, i, numCores)));
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].join();
			}
		}
		else {
			fLambda(0, 1);
		}
	}

	template<class L>
	void runLambda(L fLambda, _STD vector<size_t>& indexPool, bool multiCore)
	{
		if (multiCore) {
			_STD mutex indexPoolMutex;

			size_t numThreads = getNumCores();
			_STD vector<_STD thread> threads;
			threads.reserve(numThreads);
			for (size_t i = 0; i < numThreads; i++) {
				auto outerLambda = [fLambda, &indexPool, &indexPoolMutex]() {
					size_t index = 0;
					while (index != -1) {
						index = -1;
						{
							_STD lock_guard<_STD mutex> lock(indexPoolMutex); // Tested that mutex overhead is minimal.
							if (!indexPool.empty()) {
								index = indexPool.back();
								indexPool.pop_back();
							}
							else
								break;
						}

						if (index != -1)
							if (!fLambda(index))
								break;
					}
				};

				threads.push_back(move(_STD thread(outerLambda)));
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].join();
			}
		}
		else {
			for (size_t index : indexPool)
				if (index != -1)
					if (!fLambda(index))
						break;
		}
	}

	template<class L>
	void runLambda(L fLambda, size_t numIndices, bool multiCore)
	{
		if (multiCore) {
			size_t numThreads = getNumCores();
			_STD vector<_STD thread> threads;
			threads.reserve(numThreads);
			for (size_t threadNum = 0; threadNum < numThreads; threadNum++) {
				auto outerLambda = [fLambda, numIndices, threadNum, numThreads]() {
					for (size_t index = threadNum; index < numIndices; index += numThreads) {
						if (!fLambda(index))
							break;
					}
				};

				threads.push_back(move(_STD thread(outerLambda)));
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].join();
			}
		} else {
			for (size_t index = 0; index < numIndices; index++)
				if (!fLambda(index))
					break;
		}
	}

class ThreadPool {
private:
	enum Stage {
		AT_NOT_CREATED,
		AT_STOPPED,
		AT_RUNNING,
		AT_TERMINATED,
	};
public:
	using FuncType = _STD function<bool (size_t threadNum, size_t idx)>;

	ThreadPool(size_t numThreads, size_t numSubThreads, size_t numAvailable);

	~ThreadPool();

	inline size_t getNumAllocatedThreads() const;
	inline size_t getNumThreads() const;

	size_t numThreadsAvailable() const;

	template<class L>
	void run(size_t numSteps, const L& f, bool multiCore);
	template<class L>
	void run(size_t numSteps, const L& f, bool multiCore) const;

	template<class L>
	void runSub(size_t numSteps, const L& f, bool multiCore);
	template<class L>
	void runSub(size_t numSteps, const L& f, bool multiCore) const;

	template<class L>
	void run(size_t numThreads, size_t numSteps, const L& f, bool multiCore);

	template<class L>
	void run(size_t numThreads, size_t numSteps, const L& f, bool multiCore) const;

private:
	class Thread {
	public:
		template<class FUNC>
		inline Thread(FUNC f, ThreadPool* pPool)
			: _thread(f, pPool, this)
		{ }

		inline void join() {
			_thread.join();
		}

		std::thread::id _ownerThreadId;
		Stage _stage = AT_NOT_CREATED;
		const FuncType* _pThreadFunc = nullptr;
		mutable size_t _numThreadsForThisFunc = 0;
		mutable size_t _numSteps = 0;
		mutable size_t _startIndex = -1;
	private:
		_STD thread _thread;
	};
	void start();

	void stop();

	bool acquireThreads(size_t numRequested, size_t numSteps, const FuncType& func) const;
	void releaseThread(Thread* pThread) const;

	bool atStage(Stage st) const;

	bool atStage(Stage st0, Stage st1) const;

	void setStageForAll(Stage st) const;

	void setStageFromWorkerThread(Stage st, Thread* pThread) const;

	void runFunc_private(size_t numThreads, size_t numSteps, const FuncType& func) const;

	static void runSingleThreadStat(ThreadPool* pSelf, Thread* pThread);

	void runSingleThread(Thread* pThread);

	bool _running = true;
	const size_t _numThreads, _numSubThreads, _numAllocatedThreads;

	mutable _STD condition_variable _cv;
	mutable _STD mutex _stageMutex, _stackMutex;

	static thread_local _STD vector<Thread*> _ourThreads;
	_STD vector<Thread*> _allocatedThreads;
	mutable _STD vector<Thread*> _availThreads;
};

inline size_t ThreadPool::getNumAllocatedThreads() const
{
	return _numAllocatedThreads;
}

inline size_t ThreadPool::getNumThreads() const
{
	return _numThreads;
}

template<class L>
inline void ThreadPool::run(size_t numSteps, const L& f, bool multiCore) {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(_numThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

template<class L>
inline void ThreadPool::run(size_t numSteps, const L& f, bool multiCore) const {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(_numThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

template<class L>
inline void ThreadPool::runSub(size_t numSteps, const L& f, bool multiCore) {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(_numSubThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

template<class L>
inline void ThreadPool::runSub(size_t numSteps, const L& f, bool multiCore) const {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(_numSubThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

template<class L>
inline void ThreadPool::run(size_t numThreads, size_t numSteps, const L& f, bool multiCore) {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(numThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

template<class L>
inline void ThreadPool::run(size_t numThreads, size_t numSteps, const L& f, bool multiCore) const {
	if (multiCore) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(numThreads, numSteps, wrapper);
	} else {
		for (size_t i = 0; i < numSteps; i++) {
			if (!f(0, i))
				break;
		}
	}
}

} // namespace MultiCore

