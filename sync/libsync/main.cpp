#include "unistd.h"
#include "startup.h"
#include "private.h"

#define SYNC_RESOLUTION	500000 // ms
#define SYNC_CONFIG	"sync.json"

IMPLEMENT_BASE_ARRAY("ext_sync", 10);

class cExtSync: public iExtSync {
private:
	iFS	*mpi_fs;
	iJSON	*mpi_json;
	iLog	*mpi_log;
	iMutex	*mpi_mutex;
	volatile bool m_running, m_stopped, m_enable;

	void run(void) {
		m_running = true;
		m_stopped = false;

		while(m_running) {
			HMUTEX hm = mpi_mutex->lock();

			if(mpi_fs && m_enable) {
				sync_init(mpi_fs, mpi_log);

				if(mpi_json && m_enable) {
					config_init(mpi_fs, mpi_json, mpi_log, SYNC_CONFIG);

					if(config_touch())
						config_load();
				}

				do_sync();
			}

			mpi_mutex->unlock(hm);
			usleep(SYNC_RESOLUTION);
		}

		m_stopped = true;
	}

	void stop(void) {
		m_running = false;

		while(!m_stopped)
			usleep(10000);
	}
public:
	BASE(cExtSync, "cExtSync", RF_ORIGINAL | RF_TASK, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = true;

		switch(cmd) {
			case OCTL_INIT:
				break;
			case OCTL_UNINIT:
				break;
			case OCTL_START:
				run();
				break;
			case OCTL_STOP:
				stop();
				break;
		}

		return r;
	}

BEGIN_LINK_MAP
	LINK(mpi_mutex, I_MUTEX, NULL, RF_CLONE, NULL, NULL),
	LINK(mpi_log, I_LOG, NULL, RF_ORIGINAL, NULL, NULL),
	LINK(mpi_fs, I_FS, NULL, RF_ORIGINAL | RF_PLUGIN, [](_u32 n, void *udata) {
		cExtSync *p = (cExtSync *)udata;

		if(n == RCTL_REF) {
			p->mpi_log->write(LMT_INFO, "ExtSync: Attach FS support");
			HMUTEX hm = p->mpi_mutex->lock();

			//...
			p->m_enable = true;
			p->mpi_mutex->unlock(hm);
		} else if(n == RCTL_UNREF) {
			p->mpi_log->write(LMT_INFO, "ExtSync: Detach FS support");
			HMUTEX hm = p->mpi_mutex->lock();

			p->m_enable = false;
			//...
			p->mpi_mutex->unlock(hm);
		}
	}, this),
	LINK(mpi_json, I_JSON, NULL, RF_ORIGINAL | RF_PLUGIN, [](_u32 n, void *udata) {
		cExtSync *p = (cExtSync *)udata;

		if(n == RCTL_REF) {
			p->mpi_log->write(LMT_INFO, "ExtSync: Attach  JSON parser");
			HMUTEX hm = p->mpi_mutex->lock();

			//...
			p->m_enable = true;
			p->mpi_mutex->unlock(hm);
		} else if(n == RCTL_UNREF) {
			p->mpi_log->write(LMT_INFO, "ExtSync: Detach JSON parser");
			HMUTEX hm = p->mpi_mutex->lock();

			p->m_enable = false;
			//...
			p->mpi_mutex->unlock(hm);
		}
	}, this)
END_LINK_MAP

};

static cExtSync _g_ext_sync_;
