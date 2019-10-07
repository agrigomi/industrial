#include <string.h>
#include "private.h"

class cRepository: public iRepository {
private:
	_cstr_t	m_ext_dir;
	_v_pi_object_t	mv_pending; // vector for original pending objects
	_v_pi_object_t::iterator mv_it_pending; // iterator for original pending objects

	void enum_original_pending(_enum_cb_t *pcb, void *udata) {
		mv_it_pending = mv_pending.begin();

		while(mv_it_pending != mv_pending.end()) {
			iBase *pi_base = *mv_it_pending;
			_s32 x = pcb(pi_base, udata);

			if(x == ENUM_BREAK)
				break;
			else if(x == ENUM_DELETE)
				mv_pending.erase(mv_it_pending);

			mv_it_pending++;
		}
	}

	bool is_original(iBase *pi_base) {
		bool r = false;
		_base_entry_t *p_bentry = find_object_by_pointer(pi_base);

		if(p_bentry)
			// original object context
			r = true;

		return r;
	}

	_cstat_t get_context_state(iBase *pi_base) {
		_cstat_t r = 0;
		_base_entry_t *p_bentry = find_object_by_pointer(pi_base);

		if(p_bentry)
			// original object context
			r = p_bentry->state;
		else
			// dynamic object context
			r = dcs_get_context_state(pi_base);

		return r;
	}

	void set_context_state(iBase *pi_base, _cstat_t state) {
		_base_entry_t *p_bentry = find_object_by_pointer(pi_base);

		if(p_bentry)
			// original object context
			p_bentry->state = state;
		else
			// dynamic object context
			dcs_set_context_state(pi_base, state);
	}

#define PLMR_READY		(1<<0)
#define PLMR_KEEP_PENDING	(1<<1)
#define PLMR_FAILED		(1<<2)

	_u32 process_link_map(iBase *pi_base, iBase *pi_plugin=NULL) {
		_u32 r = PLMR_READY;
		_u32 count;
		const _link_info_t *pl = pi_base->object_link(&count);
		_cstat_t state = get_context_state(pi_base);

		if(!(state & ST_INITIALIZED)) {
			for(_u32 i = 0; pl && i < count; i++) {
				if(*pl[i].ppi_base == NULL && !(pl[i].flags & RF_POST_INIT)) {
					_object_request_t orq;

					memset(&orq, 0, sizeof(_object_request_t));
					if(pl[i].iname) {
						orq.flags |= RQ_INTERFACE;
						orq.iname = pl[i].iname;
					}
					if(pl[i].cname) {
						orq.flags |= RQ_NAME;
						orq.cname = pl[i].cname;
					}

					if(pl[i].flags & RF_KEEP_PENDING)
						r |= PLMR_KEEP_PENDING;

					if((*pl[i].ppi_base = object_request(&orq, pl[i].flags))) {
						if(pl[i].p_ref_ctl)
							pl[i].p_ref_ctl(RCTL_REF, pl[i].udata);

					} else {
						if(!(pl->flags & RF_NOCRITICAL) || !is_original(pi_base))
							r = PLMR_FAILED;
						else
							r = PLMR_KEEP_PENDING;
						break;
					}
				}
			}
		} else if(pi_plugin && pi_plugin != pi_base) {
			_object_info_t oi;

			pi_plugin->object_info(&oi);

			for(_u32 i = 0; pl && i < count; i++) {
				if(pl[i].flags & RF_POST_INIT) {
					bool attach = false;

					if(pl[i].cname) {
						if(strcmp(pl[i].cname, oi.cname) == 0)
							attach = true;
					} else if(pl[i].iname) {
						if(strcmp(pl[i].iname, oi.iname) == 0)
							attach = true;
					}

					if(attach) {
						if(pl[i].flags & RF_KEEP_PENDING)
							r = PLMR_KEEP_PENDING;

						*pl[i].ppi_base = pi_plugin;
						if(pl[i].p_ref_ctl)
							pl[i].p_ref_ctl(RCTL_REF, pl[i].udata);
						break;
					}
				}
			}
		} else
			r = 0;

		return r;
	}

	void update_users(iBase *pi_base) {
		_u32 count;
		const _link_info_t *pl = pi_base->object_link(&count);

		if(pl) {
			for(_u32 i = 0; i < count; i++) {
				if(*pl[i].ppi_base)
					users_add_object_user(*pl[i].ppi_base, pi_base);
			}
		}
	}

	void clean_link_map(iBase *pi_base) {
		_u32 count;
		const _link_info_t *pl = pi_base->object_link(&count);

		if(pl) {
			for(_u32 i = 0; i < count; i++)
				*pl[i].ppi_base = NULL;
		}
	}

	void release_link_map(iBase *pi_base) {
		_u32 count;
		const _link_info_t *pl = pi_base->object_link(&count);

		if(pl) {
			for(_u32 i = 0; i < count; i++) {
				if(*pl[i].ppi_base) {
					if(pl[i].p_ref_ctl)
						pl[i].p_ref_ctl(RCTL_UNREF, pl[i].udata);
					object_release(*pl[i].ppi_base);
					*pl[i].ppi_base = NULL;
				}
			}
		}
	}

	bool init_object(iBase *pi_base, bool notify=true) {
		bool r = false;
		_cstat_t state = get_context_state(pi_base);

		if(!(state & ST_INITIALIZED)) {
			_u32 f = process_link_map(pi_base);

			if(f & PLMR_KEEP_PENDING)
				state |= ST_PENDING;

			if(f & PLMR_READY) {
				if((r = pi_base->object_ctl(OCTL_INIT, this)))
					state |= ST_INITIALIZED;
			}

			set_context_state(pi_base, state);

			if(r)
				process_pending_lists(pi_base);
		} else
			r = true;

		return r;
	}

	// Process original pending list.
	void process_original_pending(iBase *pi_base=NULL) {
		typedef struct {
			cRepository	*p_repo;
			iBase		*pi_base;
		}_enum_info_t;
		_enum_info_t e = {this, pi_base};

		enum_original_pending([](iBase *pi_base, void *udata)->_s32 {
			_s32 r = ENUM_CONTINUE;
			_enum_info_t *pe = (_enum_info_t *)udata;
			_u32 f = pe->p_repo->process_link_map(pi_base, pe->pi_base);

			if(!(f & PLMR_KEEP_PENDING))
				r = ENUM_DELETE;

			if(f & PLMR_READY)
				pe->p_repo->init_object(pi_base);

			return r;
		}, &e);
	}

	// Process dynamic pending list.
	void process_clone_pending(iBase *pi_base=NULL) {
		_mutex_handle_t hm = dcs_lock();
		typedef struct {
			cRepository	*p_repo;
			iBase		*pi_base;
			_mutex_handle_t	hlock;
		}_enum_info_t;
		_enum_info_t e = {this, pi_base, hm};

		dcs_enum_pending([](iBase *pi_base, void *udata)->_s32 {
			_s32 r = ENUM_CONTINUE;
			_enum_info_t *pe = (_enum_info_t *)udata;
			_u32 f = pe->p_repo->process_link_map(pi_base, pe->pi_base);

			if(!(f & PLMR_KEEP_PENDING))
				r = ENUM_END_PENDING;

			if(f & PLMR_READY)
				pe->p_repo->init_object(pi_base);

			return r;
		}, &e, hm);

		dcs_unlock(hm);
	}

	void process_pending_lists(iBase *pi_plugin=NULL) {
		process_original_pending(pi_plugin);
		process_clone_pending(pi_plugin);
	}
public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 2,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		switch(cmd) {
			case OCTL_INIT:
				zinit();
				break;
			case OCTL_UNINIT:
				destroy();
				break;
		}

		return true;
	}

	iBase *object_request(_object_request_t *req, _rf_t flags) {
		iBase *r = NULL;

		//...

		return r;
	}

	void object_release(iBase *pi_base, bool notify=true) {
		//...
	}

	iBase *object_by_cname(_cstr_t cname, _rf_t flags) {
		iBase *r = NULL;

		//...

		return r;
	}

	iBase *object_by_iname(_cstr_t iname, _rf_t flags) {
		iBase *r = NULL;

		//...

		return r;
	}

	iBase *object_by_handle(HOBJECT h, _rf_t flags) {
		iBase *r = NULL;

		//...

		return r;
	}

	HOBJECT handle_by_iname(_cstr_t iname) {
		HOBJECT r = NULL;

		//...

		return r;
	}

	HOBJECT handle_by_cname(_cstr_t cname) {
		HOBJECT r = NULL;

		//...

		return r;
	}

	bool object_info(HOBJECT h, _object_info_t *poi) {
		bool r = false;

		//...

		return r;
	}

	void init_array(_base_entry_t *array, _u32 count) {
		add_base_array(array, count);
		//...
	}

	// notifications
	HNOTIFY monitoring_add(iBase *mon_obj, // monitored object
				_cstr_t mon_iname, // monitored interface
				_cstr_t mon_cname, // monitored object name
				iBase *handler_obj,// notifications receiver
				_u8 scan_flags=0 // scan for already registered objects
				) {
		HNOTIFY r = NULL;

		//...

		return r;
	}

	void monitoring_remove(HNOTIFY) {
		//...
	}

	// extensions
	void extension_dir(_cstr_t dir) {
		m_ext_dir = dir;
	}

	_err_t extension_load(_cstr_t file, _cstr_t alias=0) {
		_err_t r = ERR_UNKNOWN;

		//...

		return r;
	}

	_err_t extension_unload(_cstr_t alias) {
		_err_t r = ERR_UNKNOWN;

		//...

		return r;
	}

	void extension_enum(_enum_ext_t *pcb, void *udata) {
		//...
	}

	void destroy(void) {
		//...
	}
};

static cRepository _g_repository_;
