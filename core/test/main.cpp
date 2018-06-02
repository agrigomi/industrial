#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iFS.h"
#include "iNet.h"
#include "iCmd.h"
#include "iMemory.h"

IMPLEMENT_BASE_ARRAY("core_test", 1024);

void log_listener(_u8 lmt, _str_t msg) {
	_char_t pref = '-';

	switch(lmt) {
		case LMT_TEXT: pref = 'T'; break;
		case LMT_INFO: pref = 'I'; break;
		case LMT_ERROR: pref = 'E'; break;
		case LMT_WARNING: pref = 'W';break;
	}
	printf("[%c] %s\n", pref, msg);
}

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		if(pi_log)
			pi_log->add_listener(log_listener);

		iHeap *pi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
		if(pi_heap) {
			//...
			pi_repo->object_release(pi_heap);
		}

		pi_repo->extension_load((_str_t)"bin/core/unix/ext-1/ext-1.so");
		pi_repo->extension_load((_str_t)"bin/io/unix/libfs/libfs.so");
		pi_repo->extension_load((_str_t)"bin/io/unix/libnet/libnet.so");
		pi_repo->extension_load((_str_t)"bin/cmd/unix/libcmd/libcmd.so");

		iCmdHost *pi_cmd_host = (iCmdHost *)pi_repo->object_by_iname(I_CMD_HOST, RF_ORIGINAL);
		if(pi_cmd_host) {
			pi_cmd_host->exec((_str_t)"help -f -p 10", 0);
			pi_repo->object_release(pi_cmd_host);
		}

		iBase *obj = pi_repo->object_by_iname("iObj1", RF_CLONE|RF_ORIGINAL);

		iFS *pi_fs = (iFS *)pi_repo->object_by_iname(I_FS, RF_ORIGINAL);
		if(pi_fs) {
			iFileIO *pifio = pi_fs->open("./testfile", O_RDWR|O_CREAT);
			if(pifio) {
				pifio->write((_str_t)"alabala", 7);
				pifio->sync();
				_str_t pfc = (_str_t)pifio->map();
				if(pfc) {
					pi_log->fwrite(LMT_TEXT, "file size: %d; file content: %s", pifio->size(), pfc);
				}
				pi_fs->close(pifio);
				pi_fs->remove("./testfile");
			}

			iDir *pi_dir = pi_fs->open_dir("./");
			if(pi_dir) {
				_str_t fname;
				_u8 type;

				if(pi_dir->first(&fname, &type)) {
					do {
						pi_log->fwrite(LMT_TEXT, "dentry: '%s'", fname);
					}while(pi_dir->next(&fname, &type));
				}
				pi_fs->close_dir(pi_dir);
			}
			pi_repo->object_release(pi_fs);
		}

		iNet *pi_net = (iNet *)pi_repo->object_by_iname(I_NET, RF_ORIGINAL);
		if(pi_net) {
			iSocketIO *sio = pi_net->create_tcp_client((_str_t)"localhost", 3000);
			if(sio) {
				//...
				pi_net->close_socket(sio);
			}
			pi_repo->object_release(pi_net);
		}

		getchar();
		if((r = pi_repo->extension_unload("libfs.so")))
			pi_log->fwrite(LMT_ERROR, "unable to unload libfs.so error %d", r);
		else
			pi_log->write(LMT_INFO, "libfs.so, unloaded");
		pi_repo->object_release(obj);
		if((r = pi_repo->extension_unload("ext-1.so")))
			pi_log->fwrite(LMT_ERROR, "unable to unload ext-1.so error %d", r);
		else
			pi_log->write(LMT_INFO, "ext-1.so, unloaded");
		pi_log->write(LMT_INFO, "-------------------------");
		_str_t lm = pi_log->first();
		while(lm) {
			printf("%s\n", lm);
			lm = pi_log->next();
		}
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
				pi_repo->monitoring_add(0, I_FS, 0, this);
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
