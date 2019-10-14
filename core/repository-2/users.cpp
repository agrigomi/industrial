#include "private.h"

typedef struct {
	_base_entry_t *p_bentry;
	_set_pi_object_t s_users;
}_object_users_t;

static _map_t _g_users_map_;

void users_add_object(_base_entry_t *p_bentry) {
	_u32 sz = 0;

	if(!_g_users_map_.get(&p_bentry, sizeof(p_bentry), &sz)) {
		_object_users_t bo;

		bo.p_bentry = p_bentry;
		_g_users_map_.add(&p_bentry, sizeof(p_bentry), &bo, sizeof(_object_users_t));
	}
}

void users_remove_object(_base_entry_t *p_bentry) {
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&p_bentry, sizeof(p_bentry), &sz);

	if(pbo) {
		pbo->s_users.~_set_pi_object_t(); // destroy vector
		_g_users_map_.del(&p_bentry, sizeof(p_bentry));
	}
}

void users_add_object_user(_base_entry_t *p_bentry, iBase *pi_user) {
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&p_bentry, sizeof(p_bentry), &sz);

	if(!pbo) {
		_object_users_t bo;

		bo.p_bentry = p_bentry;
		pbo = (_object_users_t *)_g_users_map_.add(&p_bentry, sizeof(p_bentry), &bo, sizeof(_object_users_t));
	}

	if(pbo)
		pbo->s_users.insert(pi_user);
}

void users_remove_object_user(_base_entry_t *p_bentry, iBase *pi_user) {
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&p_bentry, sizeof(p_bentry), &sz);

	if(pbo)
		pbo->s_users.erase(pi_user);
}

_set_pi_object_t *get_object_users(_base_entry_t *p_bentry) {
	_set_pi_object_t *r = NULL;
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&p_bentry, sizeof(p_bentry), &sz);

	if(pbo)
		r = &pbo->s_users;

	return r;
}

void users_enum(_base_entry_t *p_bentry, _enum_cb_t *pcb, void *udata) {
	_set_pi_object_t *p_spi = get_object_users(p_bentry);

	if(p_spi) {
		_set_pi_object_t::iterator it = p_spi->begin();

		while(it != p_spi->end()) {
			_s32 x = pcb(*it, udata);

			if(x == ENUM_DELETE)
				p_spi->erase(it);
			else if(x == ENUM_BREAK)
				break;
			else if(x == ENUM_CURRENT)
				continue;

			it++;
		}
	}
}

void destroy_users_storage(void) {
	_mutex_handle_t hlock = _g_users_map_.lock();

	_g_users_map_.enm([](void *p, _u32 sz, void *udata)->_s32 {
		_object_users_t *p_users = (_object_users_t *)p;

		p_users->s_users.~_set_pi_object_t();
		return MAP_ENUM_CONTINUE;
	}, NULL, hlock);

	_g_users_map_.unlock(hlock);

	_g_users_map_.destroy();
}

