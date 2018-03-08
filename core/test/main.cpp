#include <stdio.h>
#include <unistd.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iFS.h"

IMPLEMENT_BASE_ARRAY(1024);

void log_listener(_u8 lmt, _str_t msg) {
	printf("%s\n", msg);
}

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		if(pi_log)
			pi_log->add_listener(log_listener);
		pi_repo->extension_load((_str_t)"bin/core/unix/ext-1/ext-1.so");
		pi_repo->extension_load((_str_t)"bin/io/unix/libfs/libfs.so");
		iBase *obj = pi_repo->object_by_iname("iObj1", RF_CLONE|RF_ORIGINAL);
		iFS *pi_fs = (iFS *)pi_repo->object_by_iname(I_FS, RF_ORIGINAL);
		iFileIO *pifio = pi_fs->open("./testfile", O_RDWR|O_CREAT);
		pifio->write((_str_t)"alabala", 7);
		pifio->sync();
		_str_t pfc = (_str_t)pifio->map();
		pi_fs->close(pifio);
		getchar();
		pi_repo->object_release(pi_fs);
		if((r = pi_repo->extension_unload("libfs.so")))
			pi_log->fwrite(LMT_ERROR, "unable to unload libfs.so error %d", r);
		pi_repo->object_release(obj);
		if((r = pi_repo->extension_unload("ext-1.so")))
			pi_log->fwrite(LMT_ERROR, "unable to unload ext-1.so error %d", r);
		uninit();
	}

	return r;
}

class cTest: public iBase {
private:
	iLog *mpi_log;
public:
	BASE(cTest, "cTest", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
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

			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				mpi_log->fwrite(LMT_INFO, "catch notification flags=0x%02x; object=0x%x", pn->flags, pn->object);
				if(pn->flags & NF_REMOVE)
					_gpi_repo_->object_release(pn->object);
				break;
			}
		}
		return true;
	}
};

static cTest _g_object_;
