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

#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>

namespace MultiCore {

	inline int getNumCores(int val = -1)
	{
		static int numCores = -1;
		if (numCores == -1) {
			if (val == -1)
				numCores = std::thread::hardware_concurrency();
			else
				numCores = val;
		}
		return numCores;
	}

	template<class L>
	void runLambda(L fLambda, bool multiCore)
	{
		if (multiCore) {
			size_t numCores = getNumCores();
			std::vector<std::thread> threads;
			threads.reserve(numCores);
			for (size_t i = 0; i < numCores; i++) {
				threads.push_back(std::move(std::thread(fLambda, i, numCores)));
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
	void runLambda(L fLambda, std::vector<size_t>& indexPool, bool multiCore)
	{
		if (multiCore) {
			std::mutex indexPoolMutex;

			size_t numThreads = getNumCores();
			std::vector<std::thread> threads;
			threads.reserve(numThreads);
			for (size_t i = 0; i < numThreads; i++) {
				auto outerLambda = [fLambda, &indexPool, &indexPoolMutex]() {
					size_t index = 0;
					while (index != -1) {
						index = -1;
						{
							std::lock_guard<std::mutex> lock(indexPoolMutex); // Tested that mutex overhead is minimal.
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

				threads.push_back(move(std::thread(outerLambda)));
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
			std::vector<std::thread> threads;
			threads.reserve(numThreads);
			for (size_t threadNum = 0; threadNum < numThreads; threadNum++) {
				auto outerLambda = [fLambda, numIndices, threadNum, numThreads]() {
					for (size_t index = threadNum; index < numIndices; index += numThreads) {
						if (!fLambda(index))
							break;
					}
				};

				threads.push_back(move(std::thread(outerLambda)));
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
	public:
		using FuncType = std::function<void(size_t)>;

		ThreadPool(size_t numThreads = -1)
			: _numThreads(numThreads == -1 ? getNumCores() : numThreads)
		{
			_runCount.resize(_numThreads, 0);
		}

		~ThreadPool()
		{
			int dbgBreak = 1;
		}


		void start() {
			_start.lock();

			for (size_t i = 0; i < _numThreads; i++) {
				_threads.push_back(move(std::thread(_lambda, i)));
			}
		}

		void runFunc(const FuncType* f, size_t numSteps) {
			_numSteps = numSteps;
			_pFunc = f;
			_stop.lock();
			_start.unlock();

			std::unique_lock lk(_start);
			_cv.wait(lk, [this]()->bool {
				for (size_t i = 0; i < _numThreads; i++) {
					if (_runCount[i] != _counter + 1)
						return false;
				}
				return true;
			});

			// .wait locks start
			_stop.unlock();
			_counter++;
			_cv.notify_all();

			_pFunc = nullptr;
		}

		FuncType _lambda = FuncType([this](size_t threadNum) {
			while (_running) {
				_start.lock(); // Block until we get the lock
				_start.unlock(); // release it immediately


				if (_pFunc) {
					for (size_t i = threadNum; i < _numSteps; i += _numThreads)
						(*_pFunc)(i);
				}


				_runCount[threadNum] = _counter + 1;
				_cv.notify_all();

				_stop.lock();
				_stop.unlock();
			}
		});

		void stop()
		{
			_running = false;

			for (auto& t : _threads)
				t.join();
			_threads.clear();
		}

	private:
		bool _running = true;
		size_t _numSteps = 0;
		size_t _counter = 0;
		const size_t _numThreads;

		const FuncType* _pFunc = nullptr;

		std::condition_variable _cv;
		std::mutex _start, _stop;
		std::vector<std::thread> _threads;
		std::vector<size_t> _runCount;
	};

} // namespace MultiCore

