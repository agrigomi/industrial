#include "private.h"

_u64 mutex::get_handle(void) {
	return  ((((_u64)m_hcount) << 32) | (_u32)(_u64)this);
}

_mutex_handle_t mutex::lock(_mutex_handle_t h) {
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

_mutex_handle_t mutex::try_lock(_mutex_handle_t h) {
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

void mutex::unlock(_mutex_handle_t h) {
	if(h == get_handle()) {
		if(m_lcount)
			m_lcount--;
		if(!m_lcount)
			m_mutex.unlock();
	}
}
