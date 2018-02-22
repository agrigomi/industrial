#include <stdio.h>
#include <unistd.h>
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

		pi_repo->extension_load((_str_t)"bin/core/unix/ext-1/ext-1.so");
		iBase *pi = pi_repo->object_by_iname("iObj1", RF_CLONE);
		usleep(10000);
		pi_repo->extension_unload("ext-1.so");
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
				pi_repo->monitoring_add(0, "iObj1", 0, this);
				break;
			}

			case OCTL_UNINIT:
				break;

			case OCTL_NOTIFY:
				_notification_t *pn = (_notification_t *)arg;
				break;
		}
		return true;
	}
};

static cTest _g_object_;

