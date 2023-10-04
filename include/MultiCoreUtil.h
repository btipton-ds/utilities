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

namespace MultiCore {

	enum ThreadType {
		MAIN,
		OTHER,
	};

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

	class FunctionWrapperBase
	{
	public:
        virtual ~FunctionWrapperBase() {}
		virtual void run(int threadNum, int numThreads) = 0;
		virtual FunctionWrapperBase* clone() const = 0;
	};

	template <class ObjType, class Method>
	class FunctionWrapper : public FunctionWrapperBase
	{
	public:
		FunctionWrapper(ObjType* obj, Method method)
			: m_obj(obj)
			, m_method(method)
		{}

		virtual void run(int threadNum, int numThreads)
		{
			(m_obj->*m_method)(threadNum, numThreads);
		}

		virtual FunctionWrapperBase* clone() const
		{
			return new FunctionWrapper(*this);
		}
	private:
		ObjType* m_obj;
		Method m_method;
	};

	class FunctionWrapperLoopBase
	{
	public:
        virtual ~FunctionWrapperLoopBase(){}
		virtual void run(int idx) = 0;
		virtual FunctionWrapperLoopBase* clone() const = 0;
	};

	template <class ObjType, class Method>
	class FunctionWrapperLoop : public FunctionWrapperLoopBase
	{
	public:
		FunctionWrapperLoop(ObjType* obj, Method method)
			: m_obj(obj)
			, m_method(method)
		{}

		virtual void run(int idx)
		{
			(m_obj->*m_method)(idx);
		}

		virtual FunctionWrapperLoopBase* clone() const
		{
			return new FunctionWrapperLoop(*this);
		}
	private:
		ObjType* m_obj;
		Method m_method;
	};

	//turn OFF processor targeting
	static void setProcessorTargetingEnabled(bool target);

	// Return the number of threads that will be used by run calls
	// The state of the servo loop affects the answer so be sure it 
	// doesn't change between this call and a call to a run function.
	int numThreads();

	void setThreadType(ThreadType threadType);

	// Tell multi core if the servo loop is running
	void setServoRunning(bool isRunning);

	void shutdown();

	void setMaxCores(int max);

	// If useServo is false the processor running the servo loop is not used
	// If useServo is true all processors are used including the one running the servo loop. You should
	// pause the servo loop if you choose this option. The functions won't do it for you.

	// WARNING: These threads are set to highest priority, if you set useServo true it will block
	// the servo loop!!

	// For testing purposes set the multiThread parameter to false and
	// the function will be run in the main thread. This avoids lots of if tests
	// and #defines in the main code.

	// Run a standalone function passing pData, threadNum and numThreads.
	// Expects you to write a loop such as for (i = threadNum; i < max; i += numThreads)
	void runFunc (void* pData, 
		void (*func)(void* pData, int threadNum, int numThreads), 
		bool multiThread = true);

	// Run a standalone function passing pData, and idx.
	// We run the loop "for (idx = threadNum; idx < maxIdx; idx += numThreads)" for you
	void runFunc (void* pData, 
		void (*func)(void* pData, int idx), size_t maxIdx, 
		bool multiThread = true);
	
	// used internally, don't call directly
	void runMethod (FunctionWrapperBase& func, 
		bool multiThread);
	void runMethod (FunctionWrapperLoopBase& func, size_t maxIdx, 
		bool multiThread);
	void runMethodNoWait (FunctionWrapperBase& func, 
		bool multiThread);
	void runMethodNoWait (FunctionWrapperLoopBase& func, size_t maxIdx, 
		bool multiThread);

	// Run an object's member function passing threadNum and numThreads.
	// Expects you to write a loop such as for (i = threadNum; i < max; i += numThreads)
	template <class ObjType, class Method>
	void run (ObjType* obj, Method method, 
		bool multiThread = true)
	{
		FunctionWrapper<ObjType, Method> fw(obj, method);
		runMethod(fw, multiThread);
	}

	// Same as above except does not wait for threads to complete. Call wait to
	// check for thread completion
	template <class ObjType, class Method>
	void runNoWait (ObjType* obj, Method method, 
		bool multiThread = true)
	{
		FunctionWrapper<ObjType, Method> fw(obj, method);
		runMethodNoWait(fw, multiThread);
	}

	// Run an object's member function passing threadNum and numThreads.
	// We run the loop "for (idx = threadNum; idx < maxIdx; idx += numThreads)" for you
	template <class ObjType, class Method>
	void run (ObjType* obj, Method method, size_t maxIdx, 
		bool multiThread = true)
	{
		FunctionWrapperLoop<ObjType, Method> fw(obj, method);
		runMethod(fw, maxIdx, multiThread);
	}

	// Same as above except does not wait for threads to complete. Call wait to
	// check for thread completion
	template <class ObjType, class Method>
	void runNoWait (ObjType* obj, Method method, size_t maxIdx, 
		bool multiThread = true)
	{
		FunctionWrapperLoop<ObjType, Method> fw(obj, method);
		runMethodNoWait(fw, maxIdx, multiThread);
	}

	// Wait for thread completion after calling a runNoWait.
	bool running();
	void wait();

	class MaxCores
	{
	public:
		MaxCores(int max)
		{
			setMaxCores(max);
		}
		~MaxCores()
		{
			setMaxCores(INT_MAX);
		}
	};

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
			int numT = numThreads();
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

