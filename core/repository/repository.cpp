#include "iRepository.h"
#include "iMemory.h"
#include "private.h"

class cRepository:public iRepository {
private:
	iHeap  *mpi_heap;
	iLlist *mpi_cxt_list; // context list
	iLlist *mpi_ext_list; // extensions list
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

	// extensions
	_err_t extension_load(_str_t file, _str_t name=0) {
		_err_t r = ERR_UNKNOWN;
		//...
		return r;
	}
	_err_t extension_unload(_str_t name) {
		_err_t r = ERR_UNKNOWN;
		//...
		return r;
	}


	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				mpi_heap = 0;
				mpi_heap = (iHeap*)object_by_interface(I_HEAP, RF_ORIGINAL);
				mpi_cxt_list = mpi_ext_list = 0;
				mpi_cxt_list = (iLlist*)object_by_interface(I_LLIST, RF_CLONE);
				mpi_ext_list = (iLlist*)object_by_interface(I_LLIST, RF_CLONE);
				r = true;
				break;
			case OCTL_UNINIT:
				//...
				r = true;
				break;
		}
		return r;
	}
};

static cRepository _g_object_;
