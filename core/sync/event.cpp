#include <atomic>
#include "iSync.h"

class cEvent: public iEvent {
private:
	std::atomic<_evt_t> state;
public:
	BASE(cEvent, "cEvent", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				break;
			case OCTL_UNINIT:
				break;
		}
	}

	_evt_t wait(_evt_t mask) {
	}

	_evt_t check(_evt_t mask) {
	}

	void set(_evt_t mask) {
	}
};

static cEvent _g_event_;
