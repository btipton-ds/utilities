
#include <iostream>
#include <thread>
#include <mutex>
#include <assert.h>

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
	DONE_STEP,
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
	ThreadRec(ThreadControlRec* pTcr, int threadNum);

	ThreadRec(const ThreadRec& src) = delete;

	ThreadRec& operator = (const ThreadRec& rhs) = delete;

	~ThreadRec()
	{
	}

	inline void threadFunc();
	inline void start(Step stepIn, int runCountIn);
	inline void stop(bool exiting);

	inline void acquireStart();
	inline void releaseStart();
	inline void acquireStop();
	inline void releaseStop();
	inline void acquireRun();
	inline void releaseRun();

	int _threadNum = -1;
	ThreadControlRec* _pTcr = nullptr;
	Step _step = NULL_STEP;
	int _priority = 0;
	int _runCount = -1;
	thread _thread;

	mutex startMutex, stopMutex, runMutex;
};

void gThreadFunc(void* data)
{
	ThreadRec* pThreadRec = (ThreadRec*)data;
	// Wait to finish construction
	while (pThreadRec->_pTcr == nullptr || pThreadRec->_threadNum == -1) {
		this_thread::sleep_for(100ms);
	}
	pThreadRec->threadFunc();
}

ThreadRec::ThreadRec(ThreadControlRec* pTcr, int threadNum)
	: _pTcr(pTcr)
	, startMutex()
	, stopMutex()
	, runMutex()
	, _threadNum(threadNum)
	, _thread(gThreadFunc, this)
{
}

struct ThreadControlRec {
	ThreadControlRec()
		: ownerThreadId(this_thread::get_id())
	{
	}

	void createThreads();
	void startThreads(Step step);
	void runThreads(Step step);
	void waitTillAllDone();
	void stopThreads(Step step);
	inline int calNumThreads(bool exiting) const;
	inline void setThreadCounts(Step step);
	void runThread(int threadNum);

	bool multiThread = false;
	bool threadsCreated = false;
	int maxThreads = INT_MAX;
	int numThreads = 1;
	int runCount = 0;
	size_t maxIdx = -1;

	ThreadType threadType = ThreadType::MAIN;
	thread::id ownerThreadId;

	FunctionWrapperBase* runMethodProc = nullptr;
	FunctionWrapperLoopBase* runMethodLoopProc = nullptr;
	void (*runProc)(void* pData, int threadNum, int numThreads) = nullptr;
	void (*runLoopProc)(void* pData, int idx) = nullptr;
	void* pData = nullptr;


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
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main waiting for start" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread waiting for start" << "[" << _threadNum << "]" << endl;
#endif
	startMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main owns start" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread owns start" << "[" << _threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseStart()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main releasing start" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread releasing start" << "[" << _threadNum << "]" << endl;
#endif
	startMutex.unlock();
}

inline void ThreadRec::acquireStop()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main waiting for stop" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread waiting for stop" << "[" << _threadNum << "]" << endl;
#endif
	stopMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main owns stop" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread owns stop" << "[" << _threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseStop()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main releasing stop" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread releasing stop" << "[" << _threadNum << "]" << endl;
#endif
	stopMutex.unlock();
}

inline void ThreadRec::acquireRun()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main waiting for run" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread waiting for run" << "[" << _threadNum << "]" << endl;
#endif
	runMutex.lock();
#if DUMP_LOCKS
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main owns run" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread owns run" << "[" << _threadNum << "]" << endl;
#endif
}

inline void ThreadRec::releaseRun()
{
#if DUMP_LOCKS
	bool isMain = this_thread::get_id() == _pTcr->ownerThreadId;
	printPad(isMain, _threadNum);
	if (isMain)
		cout << "Main releasing run" << "[" << _threadNum << "]" << endl;
	else
		cout << "Thread releasing run" << "[" << _threadNum << "]" << endl;
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
	_step = NULL_STEP;

	bool done = false;
	while (!done) {
		acquireStart();
		_runCount++;
		if (_step == EXIT_STEP)
			done = true;
		else if (_step == RUN_STEP)
			_pTcr->runThread(_threadNum);

		_step = DONE_STEP;

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
		cout << "runLoopProc: " << threadNum << "\n";
		for (int i = threadNum; i < maxIdx; i += numThreads)
			(*runLoopProc)(pData, i);
	} else if (runMethodProc) {
		runMethodProc->run(threadNum, numThreads);
	} else if (runMethodLoopProc) {
		cout << "runMethodLoopProc: " << threadNum << "\n";
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
		threadRecs[i] = make_shared<ThreadRec>(this, i);
		threadRecs[i]->acquireStart();
		threadRecs[i]->acquireStop();
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
			allRunning = allRunning && threadRecs[i]->_step == NULL_STEP;
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

inline void ThreadControlRec::setThreadCounts(Step step)
{
	numThreads = calNumThreads(step == EXIT_STEP);
	if (step == EXIT_STEP)
		return;
}

inline void ThreadControlRec::startThreads(Step stepIn)
{
	createThreads();
	setThreadCounts(stepIn);

#if 0 && defined(_DEBUG)
	// Verify we're in main thread on processor 1
	verifyCurrentThreadProcessor(-1);
#endif

	// threads owns run
	// main owns start and stop

	for (int i = 0; i < numThreads; i++) {
		threadRecs[i]->_step = stepIn;
		threadRecs[i]->_runCount = runCount;
	}
	// release start
	for (int i = 0; i < numThreads; i++)
		threadRecs[i]->releaseStart();

	runCount++;
}

// Called from the owner thread

inline void ThreadRec::start(Step stepIn, int runCountIn)
{
	_step = stepIn;
	_runCount = runCountIn;
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

inline void ThreadControlRec::stopThreads(Step step) 
{
	for (int i = 0; i < numThreads; i++) {
		ThreadRec& thread = *threadRecs[i];
		thread.stop(step == EXIT_STEP);

		while (thread._step != DONE_STEP) {
			this_thread::sleep_for(10us);
		}
		assert(thread._step == DONE_STEP);
		//		assert(thread.runCount == runCount);
	}
}

void ThreadControlRec::waitTillAllDone()
{
	for (int i = 0; i < numThreads; i++) {
		ThreadRec& thread = *threadRecs[i];

		while (thread._step != DONE_STEP) {
			this_thread::sleep_for(50us);
		}
		cout << "Thread " << i << " finished\n";
	}
}

inline void ThreadControlRec::runThreads(Step step)
{
	startThreads(step);
	waitTillAllDone();
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
	void (*func)(void*, int idx), size_t maxIdx, 
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
		tcr->startThreads(RUN_STEP);
	}
}

void MultiCore::runMethod (FunctionWrapperLoopBase& func, size_t maxIdx,
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
void MultiCore::runMethodNoWait (FunctionWrapperLoopBase& func, size_t maxIdx, 
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
		tcr->startThreads(RUN_STEP); // bogus priority
	}
}

bool MultiCore::running()
{
	ThreadControlRec* tcr = setupTCR();
	for (int i = 0; i < tcr->threadRecs.size(); i++) {
		if (tcr->threadRecs[i]->_step == RUN_STEP)
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
