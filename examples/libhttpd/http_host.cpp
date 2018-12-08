#include "iHttpHost.h"
#include "iRepository.h"
#include "iNet.h"
#include "iFS.h"
#include "iLog.h"

#define DEFAULT_HTTP_PORT	8080

class cHttpHost: public iHttpHost {
private:
	iNet		*mpi_net;
	iHttpServer	*mpi_http;
	iFileCache	*mpi_fcache;
	iLog		*mpi_log;
	//...

	void release_object(iRepository *pi_repo, iBase **pp_obj) {
		if(*pp_obj) {
			pi_repo->object_release(*pp_obj);
			*pp_obj = NULL;
		}
	}
public:
	BASE(cHttpHost, "cHttpHost", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_net = NULL;
				mpi_fcache = NULL;
				mpi_http = NULL;

				pi_repo->monitoring_add(NULL, I_NET, NULL, this);
				pi_repo->monitoring_add(NULL, I_FS, NULL, this);

				mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
				if(mpi_log)
					r = true;
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				release_object(pi_repo, (iBase **)&mpi_http);
				release_object(pi_repo, (iBase **)&mpi_net);
				release_object(pi_repo, (iBase **)&mpi_fcache);
				release_object(pi_repo, (iBase **)&mpi_log);
				r = true;
			} break;
			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				_object_info_t oi;

				memset(&oi, 0, sizeof(_object_info_t));
				if(pn->object) {
					pn->object->object_info(&oi);

					if(pn->flags & NF_INIT) { // catch
						//...
					} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
						//...
					}
				}
			} break;
		}

		return r;
	}
	//...
};

static cHttpHost _g_http_host_;
