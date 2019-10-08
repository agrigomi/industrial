#include "private.h"

typedef struct {
	iBase *pi_base;
	_set_pi_object_t s_users;
}_object_users_t;

static _map_t _g_users_map_;

void users_add_object(iBase *pi_object) {
	_u32 sz = 0;

	if(!_g_users_map_.get(&pi_object, sizeof(pi_object), &sz)) {
		_object_users_t bo;

		bo.pi_base = pi_object;
		_g_users_map_.add(&pi_object, sizeof(pi_object), &bo, sizeof(_object_users_t));
	}
}

void users_remove_object(iBase *pi_object) {
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&pi_object, sizeof(pi_object), &sz);

	if(pbo) {
		pbo->s_users.~_set_pi_object_t(); // destroy vector
		_g_users_map_.del(&pi_object, sizeof(pi_object));
	}
}

void users_add_object_user(iBase *pi_object, iBase *pi_user) {
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&pi_object, sizeof(pi_object), &sz);

	if(!pbo) {
		_object_users_t bo;

		bo.pi_base = pi_object;
		pbo = (_object_users_t *)_g_users_map_.add(&pi_object, sizeof(pi_object), &bo, sizeof(_object_users_t));
	}

	if(pbo)
		pbo->s_users.insert(pi_user);
}

_set_pi_object_t *get_object_users(iBase *pi_object) {
	_set_pi_object_t *r = NULL;
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&pi_object, sizeof(pi_object), &sz);

	if(pbo)
		r = &pbo->s_users;

	return r;
}

void users_enum(iBase *pi_object, _enum_cb_t *pcb, void *udata) {
	_set_pi_object_t *p_spi = get_object_users(pi_object);

	if(p_spi) {
		_set_pi_object_t::iterator it = p_spi->begin();

		while(it != p_spi->end()) {
			pcb(*it, udata);
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

