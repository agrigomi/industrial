#include "iRepository.h"
#include "iMemory.h"

class cRepository:public iRepository {
private:
	iHeap *mpi_heap;
public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 1, 0, 0);

	iBase *object_request(_object_request_t *req, _rf_t rf) {
		iBase *r = 0;
		//...
		return r;
	}

	void object_release(iBase *ptr) {
	}

	iBase *object_by_name(_cstr_t name, _rf_t rf) {
		iBase *r = 0;
		//...
		return r;

	}

	iBase *object_by_interface(_cstr_t name, _rf_t rf) {
		iBase *r = 0;
		//...
		return r;
	}



	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				//...
				r = true;
				break;
		}
		return r;
	}
};

static cRepository _g_object_;
