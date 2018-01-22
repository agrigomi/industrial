#include <string.h>
#include "iRepository.h"
#include "iMemory.h"
#include "private.h"

class cRepository:public iRepository {
private:
	iLlist *mpi_cxt_list; // context list
	iLlist *mpi_ext_list; // extensions list

	iRepoExtension *get_extension(_str_t file, _str_t alias) {
		iRepoExtension *r = 0;
		if(mpi_ext_list) {
			_u32 sz = 0;
			bool a = (alias)?false:true, f = (file)?false:true;
			HMUTEX hl = mpi_ext_list->lock();
			iRepoExtension **px = (iRepoExtension **)mpi_ext_list->first(&sz, hl);

			if(px) {
				do {
					if(file)
						f = (strcmp(file, (*px)->file()) == 0);

					if(alias)
						a = (strcmp(alias, (*px)->alias()) == 0);

					if(a && f) {
						r = *px;
						break;
					}
				} while((px = (iRepoExtension **)mpi_ext_list->next(&sz, hl)));
			}

			mpi_ext_list->unlock(hl);
		}
		return r;
	}

	bool compare(_object_request_t *req, iBase *p) {
		bool r = false;
		_object_info_t inf;

		p->object_info(&inf);

		if(req->flags & RQ_CMP_OR) {
			bool r1=false,r2=false,r3=false;
			if(req->flags & RQ_NAME)
				r1 = (strcmp(req->cname, inf.cname) == 0);
			else if(req->flags & RQ_INTERFACE)
				r2 = (strcmp(req->iname, inf.iname) == 0);
			else if(req->flags & RQ_VERSION)
				r3 = (req->version.version == inf.version.version);
			r = (r1 || r2 || r3);
		} else {
			if(req->flags & RQ_NAME) {
				if(strcmp(req->cname, inf.cname) != 0)
					goto _compare_done_;
			}
			if(req->flags & RQ_INTERFACE) {
				if(strcmp(req->iname, inf.iname) != 0)
					goto _compare_done_;
			}
			if(req->flags & RQ_VERSION) {
				if(req->version.version != inf.version.version)
					goto _compare_done_;
			}
			r = true;
		}
_compare_done_:
		return r;
	}

	_base_entry_t *find_in_vector(_object_request_t *req, _base_vector_t *vector) {
		_base_entry_t *r = 0;
		_base_vector_t::iterator it = vector->begin();

		while(it != vector->end()) {
			_base_entry_t *pe = &(*it);
			if(pe->pi_base && !(pe->state & ST_DISABLED) && (pe->state & ST_INITIALIZED)) {
				if(compare(req, pe->pi_base)) {
					r = pe;
					break;
				}
			}
			it++;
		}
		return r;
	}

	_base_entry_t *find(_object_request_t *req) {
		_base_entry_t *r = 0;
		_base_vector_t *vector = get_base_vector(); // local vector

		if(!(r = find_in_vector(req, vector))) {
			_u32 sz;
			HMUTEX hm = mpi_ext_list->lock();
			iRepoExtension **px = (iRepoExtension **)mpi_ext_list->first(&sz, hm);

			if(px) {
				do {
					vector = (*px)->vector();
					if(vector) {
						if((r = find_in_vector(req, vector)))
							break;
					}
				} while((px = (iRepoExtension**)mpi_ext_list->next(&sz, hm)));
			}
			mpi_ext_list->unlock(hm);
		}

		return r;
	}

public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 1, 0, 0);

	iBase *object_request(_object_request_t *req, _rf_t rf) {
		iBase *r = 0;
		_base_entry_t *bentry = find(req);

		if(bentry) {
			_object_info_t info;

			bentry->pi_base->object_info(&info);
			if(info.flags & rf) { // validate flags
				if((info.flags & rf) & RF_CLONE) {
					// clone object
					if((r = (iBase *)mpi_cxt_list->add(bentry->pi_base, info.size))) {
						if(r->object_ctl(OCTL_INIT, this))
							bentry->ref_cnt++;
						else { // release
							HMUTEX hm = mpi_cxt_list->lock();
							if(mpi_cxt_list->sel(r, hm)) {
								mpi_cxt_list->del(hm);
								r = 0;
							}
							mpi_cxt_list->unlock(hm);
						}
					}
				} else {
					if((info.flags & rf) & RF_ORIGINAL) {
						r = bentry->pi_base;
						bentry->ref_cnt++;
					}
				}
			}
		}

		return r;
	}

	void object_release(iBase *ptr) {
		_object_info_t info;

		ptr->object_info(&info);
		_object_request_t req = {RQ_NAME|RQ_INTERFACE|RQ_VERSION,
					info.cname, info.iname};
		req.version.version = info.version.version;

		_base_entry_t *bentry = find(&req);

		if(bentry) {
			if(ptr != bentry->pi_base) {
				// cloning
				HMUTEX hm = mpi_cxt_list->lock();
				if(mpi_cxt_list->sel(ptr, hm))
					mpi_cxt_list->del(hm);
				mpi_cxt_list->unlock(hm);
			}

			if(bentry->ref_cnt)
				bentry->ref_cnt--;
		}
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
			iRepoExtension *px = (iRepoExtension*)object_by_iname(I_REPO_EXTENSION, RF_CLONE);
			if(px) {
				if((r = px->load(file, alias))) {
					iRepoExtension **_px =
						(iRepoExtension **)mpi_ext_list->add(&px, sizeof(px));
					if(_px) {
						if((r = px->init(this)) != ERR_NONE) {
							// unload
							HMUTEX hm = mpi_ext_list->lock();
							if(mpi_ext_list->sel(_px, hm))
								mpi_ext_list->del(hm);
							mpi_ext_list->unlock(hm);
							px->unload();
						}
					} else
						r = ERR_MEMORY;
				}
				if(r != ERR_NONE)
					object_release(px);
			} else
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
