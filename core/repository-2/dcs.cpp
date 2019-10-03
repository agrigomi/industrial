#include <string.h>
#include "private.h"

#define CREADY		0
#define CPENDING	1

static _list_t	_gl_dcs_;


_mutex_handle_t dcs_lock(_mutex_handle_t hlock) {
	return _gl_dcs_.lock(hlock);
}

void dcs_unlock(_mutex_handle_t hlock) {
	_gl_dcs_.unlock(hlock);
}

iBase *dcs_create_pending_context(_base_entry_t *p_bentry, _rf_t flags, _mutex_handle_t hlock) {
	iBase *r = NULL;
	_object_info_t info;

	p_bentry->pi_base->object_info(&info);

	if((info.flags & flags) & RF_CLONE) {
		_mutex_handle_t hm = dcs_lock(hlock);
		_u32 size = info.size + 1; // one byte more space to keep state
		_cstat_t *p_state = NULL;

		_gl_dcs_.col(CPENDING, hm);
		if((r = (iBase *)_gl_dcs_.add(p_bentry->pi_base, size, hm))) {
			_u8 *p = (_u8 *)r;

			p_state = (_cstat_t *)(p + info.size);
			*p_state = ST_PENDING;
			p_bentry->ref_cnt++;
		}

		dcs_unlock(hm);
	}

	return r;
}

_cstat_t dcs_get_context_state(iBase *pi_base) {
	_cstat_t r = 0;
	_object_info_t info;
	_cstat_t *p_state = NULL;
	_u8 *p = (_u8 *)pi_base;

	pi_base->object_info(&info);
	p_state = (_cstat_t *)(p + info.size);
	r = *p_state;

	return r;
}

void dcs_set_context_state(iBase *pi_base, _cstat_t state) {
	_object_info_t info;
	_cstat_t *p_state = NULL;
	_u8 *p = (_u8 *)pi_base;

	pi_base->object_info(&info);
	p_state = (_cstat_t *)(p + info.size);
	*p_state = state;
}

bool dcs_remove_context(iBase *pi_base, _mutex_handle_t hlock) {
	bool r = false;

	//...

	return r;
}

bool dcs_end_pending(iBase *pi_base, _mutex_handle_t hlock) {
	bool r = false;

	//...

	return r;
}

void dcs_destroy_storage(void) {
	_gl_dcs_.destroy();
}
