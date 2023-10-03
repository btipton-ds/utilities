
#include <iostream>
#include <thread>
#include <mutex>

#if !defined(MAC) && !defined(LINUX)
#include <process.h>
#endif
#include <algorithm>
#include <MultiCoreUtil.h>

#if !defined(MAC) && !defined(LINUX)
#include <tchar.h>
#endif

using namespace std;
using namespace MultiCore;

#define TIME_SETUP 0

namespace {

static mutex mainMutex;

// This flag is set by ServoLoopAPI when the servo loop is stopped/started
// If the servo loop is stopped MultiCore will use processor 0 as well
static bool gServoRunning = false;
//static to turn off processor targeting
static bool gProcessorTargetingEnabled = false;

enum Step {
	START_STEP,
	RUN_STEP,
	EXIT_STEP,
	NULL_STEP,
};

// servo thread's processor is the last in the list.

// It takes 3 mutexes per thread to regulate a thread. Each thread including main must be blocked at all times. 
// That means two mutexes must always be acquired. To advance the thread you must set a new blocking mutex
// without releasing the thread - that's the third mutex. If you don't do this you can't guarantee the thread
// state and you get race conditions.

struct ThreadControlRec;

struct ThreadRec {
	ThreadRec();

	ThreadRec(const ThreadRec& src) = delete;

	ThreadRec& operator = (const ThreadRec& rhs) = delete;

	~ThreadRec()
	{
	}

	inline void threadFunc();
	inline void start(Step stepIn, int runCountIn, int priority);
	inline void stop(bool exiting);

	inline void acquireStart();
	inline void releaseStart();
	inline void acquireStop();
	inline void releaseStop();
	inline void acquireRun();
	inline void releaseRun();

	int threadNum;
	ThreadControlRec* tcr;
	Step step;
	int m_priority;
	int runCount;
	thread _thread;

	mutex startMutex, stopMutex, runMutex;
};

void gThreadFunc(void* data)
{
	ThreadRec* threadRec = (ThreadRec*)data;
	threadRec->threadFunc();
}

ThreadRec::ThreadRec()
	: _thread(gThreadFunc, this)
{
}

struct ThreadControlRec {
	ThreadControlRec()
		: threadsCreated(false)
		, ownerThreadId(this_thread::get_id())
		, numThreads(1)
		, maxIdx(0)
		, runMethodProc(NULL)
		, runMethodLoopProc(NULL)
		, runProc(NULL)
		, runLoopProc(NULL)
		, pData(NULL)
		, runCount(0)
		, threadType(MAIN)
		, maxThreads(INT_MAX)
	{
		ownerThreadId = this_thread::get_id();
	}

	void createThreads();
	void startThreads(Step step, bool skip, int priority);
	void runThreads(Step step);
	void stopThreads(Step step);
	inline int calNumThreads(bool exiting) const;
	inline void setThreadCounts(Step step, bool skip);
	void runThread(int threadNum);

	ThreadType threadType;
	thread::id ownerThreadId;

	int maxIdx;
	FunctionWrapperBase* runMethodProc;
	FunctionWrapperLoopBase* runMethodLoopProc;
	void (*runProc)(void* pData, int threadNum, int numThreads);
	void (*runLoopProc)(void* pData, int idx);
	void* pData;

	bool multiThread;
	bool threadsCreated;
	int maxThreads;
	int numThreads;
	int skipThread;
	int runCount;

	vector<std::shared_ptr<ThreadRec>> threadRecs;

};

typedef vector<ThreadControlRec*> TCRMapType;

static TCRMapType tcrMap;

TCRMapType::iterator findTCRPos()
{
	thread::id threadId = this_thread::get_id();
	TCRMapType::iterator it, pos = tcrMap.end();
	for (it = tcrMap.begin(); it != tcrMap.end(); it++)
		if ((*it)->ownerThreadId == threadId) {
			pos = it;
			break;
		}
	return pos;
}

ThreadControlRec* setupTCR()
{
	SafeLock lock(mainMutex);

	TCRMapType::iterator pos = findTCRPos();
	if (pos == tcrMap.end()) {
		ThreadControlRec* tcr = new ThreadControlRec;
		tcrMap.push_back(tcr);
		return tcr;
	}
	return *pos;
}

// TODO RRT - need accurate processor counts for Mac/Linux
int numLogicalProcessors()
{
	unsigned int numProcs = thread::hardware_concurrency();
	if (numProcs == 0)
		return 1;
	else 
		return numProcs;
}

int calProcNum(int threadNum)
{
	int servoProc = 0;
	int numProcs = numLogicalProcessors();
	int processor = (servoProc + 1 + threadNum) % numProcs;
	return processor;
}

#if 0 && defined(_DEBUG)

void verifyCurrentThreadProcessor(int threadNum)
{
	if(!gProcessorTargetingEnabled)
		return;
	HMODULE Kernel32 = LoadLibrary(_T("Kernel32"));
	if (Kernel32) {
		DWORD (*gcpn)() = (DWORD(*)())GetProcAddress(Kernel32, "GetCurrentProcessorNumber");
		if (gcpn) {
			DWORD procNum = (*gcpn)();
			static int count = 0;
			bool rightProc;
			int processor = calProcNum(threadNum);
			if (threadNum == -1)
				rightProc = procNum == 0 || procNum == 1;
			else
				rightProc = procNum == processor;
			//if (!rightProc) {
			//	std::cout << "MultiCore thread on wrong processor. Num:" << procNum << " " << (count++) << std::endl;
			//}
		}
	}
}

#endif

#define DUMP_LOCKS 0

#if DUMP_LOCKS

void printPad(bool isMain, int threadNum)
{
	if (isMain)
		return;
	for (int i = 0; i <= threadNum; i++)
		cout << "                     ";
}

#endif

inline void ThreadRec::acquireStart()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main waiting for start" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread waiting for start" << "[" << threadNum << "]" << endl;
#endif
	startMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main owns start" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread owns start" << "[" << threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseStart()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main releasing start" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread releasing start" << "[" << threadNum << "]" << endl;
#endif
	startMutex.unlock();
}

inline void ThreadRec::acquireStop()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main waiting for stop" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread waiting for stop" << "[" << threadNum << "]" << endl;
#endif
	stopMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main owns stop" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread owns stop" << "[" << threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseStop()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main releasing stop" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread releasing stop" << "[" << threadNum << "]" << endl;
#endif
	stopMutex.unlock();
}

inline void ThreadRec::acquireRun()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main waiting for run" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread waiting for run" << "[" << threadNum << "]" << endl;
#endif
	runMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main owns run" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread owns run" << "[" << threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseRun()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == tcr->ownerThreadId;
	printPad(isMain, threadNum);
	if (isMain)
		cout << "Main releasing run" << "[" << threadNum << "]" << endl;
	else
		cout << "Thread releasing run" << "[" << threadNum << "]" << endl;
#endif
	runMutex.unlock();
}

#if defined(MAC) || defined(LINUX)
void* gThreadFunc2(void* data)
#else
unsigned __stdcall gThreadFunc2(void* data)
#endif
{
	ThreadRec* threadRec = (ThreadRec*) data;
	threadRec->threadFunc();
#if defined(MAC) || defined(LINUX)
    pthread_exit(NULL);
#else
	_endthreadex( 0 );
#endif
    return 0;
}

inline void ThreadRec::threadFunc()
{
	acquireRun();
	step = NULL_STEP;

	bool done = false;
	while (!done) {
		acquireStart();
		runCount++;
		if (step == EXIT_STEP)
			done = true;
		else if (step == RUN_STEP)
			tcr->runThread(threadNum);

		step = NULL_STEP;

		releaseRun();
		acquireStop();
		releaseStart();
		if (!done)
			acquireRun();
		releaseStop();

	}
}

void ThreadControlRec::runThread(int threadNum)
{
	if (runProc) {
		(*runProc)(pData, threadNum, numThreads);
	} else if (runLoopProc) {
		for (int i = threadNum; i < maxIdx; i += numThreads)
			(*runLoopProc)(pData, i);
	} else if (runMethodProc) {
		runMethodProc->run(threadNum, numThreads);
	} else if (runMethodLoopProc) {
		for (int i = threadNum; i < maxIdx; i += numThreads)
			runMethodLoopProc->run(i);
	}
}

int ThreadControlRec::calNumThreads(bool exiting) const
{
	int numThreads = numLogicalProcessors();
	if (exiting)
		return numThreads;
	if (threadType == MAIN && gServoRunning)
		numThreads--;
	if (numThreads > maxThreads)
		numThreads = maxThreads;
	return numThreads;
}

void ThreadControlRec::createThreads()
{
#if TIME_SETUP
	LARGE_INTEGER startTime, freq, endTime, lastTime;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&startTime);
	lastTime = startTime;
#endif
	
	if (threadsCreated) 
		return;

	threadsCreated = true;

#if TIME_SETUP
	QueryPerformanceCounter(&endTime);
	double time1 = ((double) endTime.QuadPart - (double) lastTime.QuadPart) / (double) freq.QuadPart;
	lastTime = endTime;
#endif

	int numProcs = numLogicalProcessors();
	threadRecs.resize(numProcs);
	int threadStart = 0;
#if !defined(MAC) && !defined(LINUX)
	bool isProcessorTargeting = false;
#endif
    
	for (int i = threadStart; i < numProcs; i++) {
		threadRecs[i] = make_shared<ThreadRec>();
		threadRecs[i]->threadNum = i;
		threadRecs[i]->tcr = this;
		threadRecs[i]->step = START_STEP;
		threadRecs[i]->acquireStart();
		threadRecs[i]->acquireStop();

		int processor = calProcNum(i);
	}

#if TIME_SETUP
	QueryPerformanceCounter(&endTime);
	double time2 = ((double) endTime.QuadPart - (double) lastTime.QuadPart) / (double) freq.QuadPart;
	lastTime = endTime;
#endif

	// Lower our priority so the other threads are higher.
	// If you don't do this the servo thread takes 65 ms to start
	// instead of 1.5 ms.

	bool allRunning = false;
	while (!allRunning) {
		allRunning = true;
		for (int i = threadStart; i < numProcs; i++) {
			allRunning = allRunning && threadRecs[i]->step == NULL_STEP;
			if (!allRunning)
				break;
		}
	}

#if TIME_SETUP
	QueryPerformanceCounter(&endTime);
	double time3 = ((double) endTime.QuadPart - (double) lastTime.QuadPart) / (double) freq.QuadPart;
	lastTime = endTime;
#endif

//	runThreads(START_STEP, true);

#if TIME_SETUP
	QueryPerformanceCounter(&endTime);
	double time4 = ((double) endTime.QuadPart - (double) lastTime.QuadPart) / (double) freq.QuadPart;

	double time = ((double) endTime.QuadPart - (double) startTime.QuadPart) / (double) freq.QuadPart;
	cout << "Time 1:" << time1 << endl;
	cout << "Time 2:" << time2 << endl;
	cout << "Time 3:" << time3 << endl;
	cout << "Time 4:" << time4 << endl;
	cout << "Thread start time:" << time << endl;
#endif
//	run(NULL, test);
}

inline void ThreadControlRec::setThreadCounts(Step step, bool skip)
{
	numThreads = calNumThreads(step == EXIT_STEP);
	skipThread = -1;
	if (step == EXIT_STEP)
		return;
	switch(threadType) {
		case MAIN:
			skipThread = skip ? 0 : -1;
			break;
		default:
			skipThread = -1;
			break;
	}
}

inline void ThreadControlRec::startThreads(Step stepIn, bool skip, int priority)
{
	createThreads();
	setThreadCounts(stepIn, skip);

#if 0 && defined(_DEBUG)
	// Verify we're in main thread on processor 1
	verifyCurrentThreadProcessor(-1);
#endif

	// threads owns run
	// main owns start and stop

	// release start
	for (int i = 0; i < numThreads; i++)
		if (i != skipThread)
			threadRecs[i]->start(stepIn, runCount, priority);
	if (skipThread != -1)
		runThread(skipThread);
	runCount++;
}

// Called from the owner thread

inline void ThreadRec::start(Step stepIn, int runCountIn, int priority)
{
	m_priority = priority;
	step = stepIn;
	runCount = runCountIn;
	releaseStart();
}

inline void ThreadRec::stop(bool exiting)
{
	acquireRun();
	releaseStop();
	if (!exiting)
		acquireStart();
	releaseRun();
	if (!exiting)
		acquireStop();
}

#define STOP_ALL 0
inline void ThreadControlRec::stopThreads(Step step) 
{
#if STOP_ALL
	for (int i = 0; i < numThreads; i++)
		if (i != skipThread)
			threadRecs[i].acquireRun();
	for (int i = 0; i < numThreads; i++)
		if (i != skipThread)
			threadRecs[i].releaseStop();
	if (step != EXIT_STEP)
		for (int i = 0; i < numThreads; i++)
			if (i != skipThread)
				threadRecs[i].acquireStart();
	for (int i = 0; i < numThreads; i++)
		if (i != skipThread)
			threadRecs[i].releaseRun();
	if (step != EXIT_STEP)
		for (int i = 0; i < numThreads; i++)
			if (i != skipThread)
				threadRecs[i].acquireStop();
#else
	for (int i = 0; i < numThreads; i++) {
		if (i != skipThread) {
			ThreadRec& thread = *threadRecs[i];
			thread.stop(step == EXIT_STEP);
//			assert(thread.runCount == runCount);
		}
	}
#endif
}

inline void ThreadControlRec::runThreads(Step step)
{
	startThreads(step, true, 0); // bugus priority
	stopThreads(step);
}

} // namespace 

//allows to turn OFF processor targeting from STIWinApp
//via pref ProcessorTargeting
void MultiCore::setProcessorTargetingEnabled(bool enabled)
{
	gProcessorTargetingEnabled = enabled;
}

// Return the number of threads that will be used by run calls
// The state of the servo loop affects the answer so be sure it 
// doesn't change between this call and a call to a run function.
int MultiCore::numThreads()
{
	ThreadControlRec* tcr = setupTCR();
	return tcr->calNumThreads(false);
}

void MultiCore::setThreadType(ThreadType threadType)
{
	ThreadControlRec* tcr = setupTCR();
	tcr->threadType = threadType;
#if 0 && defined(_DEBUG)
	SafeLock lock(mainMutex);
	TCRMapType::iterator it;
	for (it = tcrMap.begin(); it != tcrMap.end(); it++) {
		tcr = *it;
		cout << "Thread:" << tcr->ownerThreadId << " is ";
		switch (tcr->threadType) {
			case MAIN:
				cout << "Main";
				break;
			case SERVO:
				cout << "Servo";
				break;
			default:
				cout << "Other";
				break;
		}
		cout << endl;
	}
#endif
}


// Tell multi core if the servo loop is running
void MultiCore::setServoRunning(bool isRunning)
{
	gServoRunning = isRunning;
}

void MultiCore::shutdown()
{
	ThreadControlRec* tcr = setupTCR();
    
	tcr->runThreads(EXIT_STEP);
	SafeLock lock(mainMutex);

	TCRMapType::iterator pos = findTCRPos();
	if (pos != tcrMap.end()) {
		delete *pos;
		tcrMap.erase(pos);
	}
}

void MultiCore::setMaxCores(int max)
{
	ThreadControlRec* tcr = setupTCR();
	tcr->maxThreads = max;
}

// ************************************** run functions **************************************

void MultiCore::runFunc (void* pData, 
	void (*func)(void*, int threadNum, int numThreads), 
	bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3)
		(*func)(pData, 0, 1);
	else {

		tcr->pData = pData;
		tcr->runProc = func;

		tcr->runThreads(RUN_STEP);

		tcr->pData = NULL;
		tcr->runProc = NULL;
	}
}

void MultiCore::runFunc (void* pData, 
	void (*func)(void*, int idx), int maxIdx, 
	bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3) {
		for (int i = 0; i < maxIdx; i++)
			(*func)(pData, i);
	} else {
		tcr->pData = pData;
		tcr->maxIdx = maxIdx;
		tcr->runLoopProc = func;

		tcr->runThreads(RUN_STEP);

		tcr->pData = NULL;
		tcr->maxIdx = -1;
		tcr->runLoopProc = NULL;
	}
}

void MultiCore::runMethod (FunctionWrapperBase& func, 
		bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3) {
		func.run(0, 1);
	} else {
		tcr->runMethodProc = &func;
		tcr->runThreads(RUN_STEP);
		tcr->runMethodProc = NULL;
	}
}

void MultiCore::runMethodNoWait (FunctionWrapperBase& func, 
		bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	tcr->multiThread = multiThread;
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3) {
		func.run(0, 1);
	} else {
		tcr->runMethodProc = func.clone();
		tcr->startThreads(RUN_STEP, false, 0); // bogus priority
	}
}

void MultiCore::runMethod (FunctionWrapperLoopBase& func, int maxIdx,
		bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3) {
		for (int i = 0; i < maxIdx; i++)
			func.run(i);
	} else {
		tcr->runMethodLoopProc = &func;
		tcr->maxIdx = maxIdx;

		tcr->runThreads(RUN_STEP);

		tcr->maxIdx = -1;
		tcr->runMethodLoopProc = NULL;
	}
}
void MultiCore::runMethodNoWait (FunctionWrapperLoopBase& func, int maxIdx, 
		bool multiThread)
{
	ThreadControlRec* tcr = setupTCR();
	tcr->multiThread = multiThread;
	int numThreads = tcr->calNumThreads(false);
	if (!multiThread || numThreads < 3) {
		for (int i = 0; i < maxIdx; i++)
			func.run(i);
	} else {
		tcr->runMethodLoopProc = func.clone();
		tcr->maxIdx = maxIdx;
		tcr->startThreads(RUN_STEP, false, 0); // bogus priority
	}
}

bool MultiCore::running()
{
	ThreadControlRec* tcr = setupTCR();
	for (int i = 0; i < tcr->threadRecs.size(); i++) {
		if (tcr->threadRecs[i]->step == RUN_STEP)
			return true;
	}
	return false;
}

void MultiCore::wait()
{
	ThreadControlRec* tcr = setupTCR();
	int numThreads = tcr->calNumThreads(false);
	if (tcr->multiThread && numThreads >= 3) {
		tcr->stopThreads(RUN_STEP);
		tcr->maxIdx = -1;
//		delete_ptr(tcr->runMethodProc);
//		delete_ptr(tcr->runMethodLoopProc);
	}
}
