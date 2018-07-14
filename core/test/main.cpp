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

const char *xml="\
<tag1>\
	<chtag1 param1=\"v1\">\
		<inltag param=\"value\"/>\
	</chtag1>\
</tag1>\
";

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();

		pi_repo->extension_dir("./bin/deploy");
		pi_repo->extension_load("extht.so");
		pi_repo->extension_load("ext-1.so");

		iHT *pi_ht = (iHT *)pi_repo->object_by_iname(I_HT, RF_ORIGINAL);
		if(pi_ht) {
			HTCONTEXT hc = pi_ht->create_context();
			_ulong xlen = strlen(xml);
			pi_ht->parse(hc, (_str_t)xml, xlen);
			//...
			pi_ht->destroy_context(hc);
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
