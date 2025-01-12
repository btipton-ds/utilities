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

#define _STD ::std::


