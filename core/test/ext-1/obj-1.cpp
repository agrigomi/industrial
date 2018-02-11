#include "private.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"

class cObj_1: public iObj_1 {
private:
	iRepository *mpi_repo;
	iLog *mpi_log;

	iLog *get_log(void) {
		iLog *r = 0;

		if(!mpi_log && mpi_repo)
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
				r = true;
				break;
			case OCTL_UNINIT:
				break;
		}
		return r;
	}

	void method_1(void) {
	}

	void method_2(void) {
	}
};
