#include "private.h"

bool cDir::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			break;
		case OCTL_UNINIT:
			break;
	}

	return r;
}

void cDir::_init(DIR *p_dir) {
}

void cDir::_close(void) {
}

bool cDir::first(_str_t *fname, _u8 *type) {
	bool r = false;
	//...
	return r;
}

bool cDir::next(_str_t *fname, _u8 *type) {
	bool r = false;
	//...
	return r;
}
