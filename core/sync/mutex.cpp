#include <pthread.h>
#include "iSync.h"

#define GET_HANDLE() ((((_u64)m_hcount) << 32) | (_u32)(_u64)this)

class cMutex:public iMutex {
private:
	pthread_mutex_t	m_mutex;
	_u32		m_lcount; // lock count
	_u32		m_hcount; // handle count
public:
	BASE(cMutex, "cMutex", RF_CLONE, 1, 0, 0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				m_lcount = 0;
				m_hcount = 0;
				pthread_mutex_init(&m_mutex, NULL);
				r = true;
				break;
			case OCTL_UNINIT:
				pthread_mutex_destroy(&m_mutex);
				r = true;
				break;
		}
		return r;
	}

	HMUTEX lock(HMUTEX h) {
		HMUTEX r = 0;
		if(h != GET_HANDLE()) {
			pthread_mutex_lock(&m_mutex);
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
			if(pthread_mutex_trylock(&m_mutex) == 0) {
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
			if(!m_lcount)
				pthread_mutex_unlock(&m_mutex);
		}
	}
};

static cMutex _g_object_;
