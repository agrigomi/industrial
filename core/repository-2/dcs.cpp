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

	//...

	return r;
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
}
