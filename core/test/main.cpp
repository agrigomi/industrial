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
#include "iHT.h"

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
		iLog *pi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);

		pi_log->add_listener(log_listener);

		pi_repo->extension_dir("./bin/deploy");
		pi_repo->extension_load("extht.so");
		//pi_repo->extension_load("ext-1.so");
		pi_repo->extension_load("extfs.so");

		iFS *pi_fs = (iFS *)pi_repo->object_by_iname(I_FS, RF_ORIGINAL);
		if(pi_fs) {
			iFileIO *pi_fio = pi_fs->open("test.xml", O_RDONLY);
			if(pi_fio) {
				iXML *pi_xml = (iXML *)pi_repo->object_by_iname(I_XML, RF_ORIGINAL);
				if(pi_xml) {
					_str_t xml = (_str_t)pi_fio->map(MPF_READ);
					_ulong xlen = pi_fio->size();
					HTCONTEXT hc = pi_xml->create_context();
					if(pi_xml->parse(hc, xml, xlen)) {
						HTTAG c1 = pi_xml->select(hc, "/t1/c1", NULL, 0);
						//...
					}
					pi_xml->destroy_context(hc);
					pi_repo->object_release(pi_xml);
				}

				pi_fs->close(pi_fio);
			}
			pi_repo->object_release(pi_fs);
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
