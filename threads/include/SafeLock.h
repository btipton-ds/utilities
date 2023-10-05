//-----------------------------------------------------------------
//
// Copyright 1998, SensAble Technologies, Inc.
//
// File: SafeLock.h
//
// Author: Josh Handley
//
// Created: 9/15/98
//
//-----------------------------------------------------------------

#ifndef FSSafeLock_DEFINE
#define FSSafeLock_DEFINE

#include <Kernel/Util/Lock.h>

namespace FS {

    // Exception safe wrapper for lock classes.
    // Instead of direclty calling acquire and release
    // on a lock you construct an instance fo this class
    // with a lock object.  The constructor attempts to acquire the lock
    // and the destructor automatically releases the lock.  This way
    // if there is an exception while the lock is acquired it will get
    // released correctly.

    class SafeLock {

    public:

        // Constructor.
        SafeLock(Lock &lock)
            : m_lock(lock),
              m_lockCount(0)
        {
            acquire();
        }

        // Destructor.
        ~SafeLock() 
        {
            if (m_lockCount > 0)
                release();
        }

        // Get lock.
        inline Lock &lock() {return m_lock;}

        // Release lock
        inline void release()
        {
            assert(m_lockCount > 0);
            m_lockCount--;
            lock().release();
        }

        // Acquire the lock.
        inline void acquire()
        {
            lock().acquire();
            m_lockCount++;
            assert(m_lockCount > 0);
        }


    private:

        Lock &m_lock;
        int m_lockCount;
    };

} // end namespace FS

#endif



