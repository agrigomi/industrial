#include <string.h>
#include "iRepository.h"
#include "iMemory.h"
#include "private.h"

class cRepository:public iRepository {
private:
	iHeap  *mpi_heap;
	iLlist *mpi_cxt_list; // context list
	iLlist *mpi_ext_list; // extensions list

	iRepoExtension *get_extension(_str_t file, _str_t alias) {
		iRepoExtension *r = 0;
		if(mpi_ext_list) {
			_u32 sz = 0;
			bool a = (alias)?false:true, f = (file)?false:true;
			HMUTEX hl = mpi_ext_list->lock();
			iRepoExtension *px = (iRepoExtension *)mpi_ext_list->first(&sz, hl);

			if(px) {
				do {
					if(file)
						f = (strcmp(file, px->file()) == 0);

					if(alias)
						a = (strcmp(alias, px->alias()) == 0);

					if(a && f) {
						r = px;
						break;
					}
				} while((px = (iRepoExtension *)mpi_ext_list->next(&sz, hl)));
			}

			mpi_ext_list->unlock(hl);
		}
		return r;
	}

public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 1, 0, 0);

	iBase *object_request(_object_request_t *req, _rf_t rf) {
		iBase *r = 0;
		//...
		return r;
	}

	void object_release(iBase *ptr) {
	}

	iBase *object_by_cname(_cstr_t name, _rf_t rf) {
		_object_request_t req;
		req.cname = name;
		req.flags = RQ_NAME;
		return object_request(&req, rf);
	}

	iBase *object_by_iname(_cstr_t name, _rf_t rf) {
		_object_request_t req;
		req.iname = name;
		req.flags = RQ_INTERFACE;
		return object_request(&req, rf);
	}

	// extensions
	_err_t extension_load(_str_t file, _str_t alias=0) {
		_err_t r = ERR_UNKNOWN;
		if(!get_extension(file, alias)) {
			iRepoExtension *px = object_by_iname(I_REPO_EXTENSION, RF_CLONE);
			if(px)
				r = px->load(file, alias);
			 else
				r = ERR_MISSING;
		} else
			r = ERR_DUPLICATED;
		return r;
	}

	_err_t extension_unload(_str_t alias) {
		_err_t r = ERR_UNKNOWN;
		//...
		return r;
	}

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				mpi_heap = 0;
				mpi_heap = (iHeap*)object_by_iname(I_HEAP, RF_ORIGINAL);
				mpi_cxt_list = mpi_ext_list = 0;
				mpi_cxt_list = (iLlist*)object_by_iname(I_LLIST, RF_CLONE);
				mpi_ext_list = (iLlist*)object_by_iname(I_LLIST, RF_CLONE);
				if(mpi_cxt_list && mpi_ext_list) {
					mpi_cxt_list->init(LL_VECTOR, 1);
					mpi_ext_list->init(LL_VECTOR, 1);
					r = true;
				}
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
