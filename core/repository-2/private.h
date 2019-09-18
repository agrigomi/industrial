#ifndef __REPOSITORY_PRIVATE_H__
#define __REPOSITORY_PRIVATE_H__

#include <mutex>
#include "dtype.h"

typedef struct 	mutex _mutex_t;
typedef _u64	_mutex_handle_t;

struct mutex {
private:
	std::mutex	m_mutex;
	_u32		m_lcount; // lock count
	_u32		m_hcount; // handle count

	_u64 get_handle(void) {
		return  ((((_u64)m_hcount) << 32) | (_u32)(_u64)this);
	}

public:
	_mutex_handle_t lock(_mutex_handle_t h) {
		_mutex_handle_t r = 0;
		if(h != get_handle()) {
			m_mutex.lock();
			m_lcount = 1;
			m_hcount++;
			r = get_handle();
		} else {
			r = h;
			m_lcount++;
		}
		return r;
	}

	_mutex_handle_t try_lock(_mutex_handle_t h) {
		_mutex_handle_t r = 0;
		if(h != get_handle()) {
			if(m_mutex.try_lock()) {
				m_lcount = 1;
				m_hcount++;
				r = get_handle();
			}
		} else {
			r = h;
			m_lcount++;
		}
		return r;
	}

	void unlock(_mutex_handle_t h) {
		if(h == get_handle()) {
			if(m_lcount)
				m_lcount--;
			if(!m_lcount)
				m_mutex.unlock();
		}
	}
};

#endif

