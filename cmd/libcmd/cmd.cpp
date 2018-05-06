#include "iCmd.h"
#include "startup.h"

IMPLEMENT_BASE_ARRAY("libcmd", 10)

class cCmd: public iCmd {
private:
public:
	BASE(cCmd, "cCmd", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}
};

