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

			pcb(pi_base, udata);
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

#define PLMR_READY		0
#define PLMR_PENDING		1
#define PLMR_KEEP_PENDING	2
#define PLMR_FAILED		3

	_u32 process_link_map(iBase *pi_base, bool post_init=false) {
		_u32 r = PLMR_READY;
		_cstat_t state = get_context_state(pi_base);
		_u32 count;
		const _link_info_t *pl = pi_base->object_link(&count);

		if(pl) {
			//...
		}

		return r;
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

public:
	BASE(cRepository, "cRepository", RF_ORIGINAL, 2,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		if(cmd == OCTL_UNINIT)
			destroy();
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

	iBase *object_by_handle(HOBJECT, _rf_t flags) {
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
