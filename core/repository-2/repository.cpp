#include <string.h>
#include "private.h"
#include "iTaskMaker.h"

class cRepository: public iRepository {
private:
	_cstr_t	m_ext_dir;
	_v_pi_object_t	mv_pending; // vector for original pending objects
	_v_pi_object_t::iterator mv_it_pending; // iterator for original pending objects
	iTaskMaker *mpi_tasks;

	void enum_pending(_enum_cb_t *pcb, void *udata) {
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

	iTaskMaker *get_task_maker(void) {
		if(!mpi_tasks)
			mpi_tasks = (iTaskMaker *)object_by_iname(I_TASK_MAKER, RF_ORIGINAL);

		return mpi_tasks;
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

	// Process original pending list.
	void process_pending_list(iBase *pi_base=NULL) {
		typedef struct {
			cRepository	*p_repo;
			iBase		*pi_base;
		}_enum_info_t;
		_enum_info_t e = {this, pi_base};

		enum_pending([](iBase *pi_base, void *udata)->_s32 {
			_s32 r = ENUM_CONTINUE;
			_enum_info_t *pe = (_enum_info_t *)udata;

			//...

			return r;
		}, &e);
	}

	void init_base_array(_base_entry_t *p_bentry, _u32 count) {
		for(_u32 i = 0; i < count; i++) {
			_object_info_t oi;

			p_bentry->pi_base->object_info(&oi);

			if(oi.flags & RF_ORIGINAL) {
				//...
			}
		}
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
		init_base_array(array, count);
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
