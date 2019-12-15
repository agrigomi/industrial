#include "private.h"
#include "iMemory.h"
#include "iRepository.h"

#define CFREE		1
#define CPARTIAL	2
#define CFULL		3

class cSQLPool: public iSQLPool {
private:
	iLlist	*mpi_list;
public:
	BASE(cSQLPool, "cSQLPool", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_list = (iLlist *)_gpi_repo_->object_by_iname(I_SQL_POOL, RF_CLONE)))
					r = mpi_list->init(LL_VECTOR, 3);
				break;
			case OCTL_UNINIT:
				if(mpi_list)
					_gpi_repo_->object_release(mpi_list);
				r = true;
				break;
		}

		return r;
	}
};
