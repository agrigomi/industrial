#include "unistd.h"
#include "startup.h"
#include "iFS.h"
#include "iLog.h"
#include "iExtSync.h"

#define SYNC_RESOLUTION	500000 // ms

IMPLEMENT_BASE_ARRAY("ext_sync", 10);

class cExtSync: public iExtSync {
private:
	iFS	*mpi_fs;
	iLog	*mpi_log;
	iMutex	*mpi_mutex;
	volatile bool m_running, m_stopped, m_enable;

	void run(void) {
		m_running = true;
		m_stopped = false;

		while(m_running) {
			HMUTEX hm = mpi_mutex->lock();

			if(mpi_fs && m_enable) {
				//...
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
	}, this)
END_LINK_MAP

};

static cExtSync _g_ext_sync_;
