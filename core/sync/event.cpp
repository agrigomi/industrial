#include <unistd.h>
#include <atomic>
#include "iSync.h"

class cEvent: public iEvent {
private:
	std::atomic<_evt_t> m_state;
public:
	BASE(cEvent, "cEvent", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				m_state = 0;
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	_evt_t check(_evt_t mask) {
		_evt_t state = m_state;
		_evt_t r = state & ~mask;

		m_state.exchange(r);
		if(mask)
			r &= mask;

		return r;
	}

	_evt_t wait(_evt_t mask, _u32 sleep=0) {
		_evt_t r = 0;

		while(!(r = check(mask))) {
			if(sleep)
				usleep(sleep);
		}

		return r;
	}

	void set(_evt_t mask) {
		_evt_t state = m_state;
		_evt_t r = mask | state;
		m_state.exchange(r);
	}
};

static cEvent _g_event_;
