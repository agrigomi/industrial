#include "private.h"

typedef struct {
	iBase *pi_base;
	_v_pi_object_t v_users;
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
		pbo->v_users.~_v_pi_object_t(); // destroy vector
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
		pbo->v_users.push_back(pi_user);
}

_v_pi_object_t *get_object_users(iBase *pi_object) {
	_v_pi_object_t *r = NULL;
	_u32 sz = 0;
	_object_users_t *pbo = (_object_users_t *)_g_users_map_.get(&pi_object, sizeof(pi_object), &sz);

	if(pbo)
		r = &pbo->v_users;

	return r;
}

void users_enum(iBase *pi_object, _enum_cb_t *pcb, void *udata) {
	_v_pi_object_t *p_vpi = get_object_users(pi_object);

	if(p_vpi) {
		_v_pi_object_t::iterator it = p_vpi->begin();

		while(it != p_vpi->end()) {
			pcb(*it, udata);
			it++;
		}
	}
}

void destroy_users_storage(void) {
	_mutex_handle_t hlock = _g_users_map_.lock();

	_g_users_map_.enm([](void *p, _u32 sz, void *udata)->_s32 {
		_object_users_t *p_users = (_object_users_t *)p;

		p_users->v_users.~_v_pi_object_t();
		return MAP_ENUM_CONTINUE;
	}, NULL, hlock);

	_g_users_map_.unlock(hlock);

	_g_users_map_.destroy();
}

