#include <stdio.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"

IMPLEMENT_BASE_ARRAY(1024);

void log_listener(_u8 lmt, _str_t msg) {
	printf("%s\n", msg);
}

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		if(pi_log) {
			pi_log->add_listener(log_listener);
			pi_repo->object_release(pi_log);
		}

		pi_repo->extension_load((_str_t)"bin/core/unix/ext-1/ext-1.so", (_str_t)"ext-1");
	}
	return r;
}

class cTest: public iBase {
private:
	iLog *mpi_log;
public:
	BASE(cTest, "cTest", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository*)arg;
				if((mpi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL)))
					mpi_log->write(LMT_INFO, "init cTest");
				break;
			}

			case OCTL_UNINIT:
				break;
		}
		return true;
	}
};

static cTest _g_object_;

