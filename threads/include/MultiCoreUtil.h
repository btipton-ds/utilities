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
#include <functional>

#define STD ::std

namespace MultiCore {

	inline int getNumCores()
	{
		static int numCores = -1;
		if (numCores == -1) {
			numCores = STD::thread::hardware_concurrency();
		}
		return numCores;
	}

	template<class L>
	void runLambda(L fLambda, bool multiCore)
	{
		if (multiCore) {
			size_t numCores = getNumCores();
			STD::vector<STD::thread> threads;
			threads.reserve(numCores);
			for (size_t i = 0; i < numCores; i++) {
				threads.push_back(STD::move(STD::thread(fLambda, i, numCores)));
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
	void runLambda(L fLambda, STD::vector<size_t>& indexPool, bool multiCore)
	{
		if (multiCore) {
			STD::mutex indexPoolMutex;

			size_t numThreads = getNumCores();
			STD::vector<STD::thread> threads;
			threads.reserve(numThreads);
			for (size_t i = 0; i < numThreads; i++) {
				auto outerLambda = [fLambda, &indexPool, &indexPoolMutex]() {
					size_t index = 0;
					while (index != -1) {
						index = -1;
						{
							STD::lock_guard<STD::mutex> lock(indexPoolMutex); // Tested that mutex overhead is minimal.
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

				threads.push_back(move(STD::thread(outerLambda)));
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
			STD::vector<STD::thread> threads;
			threads.reserve(numThreads);
			for (size_t threadNum = 0; threadNum < numThreads; threadNum++) {
				auto outerLambda = [fLambda, numIndices, threadNum, numThreads]() {
					for (size_t index = threadNum; index < numIndices; index += numThreads) {
						if (!fLambda(index))
							break;
					}
				};

				threads.push_back(move(STD::thread(outerLambda)));
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
		AT_DONE,
	};
public:
	using FuncType = STD::function<void(size_t threadNum, size_t idx)>;

	ThreadPool(size_t numThreads = -1)
		: _numThreads(numThreads == -1 ? getNumCores() : numThreads)
	{
		// In primary thread
		_stage.resize(_numThreads, AT_NOT_CREATED);
		start();
	}

	~ThreadPool()
	{
		// In primary thread
		int dbgBreak = 1;
		stop();
	}

	inline size_t getNumThreads() const
	{
		return _numThreads;
	}

	template<class L>
	inline void run(size_t numSteps, const L& f) {
		// In primary thread
		FuncType wrapper(f);
		runFunc_private(numSteps, &wrapper);
	}

private:
	void start() {
		// In primary thread

		for (size_t i = 0; i < _numThreads; i++) {
			_threads.push_back(move(STD::thread(runStat, this, i)));
		}
	}

	void stop()
	{
		// In primary thread
		{
			_cv.notify_all();
			std::unique_lock lk(_stageMutex);
			_cv.wait(lk, [this]()->bool {
				return atStage(AT_STOPPED);
			});

			_running = false;
			setStageForAll(AT_RUNNING);
		}

		for (auto& t : _threads)
			t.join();
		_threads.clear();
	}

	bool atStage(Stage st)
	{
		for (size_t i = 0; i < _numThreads; i++) {
			if (_stage[i] != st)
				return false;
		}
		_cv.notify_all();
		return true;
	}

	bool atStage(Stage st0, Stage st1)
	{
		for (size_t i = 0; i < _numThreads; i++) {
			if (_stage[i] != st0 && _stage[i] != st1)
				return false;
		}
		_cv.notify_all();
		return true;
	}

	void setStageForAll(Stage st)
	{
		for (size_t i = 0; i < _numThreads; i++) {
			_stage[i] = st;
		}
		_cv.notify_all();
	}

	void setStage(Stage st, size_t threadNum)
	{
		_stage[threadNum] = st;
		_cv.notify_all();
	}

	void runFunc_private(size_t numSteps, const FuncType* f) {
		// In primary thread
		_cv.notify_all();
		{
			std::unique_lock lk(_stageMutex);
			_cv.wait(lk, [this]()->bool {
				return atStage(AT_STOPPED);
			});

			_numSteps = numSteps;
			_pFunc = f;
			setStageForAll(AT_RUNNING);
		}

		{
			std::unique_lock lk(_stageMutex);
			_cv.wait(lk, [this]()->bool {
				return atStage(AT_DONE);
			});

			_numSteps = 0;
			_pFunc = nullptr;
			setStageForAll(AT_STOPPED);
		}
	}

	static void runStat(ThreadPool* pSelf, size_t threadNum) {
		pSelf->run(threadNum);
	}

	void run(size_t threadNum) {
		{
			std::unique_lock lk(_stageMutex);
			setStage(AT_STOPPED, threadNum);
		}
		// In worker thread
		while (_running) {
			{
				std::unique_lock lk(_stageMutex);
				_cv.wait(lk, [this]()->bool {
					return atStage(AT_RUNNING, AT_DONE) || !_running;
				});
				if (!_running)
					break;
			}

			if (_pFunc) {
				for (size_t i = threadNum; i < _numSteps; i += _numThreads)
					(*_pFunc)(threadNum, i);
			}

			if (_stage[threadNum] == AT_RUNNING) {
				std::unique_lock lk(_stageMutex);
				setStage(AT_DONE, threadNum);
			}
		}
	}

	bool _running = true;
	size_t _numSteps = 0;
	size_t _counter = 0;
	const size_t _numThreads;

	const FuncType* _pFunc = nullptr;

	std::condition_variable _cv;
	STD::mutex _stageMutex;
	STD::vector<Stage> _stage;

	STD::vector<STD::thread> _threads;
};

} // namespace MultiCore

