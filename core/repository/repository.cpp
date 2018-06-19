#include <string.h>
#include <limits.h>
#include "iRepository.h"
#include "iMemory.h"
#include "iTaskMaker.h"
#include "private.h"

typedef struct {
	iBase	*monitored;
	_cstr_t	iname;
	_cstr_t	cname;
	iBase	*handler;
}_notify_t;

class cRepository:public iRepository {
private:
	iLlist *mpi_cxt_list; // context list
	iLlist *mpi_ext_list; // extensions list
	iLlist *mpi_notify_list;
	iTaskMaker *mpi_tmaker;
	_cstr_t	m_ext_dir;

	iRepoExtension *get_extension(_cstr_t file, _cstr_t alias) {
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

	void remove_extension(iRepoExtension *pi_ext) {
		HMUTEX hm = mpi_ext_list->lock();
		_u32 sz = 0;
		iRepoExtension **ppx = (iRepoExtension **)mpi_ext_list->first(&sz, hm);

		while(ppx) {
			if(*ppx == pi_ext) {
				mpi_ext_list->del(hm);
				break;
			}
			ppx = (iRepoExtension **)mpi_ext_list->next(&sz, hm);
		}

		mpi_ext_list->unlock(hm);
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

	_base_entry_t *find_in_array(_object_request_t *req, _base_entry_t *array, _u32 count, bool check_disable_flag) {
		_base_entry_t *r = 0;
		_u32 it = 0;

		while(array && it < count) {
			_base_entry_t *pe = &array[it];
			if(pe->pi_base) {
				bool check = true;

				if(check_disable_flag) {
					if(pe->state & ST_DISABLED)
						check = false;
				}

				if(check) {
					if(compare(req, pe->pi_base)) {
						r = pe;
						break;
					}
				}
			}
			it++;
		}
		return r;
	}

	_base_entry_t *find(_object_request_t *req, bool check_disable_flag=true) {
		_base_entry_t *r = 0;
		_u32 count, limit;
		_base_entry_t *array = get_base_array(&count, &limit); // local vector

		if(!(r = find_in_array(req, array, count, check_disable_flag))) {
			_u32 sz;
			HMUTEX hm = mpi_ext_list->lock();
			iRepoExtension **px = (iRepoExtension **)mpi_ext_list->first(&sz, hm);

			if(px) {
				do {
					if((array = (*px)->array(&count, &limit))) {
						if((r = find_in_array(req, array, count, check_disable_flag)))
							break;
					}
				} while((px = (iRepoExtension**)mpi_ext_list->next(&sz, hm)));
			}
			mpi_ext_list->unlock(hm);
		}

		return r;
	}

	void disable_array(_base_entry_t *array, _u32 count) {
		_u32 it = 0;

		while(array && it < count) {
			_base_entry_t *pe = &array[it];
			pe->state |= ST_DISABLED;
			it++;
		}
	}

	_u32 get_references(_base_entry_t *array, _u32 count) {
		_u32 r = 0;
		_u32 it = 0;

		while(array && it < count) {
			r += array[it].ref_cnt;
			it++;
		}

		return r;
	}

	void remove_notifications(iBase *handler) {
		if(mpi_notify_list) {
			HMUTEX hm = mpi_notify_list->lock();
			_u32 sz = 0;
			_notify_t *rec = (_notify_t *)mpi_notify_list->first(&sz, hm);

			while(rec) {
				if(rec->handler == handler) {
					mpi_notify_list->del(hm);
					rec = (_notify_t *)mpi_notify_list->current(&sz, hm);
				} else
					rec = (_notify_t *)mpi_notify_list->next(&sz, hm);
			}

			mpi_notify_list->unlock(hm);
		}
	}

	void notify(_u32 flags, iBase *mon_obj) {
		if(mpi_notify_list) {
			_notification_t msg = {flags, mon_obj};
			_object_info_t info;

			mon_obj->object_info(&info);

			HMUTEX hm = mpi_notify_list->lock();
			bool send = false;
			_u32 sz = 0;
			_notify_t *rec = (_notify_t *)mpi_notify_list->first(&sz, hm);

			while(rec) {
				if(rec->monitored && rec->monitored == mon_obj)
					send = true;
				else if(rec->iname && strcmp(rec->iname, info.iname) == 0)
					send = true;
				else if(rec->cname && strcmp(rec->cname, info.cname) == 0)
					send = true;

				if(send) {
					mpi_notify_list->unlock(hm);
					rec->handler->object_ctl(OCTL_NOTIFY, &msg);
					hm = mpi_notify_list->lock();
					if(!mpi_notify_list->sel(rec, hm))
						break;

					send = false;
				}

				rec = (_notify_t *)mpi_notify_list->next(&sz, hm);
			}

			mpi_notify_list->unlock(hm);
		}
	}

	bool init_object(iBase *pi, _u8 *state, _object_info_t *info) {
		bool r = false;

		if(!(*state & ST_INITIALIZED)) {
			if((r = pi->object_ctl(OCTL_INIT, this))) {
				_u32 flags = NF_INIT;
				*state |= ST_INITIALIZED;
				if((info->flags & RF_TASK) && mpi_tmaker) {
					if(mpi_tmaker->start(pi))
						flags |= NF_START;
				}

				notify(flags, pi);
			} else
				pi->object_ctl(OCTL_UNINIT, this);
		} else
			r = true;

		return r;
	}

	bool uninit_object(iBase *pi, _u8 *state, _u32 *ref_cnt, _object_info_t *info, _u32 f=0) {
		bool r = false;
		_u32 flags = f;

		if(*state & ST_INITIALIZED) {
			flags |= NF_UNINIT;
			if(info->flags & RF_TASK)
				flags |= NF_STOP;

			notify(flags, pi);

			if(!(*state & ST_INITIALIZED))
				// removed
				return true;

			if((flags & NF_REMOVE) && *ref_cnt)
				// the object still busy, can't be removed
				return false;

			remove_notifications(pi);

			if((flags & NF_STOP) && mpi_tmaker) {
				HTASK h = mpi_tmaker->handle(pi);
				if(h)
					mpi_tmaker->stop(h);
			}

			if((r = pi->object_ctl(OCTL_UNINIT, this)))
				*state &= ~ST_INITIALIZED;
		} else
			r = true;

		return r;
	}

	void remove_context(iBase *obj, HMUTEX hlock=0) {
		HMUTEX hm = mpi_cxt_list->lock(hlock);

		if(mpi_cxt_list->sel(obj, hm))
			mpi_cxt_list->del(hm);

		mpi_cxt_list->unlock(hm);
	}

#define UNINIT_STATIC		1
#define UNINIT_DYNAMIC		2
#define UNINIT_REMOVE_CONTEXT	4

	void uninit_array(_base_entry_t *array, _u32 count,
			_u8 flags=UNINIT_STATIC|UNINIT_DYNAMIC|UNINIT_REMOVE_CONTEXT) {
		_u32 it = 0;
		_object_info_t oi;

		while(array && it < count && (flags & UNINIT_STATIC)) {
			_base_entry_t *pe = &array[it];

			pe->pi_base->object_info(&oi);
			if(oi.flags & RF_ORIGINAL)
				uninit_object(pe->pi_base, &pe->state, &pe->ref_cnt, &oi, NF_REMOVE);
			it++;
		}

		it = 0;
		while(array && it < count && (flags & UNINIT_DYNAMIC)) {
			_base_entry_t *pe = &array[it];

			pe->pi_base->object_info(&oi);
			if(oi.flags & RF_CLONE) {
				_u32 sz;
				_u8 *state;
				_object_info_t info;
				HMUTEX hm = mpi_cxt_list->lock();
				iBase *obj = (iBase *)mpi_cxt_list->first(&sz, hm);

				while(obj) {
					state = (_u8 *)obj;
					state += sz;
					obj->object_info(&info);
					if(strcmp(oi.iname, info.iname) == 0 &&
							strcmp(oi.cname, info.cname) == 0 &&
							oi.version.version == info.version.version) {
						mpi_cxt_list->unlock(hm);
						bool r = uninit_object(obj, state, &pe->ref_cnt, &info, NF_REMOVE);
						hm = mpi_cxt_list->lock();
						if(r && (flags & UNINIT_REMOVE_CONTEXT)) {
							remove_context(obj, hm);
							obj = (iBase *)mpi_cxt_list->current(&sz, hm);
							continue;
						}
					}
					obj = (iBase *)mpi_cxt_list->next(&sz, hm);
				}

				mpi_cxt_list->unlock(hm);
			}
			it++;
		}
	}

	void destroy(void) {
		_u32 sz = 0;
		_u32 count = 0, limit = 0;
		_base_entry_t *array = 0;
		HMUTEX hm = mpi_ext_list->lock();
		iRepoExtension **pprx = (iRepoExtension **)mpi_ext_list->first(&sz, hm);

		while(pprx) {
			array = (*pprx)->array(&count, &limit);
			disable_array(array, count);
			uninit_array(array, count);
			(*pprx)->unload();
			mpi_ext_list->del(hm);
			pprx = (iRepoExtension **)mpi_ext_list->current(&sz, hm);
		}

		mpi_ext_list->unlock(hm);

		array = get_base_array(&count, &limit);
		disable_array(array, count);
		uninit_array(array, count, UNINIT_STATIC);
	}

	void scan_array_for_notifications(_base_entry_t *array, _u32 count, _notify_t *pn) {
		_u32 i = 0;

		for(; i < count; i++) {
			if(array[i].pi_base) {
				_object_info_t oi;
				_notification_t msg;
				bool send = false;

				array[i].pi_base->object_info(&oi);
				if(oi.flags & RF_ORIGINAL) {
					if(array[i].pi_base == pn->monitored)
						send = true;
					else if(pn->iname && strcmp(pn->iname, oi.iname) == 0)
						send = true;
					else if(pn->cname && strcmp(pn->cname, oi.cname) == 0)
						send = true;

					if(send) {
						msg.flags = NF_INIT;
						msg.object = array[i].pi_base;

						pn->handler->object_ctl(OCTL_NOTIFY, &msg);
						send = false;
					}
				}
			}
		}
	}

	void scan_for_notifications(_notify_t *pn, _u8 scan_flags) {
		if(scan_flags & SCAN_ORIGINAL) {
			_u32 count = 0, limit = 0;
			_base_entry_t *p_array = get_base_array(&count, &limit);

			// scan core array
			if(p_array)
				scan_array_for_notifications(p_array, count, pn);

			// scan in extensions
			HMUTEX hxm = mpi_ext_list->lock();
			_u32 sz = 0;
			iRepoExtension **pprx = (iRepoExtension **)mpi_ext_list->first(&sz, hxm);

			while(pprx) {
				p_array = (*pprx)->array(&count, &limit);
				mpi_ext_list->unlock(hxm);
				scan_array_for_notifications(p_array, count, pn);
				hxm = mpi_ext_list->lock();
				mpi_ext_list->sel(pprx, hxm);
				pprx = (iRepoExtension **)mpi_ext_list->next(&sz, hxm);
			}

			mpi_ext_list->unlock(hxm);
		}

		if(scan_flags & SCAN_CLONE) {
			HMUTEX hcm = mpi_cxt_list->lock();
			_u32 cxt_size = 0;
			_object_info_t oi;
			iBase *p_cxt = (iBase *)mpi_cxt_list->first(&cxt_size, hcm);
			bool send = false;
			_notification_t msg;

			while(p_cxt) {
				p_cxt->object_info(&oi);
				if(p_cxt == pn->monitored)
					send = true;
				else if(pn->iname && strcmp(pn->iname, oi.iname) == 0)
					send = true;
				else if(pn->cname && strcmp(pn->cname, oi.cname) == 0)
					send = true;

				if(send) {
					msg.flags = NF_INIT;
					msg.object = p_cxt;

					mpi_cxt_list->unlock(hcm);
					pn->handler->object_ctl(OCTL_NOTIFY, &msg);
					hcm = mpi_cxt_list->lock();
					mpi_cxt_list->sel(p_cxt, hcm);
					send = false;
				}

				p_cxt = (iBase *)mpi_cxt_list->next(&cxt_size, hcm);
			}

			mpi_cxt_list->unlock(hcm);
		}
	}

public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 1, 0, 0);

	void init_array(_base_entry_t *array, _u32 count) {
		_u32 it = 0;

		while(array && it < count) {
			_base_entry_t *pe = &array[it];

			if(pe->pi_base && !(pe->state & (ST_DISABLED | ST_INITIALIZED))) {
				_object_info_t oi;

				pe->pi_base->object_info(&oi);
				if(oi.flags & RF_ORIGINAL)
					init_object(pe->pi_base, &pe->state, &oi);
			}
			it++;
		}
	}

	iBase *object_request(_object_request_t *req, _rf_t rf) {
		iBase *r = 0;
		_base_entry_t *bentry = find(req);

		if(bentry) {
			_object_info_t info;

			bentry->pi_base->object_info(&info);
			if(info.flags & rf) { // validate flags
				if((info.flags & rf) & RF_CLONE) {
					_u32 size = info.size+1;

					if(mpi_cxt_list) {
						// clone object
						if((r = (iBase *)mpi_cxt_list->add(bentry->pi_base, size))) {
							_u8 *state = (_u8 *)r;
							state += size;
							*state = 0;
							if(!init_object(r, state, &info))
								remove_context(r);
							else
								bentry->ref_cnt++;
						}
					} else {
						// here we needed of heap
						_object_request_t req = {RQ_INTERFACE, 0, I_HEAP};
						iHeap *pi_heap = (iHeap*)object_request(&req, RF_ORIGINAL);
						if(pi_heap) {
							r = (iBase *)pi_heap->alloc(size);
							_u8 *state = (_u8 *)r;
							state += size;
							*state = 0;

							memcpy(r, bentry->pi_base, info.size);

							if(!init_object(r, state, &info)) {
								pi_heap->free(r, size);
								r = 0;
							} else
								bentry->ref_cnt++;

							object_release(pi_heap);
						}
					}
				} else if((info.flags & rf) & RF_ORIGINAL) {
					if(init_object(bentry->pi_base, &bentry->state, &info)) {
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
		bool unref = true;
		_base_entry_t *bentry = find(&req, false);

		if(bentry) {
			if(ptr != bentry->pi_base) {
				// cloning
				_u8 *state = (_u8 *)ptr;
				state += info.size+1;

				if((unref = uninit_object(ptr, state, &bentry->ref_cnt, &info)))
					remove_context(ptr);
			}

			if(bentry->ref_cnt && unref)
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
	void extension_dir(_cstr_t dir) {
		m_ext_dir = dir;
	}

	_err_t extension_load(_cstr_t file, _cstr_t alias=0) {
		_err_t r = ERR_UNKNOWN;
		_char_t path[PATH_MAX]="";

		sprintf(path, "%s/%s", m_ext_dir, file);

		if(!get_extension(path, 0)) {
			iRepoExtension *px = (iRepoExtension*)object_by_iname(I_REPO_EXTENSION, RF_CLONE);
			if(px) {
				if((r = px->load(path, alias)) == ERR_NONE) {
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

	_err_t extension_unload(_cstr_t alias) {
		_err_t r = ERR_UNKNOWN;
		iRepoExtension *pi_ext = get_extension(0, alias);

		if(pi_ext) {
			_u32 count = 0, limit = 0;
			_base_entry_t *array = pi_ext->array(&count, &limit);

			if(array) {
				disable_array(array, count);
				uninit_array(array, count);
				if(get_references(array, count) == 0) {
					if((r = pi_ext->unload()) == ERR_NONE) {
						remove_extension(pi_ext);
						object_release(pi_ext);
					}
				} else
					r = ERR_BUSY;
			}
		}

		return r;
	}

	// notifications
	HNOTIFY monitoring_add(iBase *mon_obj,
				_cstr_t mon_iname,
				_cstr_t mon_cname,
				iBase *handler_obj,
				_u8 scan_flags) {
		_notify_t *r = 0;

		if(mpi_notify_list && handler_obj) {
			_notify_t n = {	mon_obj, mon_iname, mon_cname, handler_obj };
			if((r = (_notify_t *)mpi_notify_list->add(&n, sizeof(_notify_t)))) {
				if(scan_flags)
					scan_for_notifications(&n, scan_flags);
			}
		}

		return r;
	}

	void monitoring_remove(HNOTIFY h) {
		_notify_t *rec = (_notify_t *)h;

		if(mpi_notify_list) {
			HMUTEX hm = mpi_notify_list->lock();
			if(mpi_notify_list->sel(rec, hm))
				mpi_notify_list->del(hm);
			mpi_notify_list->unlock(hm);
		}
	}

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				mpi_cxt_list = mpi_ext_list = mpi_notify_list = 0;
				mpi_tmaker = 0;
				m_ext_dir = "";
				if((mpi_cxt_list = (iLlist*)object_by_iname(I_LLIST, RF_CLONE)))
					mpi_cxt_list->init(LL_VECTOR, 1);
				mpi_ext_list = (iLlist*)object_by_iname(I_LLIST, RF_CLONE);
				mpi_tmaker = (iTaskMaker*)object_by_iname(I_TASK_MAKER, RF_ORIGINAL);
				mpi_notify_list = (iLlist *)object_by_iname(I_LLIST, RF_CLONE);
				if(mpi_cxt_list && mpi_ext_list && mpi_tmaker && mpi_notify_list) {
					mpi_ext_list->init(LL_VECTOR, 1);
					mpi_notify_list->init(LL_VECTOR, 1);
					r = true;
				}
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	// enumeration
#define ENUM_CORE_EXT	(_enum_ext_t)0x1cc2cc
#define ENUM_CORE_ALIAS	"core"

	_enum_ext_t enum_ext_first(void) {
		return ENUM_CORE_EXT;
	}

	_enum_ext_t enum_ext_next(_enum_ext_t en) {
		_enum_ext_t r = 0;
		_u32 sz;

		if(en == ENUM_CORE_EXT)
			return mpi_ext_list->first(&sz);

		HMUTEX hm = mpi_ext_list->lock();

		if(mpi_ext_list->sel(en, hm))
			r = mpi_ext_list->next(&sz, hm);

		mpi_ext_list->unlock(hm);

		return r;
	}

	_cstr_t enum_ext_alias(_enum_ext_t en) {
		_cstr_t r = 0;

		if(en == ENUM_CORE_EXT)
			return ENUM_CORE_ALIAS;

		HMUTEX hm = mpi_ext_list->lock();

		if(mpi_ext_list->sel(en, hm)) {
			_u32 sz = 0;
			iRepoExtension **ppx = (iRepoExtension **)mpi_ext_list->current(&sz, hm);
			if(ppx) {
				r = (*ppx)->alias();
			}
		}

		mpi_ext_list->unlock(hm);

		return r;
	}

	_u32 enum_ext_array_count(_enum_ext_t en) {
		_u32 r = 0;
		_u32 limit=0;

		if(en == ENUM_CORE_EXT)
			get_base_array(&r, &limit);
		else {
			_u32 sz = 0;
			HMUTEX hm = mpi_ext_list->lock();

			if(mpi_ext_list->sel(en, hm)) {
				iRepoExtension **ppx = (iRepoExtension **)mpi_ext_list->current(&sz, hm);
				if(ppx)
					(*ppx)->array(&r, &limit);
			}

			mpi_ext_list->unlock(hm);
		}

		return r;
	}

	_u32 enum_ext_array_limit(_enum_ext_t en) {
		_u32 r = 0;
		_u32 sz = 0;
		_u32 count=0;

		if(en == ENUM_CORE_EXT)
			get_base_array(&count, &r);
		else {
			HMUTEX hm = mpi_ext_list->lock();

			if(mpi_ext_list->sel(en, hm)) {
				iRepoExtension **ppx = (iRepoExtension **)mpi_ext_list->current(&sz, hm);
				if(ppx)
					(*ppx)->array(&count, &r);
			}

			mpi_ext_list->unlock(hm);
		}

		return r;
	}

	bool enum_ext_array(_enum_ext_t en, _u32 aidx, _base_entry_t *p_base_entry) {
		bool r = false;
		_u32 sz = 0;
		_u32 count, limit;
		_base_entry_t *pbe = 0;

		if(en == ENUM_CORE_EXT) {
			pbe = get_base_array(&count, &limit);
			if(pbe && aidx < count) {
				memcpy(p_base_entry, &pbe[aidx], sizeof(_base_entry_t));
				r = true;
			}
		} else {
			HMUTEX hm = mpi_ext_list->lock();

			if(mpi_ext_list->sel(en, hm)) {
				iRepoExtension **ppx = (iRepoExtension **)mpi_ext_list->current(&sz, hm);
				if(ppx) {
					pbe = (*ppx)->array(&count, &limit);
					if(pbe && aidx < count) {
						memcpy(p_base_entry, &pbe[aidx], sizeof(_base_entry_t));
						r = true;
					}
				}
			}

			mpi_ext_list->unlock(hm);
		}

		return r;
	}
};

static cRepository _g_object_;
