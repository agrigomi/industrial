#include <stdio.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"

void log_listener(_u8 lmt, _str_t msg) {
	printf("%s\n", msg);
}

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		if(pi_log) {
			pi_log->init(16384);
			pi_log->add_listener(log_listener);
			pi_log->write(LMT_INFO, "-- test --");
		}
		//...
	}
	return r;
}

class cTest: public iBase {
private:
	//...
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

