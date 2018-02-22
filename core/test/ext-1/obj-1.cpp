#include "private.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "startup.h"

IMPLEMENT_BASE_ARRAY(10);

class cObj_1: public iObj_1 {
private:
	iRepository *mpi_repo;
	iLog *mpi_log;

	iLog *get_log(void) {
		iLog *r = mpi_log;

		if(!r && mpi_repo)
			r = (iLog *)mpi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		return r;
	}

public:
	BASE(cObj_1, "cObj_1", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				mpi_repo = (iRepository *)arg;
				mpi_log = get_log();
				if(mpi_log)
					mpi_log->fwrite(LMT_INFO, "init %s", "cObj_1");
				r = true;
				break;
			case OCTL_UNINIT:
				if(mpi_log)
					mpi_log->fwrite(LMT_INFO, "uninit %s", "cObj_1");
				r = true;
				break;
		}
		return r;
	}

	void method_1(void) {
	}

	void method_2(void) {
	}
};

static cObj_1 _g_object_1_;
