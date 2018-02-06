#include "startup.h"

_err_t main(int argc, char *argv[]) {
	_err_t r = init(0);
	if(r == ERR_NONE) {
		//...
	}
	return r;
}

class cTest: public iBase {
public:
	BASE(cTest, "cTest", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		switch(cmd) {
			case OCTL_INIT:
				break;
			case OCTL_UNINIT:
				break;
		}
		return true;
	}
};

static cTest _g_object_;

