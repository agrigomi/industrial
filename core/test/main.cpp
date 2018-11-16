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

		pi_repo->extension_dir("./bin/deploy/unix");
		pi_repo->extension_load("extht.so");
		//pi_repo->extension_load("ext-1.so");
		pi_repo->extension_load("extfs.so");
		pi_repo->extension_load("extnet.so");
/*
		iBufferMap *pi_bmap = dynamic_cast<iBufferMap *>(pi_repo->object_by_iname(I_BUFFER_MAP, RF_CLONE));
		if(pi_bmap) {
			pi_bmap->init(8192, NULL);
			HBUFFER b1 = pi_bmap->alloc();
			pi_bmap->free(b1);
			b1 = pi_bmap->alloc();
			HBUFFER b2 = pi_bmap->alloc();
			pi_bmap->free(b1);
			pi_bmap->free(b2);
			b1 = pi_bmap->alloc();
			b2 = pi_bmap->alloc();
		}
*/
/*		iNet *pi_net = (iNet*)pi_repo->object_by_iname(I_NET, RF_ORIGINAL);
		if(pi_net) {
			iSocketIO *sio = pi_net->create_multicast_listener("239.0.0.1", 9000);
			if(sio) {
				int i = 10;
				char b[20]="";

				while(i--) {
					if(sio->read(b, 20))
						printf(b);
				}
				pi_net->close_socket(sio);
			}
			pi_repo->object_release(pi_net);
		}
*/
		iFS *pi_fs = dynamic_cast<iFS*>(pi_repo->object_by_iname(I_FS, RF_ORIGINAL));
		if(pi_fs) {
			iFileIO *pi_fio = pi_fs->open("test.xml", O_RDONLY);
			if(pi_fio) {
				iXML *pi_xml = (iXML *)pi_repo->object_by_iname(I_XML, RF_ORIGINAL);
				if(pi_xml) {
					_str_t xml = (_str_t)pi_fio->map(MPF_READ);
					_ulong xlen = pi_fio->size();
					HTCONTEXT hc = pi_xml->create_context();
					if(pi_xml->parse(hc, xml, xlen)) {
						_u32 sz  = 0;

						HTTAG test = pi_xml->select(hc, "/t-1/test", NULL, 0);
						if(test) {
							_str_t test_content = pi_xml->content(test, &sz);
							fwrite(test_content, sz, 1, stdout);
							asm("nop");
						} else
							pi_log->write(LMT_ERROR, "XML: No tag 't-1/test' found\n");
						//...

					} else
						pi_log->write(LMT_ERROR, "XML parse error");
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
