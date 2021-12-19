#include "iSync.h"

#define USE_SPINLOCK
//#define USE_MUTEX

#ifdef USE_MUTEX
#include <pthread.h>
#endif
#ifdef USE_SPINLOCK
#include "spinlock.h"
#endif

#define GET_HANDLE() ((((_u64)m_hcount) << 32) | (_u32)(_u64)this)

class cMutex:public iMutex {
private:
#ifdef USE_MUTEX
	pthread_mutex_t		m_mutex;
#endif
#ifdef USE_SPINLOCK
	_spinlock_t		m_spinlock;
#endif
	volatile _u32		m_lcount; // lock count
	volatile _u32		m_hcount; // handle count
public:
	BASE(cMutex, "cMutex", RF_CLONE, 1, 0, 0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				m_lcount = 0;
				m_hcount = 0;
#ifdef USE_MUTEX
				pthread_mutex_init(&m_mutex, NULL);
#endif
				r = true;
				break;
			case OCTL_UNINIT:
#ifdef USE_MUTEX
				pthread_mutex_destroy(&m_mutex);
#endif
				r = true;
				break;
		}
		return r;
	}

	HMUTEX lock(HMUTEX h) {
		HMUTEX r = 0;
		if(h != GET_HANDLE()) {
#ifdef USE_MUTEX
			pthread_mutex_lock(&m_mutex);
#endif
#ifdef USE_SPINLOCK
			m_spinlock.lock();
#endif
			m_lcount = 1;
			m_hcount++;
			r = GET_HANDLE();
		} else {
			r = h;
			m_lcount++;
		}
		return r;
	}

	HMUTEX try_lock(HMUTEX h) {
		HMUTEX r = 0;
		if(h != GET_HANDLE()) {
#ifdef USE_MUTEX
			if(pthread_mutex_trylock(&m_mutex) == 0) {
#endif
#ifdef USE_SPINLOCK
			if(m_spinlock.try_lock()) {
#endif
				m_lcount = 1;
				m_hcount++;
				r = GET_HANDLE();
			}
		} else {
			r = h;
			m_lcount++;
		}
		return r;
	}
	void unlock(HMUTEX h) {
		if(h == GET_HANDLE()) {
			if(m_lcount)
				m_lcount--;
			if(!m_lcount) {
#ifdef USE_MUTEX
				pthread_mutex_unlock(&m_mutex);
#endif
#ifdef USE_SPINLOCK
				m_spinlock.unlock();
#endif
			}
		}
	}
};

static cMutex _g_object_;
