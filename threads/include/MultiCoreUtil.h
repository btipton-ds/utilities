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

	class SafeLock{
	public:
		inline SafeLock(std::mutex& mtx)
			: _mtx(mtx)
		{
			_mtx.lock();
		}

		inline ~SafeLock()
		{
			_mtx.unlock();
		}
	private:
		std::mutex& _mtx;
	};

	

	inline int getNumCores()
	{
		return std::thread::hardware_concurrency();
	}

	template<class T, typename M>
	class FuncWrapper {
	public:
		FuncWrapper()
		{
		}

		void start()
		{
			_pThread = std::make_shared<std::thread>(&func, this);
		}

		static void func(void* p) {
			FuncWrapper* pFW = (FuncWrapper*)p;
			(pFW->_obj->*pFW->_method)(pFW->_threadNum, pFW->_numThreads);
		}

		T* _obj;
		M _method;
		size_t _threadNum;
		size_t _numThreads;
		std::shared_ptr<std::thread> _pThread = nullptr;
	};

	template<class T, class M>
	void runMethod(T* obj, M method, bool multiCore)
	{
		if (multiCore) {
			std::vector<FuncWrapper<T, M>> threads;
			threads.resize(getNumCores());
			for (size_t i = 0; i < threads.size(); i++) {
				threads[i]._obj = obj;
				threads[i]._method = method;
				threads[i]._threadNum = i;
				threads[i]._numThreads = threads.size();
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].start();
			}

			for (size_t i = 0; i < threads.size(); i++) {
				threads[i]._pThread->join();
			}

		}
		else {
			(obj->*method)(0, 1);
		}
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

	class ThreadPool
	{
	public:
		void init();
	};

	template<class L>
	void runLambda(ThreadPool& pool, L fLambda, const std::vector<size_t>& indexPoolIn, bool multiCore)
	{
		pool.init();
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

	// ********************************************
	// MultiCore std::vector sorting classes
	// ********************************************

	template <class T>
	class DefaultComp
	{
	public:
		bool operator()(const T& a, const T& b) const
		{
			return a < b;
		}
	};

	template <class T, class Comp>
	struct SortRec
	{
		struct Range
		{
			Range(int startIn = 0, int stopIn = 0)
				: start(startIn)
				, stop(stopIn)
			{}

			int start, stop;
		};

		// All the work is done in the constructor which modifies list
		SortRec(std::vector<T>& list, const Comp& comp)
			: m_list(list)
			, m_comp(comp)
		{
			int size = (int)m_list.size();
			int numT = getNumCores();
			bool runMulti = size > (50 * numT);
			if (!runMulti)
				numT = 1;
			run(this, &SortRec::sortRangeMC, runMulti);

			if (runMulti) {
				m_ranges.resize(numT);
				for (int i = 0; i < numT; i++) {
					m_ranges[i].start = (i * size) / numT;
					m_ranges[i].stop = ((i+1) * size) / numT;
				}
				while (m_ranges.size() > 1) {
					run(this, &SortRec::mergeZonesMC);
					if (m_ranges.size() % 2 == 1)
						numT = m_ranges.size() / 2 + 1;
					else
						numT = m_ranges.size() / 2;
					for (int i = 0; i < numT; i++) {
						if (i == 0)
							m_ranges[i].start = 0;
						else
							m_ranges[i].start = m_ranges[i-1].stop;
						int idx = 2 * i + 1;
						if (idx >= m_ranges.size())
							idx = m_ranges.size() - 1;
						m_ranges[i].stop = m_ranges[idx].stop;
					}
					m_ranges.resize(numT);
				}
			}
		}

		// Sorts each range of m_list on its own CPU
		void sortRangeMC(int threadNum, int numThreads)
		{
			int size = (int)m_list.size();
			int start = (threadNum * size) / numThreads;
			int stop = ((threadNum+1) * size) / numThreads;
			std::sort(m_list.begin() + start, m_list.begin() + stop, m_comp);
		}

		// Merges pairs of ranges of m_list on its own CPU
		// If there is no range for this CPU it simply returns
		void mergeZonesMC(int threadNum, int numThreads)
		{
			int steps = (int)(m_ranges.size() / 2);
			for (int i = threadNum; i < steps; i += numThreads) {
				if (i % numThreads == threadNum) {
					std::inplace_merge(m_list.begin() + m_ranges[2*i].start, 
									   m_list.begin() + m_ranges[2*i+1].start, 
									   m_list.begin() + m_ranges[2*i+1].stop, m_comp);
				}
			}
		}

	private:
		std::vector<Range> m_ranges; // Range of each sub sorting region
		std::vector<T>& m_list;		// List to be sorted
		const Comp& m_comp;		// Comparison functor
	};

	// sort sorts an std::vector<T> from begin to end.
	// Measured time on a 6 core system in a release build was about 4X - 5X 
	// faster than std::sort.
	// If there are fewer than 50 X numCPUs entries in list it runs single threaded
	// It divides the vector into ranges, sorts each range on its own core
	// and then merges the results together. 
	// All the work is done with std::sort and std::inplace_merge so if it
	// works with sort it should work with this.
	template <class T>
	void sort(std::vector<T>& list)
	{
		SortRec<T, DefaultComp<T>> rec(list, DefaultComp<T>());
	}

	// sort sorts an std::vector<T> from begin to end.
	// Measured time on a 6 core system in a release build was about 4X - 5X 
	// faster than std::sort.
	// If there are fewer than 50 X numCPUs entries in list it runs single threaded
	// It divides the vector into ranges, sorts each range on its own core
	// and then merges the results together. 
	// All the work is done with std::sort and std::inplace_merge so if it
	// works with sort it should work with this.
	// This version takes a comparison functor.
	template <class T, class Comp>
	void sort(std::vector<T>& list, const Comp& comp = DefaultComp<T>())
	{
		SortRec<T, Comp> rec(list, comp);
	}

} // namespace MultiCore

