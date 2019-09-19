#include "private.h"

hash_map::hash_map() {
	m_context.records = m_context.collisions = 0;
	m_context.capacity = 127;

	m_context.pf_mem_alloc = [](_u32 size, void *udata)->void* {
		return zalloc(size);
	};

	m_context.pf_mem_free = [](void *ptr, _u32 size, void *udata) {
		zfree(ptr, size);
	};

	m_context.pf_hash = [](void *data, _u32 sz_data, _u8 *result, void *udata) {
		_map_t *p_obj = (_map_t *)udata;

		SHA1Reset(&p_obj->m_sha1);
		SHA1Input(&p_obj->m_sha1, (_u8 *)data, sz_data);
		SHA1Result(&p_obj->m_sha1, result);
	};

	m_context.pp_list = NULL;
	m_context.udata = this;

	zinit();
	map_init(&m_context);
}

hash_map::~hash_map() {
	_mutex_handle_t hm = m_mutex.lock();

	map_destroy(&m_context);
	m_mutex.unlock(hm);
}

_mutex_handle_t hash_map::lock(_mutex_handle_t hlock) {
	return m_mutex.lock(hlock);
}

void hash_map::unlock(_mutex_handle_t hlock) {
	m_mutex.unlock(hlock);
}

void *hash_map::add(void *key, _u32 sz_key, void *data, _u32 sz_data, _mutex_handle_t hlock) {
	void *r = NULL;
	_mutex_handle_t hm = m_mutex.lock(hlock);

	r = map_add(&m_context, key, sz_key, data, sz_data);
	m_mutex.unlock(hm);

	return r;
}

void *hash_map::set(void *key, _u32 sz_key, void *data, _u32 sz_data, _mutex_handle_t hlock) {
	void *r = NULL;
	_mutex_handle_t hm = m_mutex.lock(hlock);

	r = map_set(&m_context, key, sz_key, data, sz_data);
	m_mutex.unlock(hm);

	return r;
}

void *hash_map::get(void *key, _u32 sz_key, _u32 *sz_data, _mutex_handle_t hlock) {
	void *r = NULL;
	_mutex_handle_t hm = m_mutex.lock(hlock);

	r = map_get(&m_context, key, sz_key, sz_data);
	m_mutex.unlock(hm);

	return r;
}

void  hash_map::del(void *key, _u32 sz_key, _mutex_handle_t hlock) {
	_mutex_handle_t hm = m_mutex.lock(hlock);

	map_del(&m_context, key, sz_key);
	m_mutex.unlock(hm);
}

void  hash_map::clr(void) {
	_mutex_handle_t hm = m_mutex.lock();

	map_clr(&m_context);
	m_mutex.unlock(hm);
}

void  hash_map::enm(_s32 (*cb)(void *, _u32, void *), void *udata, _mutex_handle_t hlock) {
	_mutex_handle_t hm = m_mutex.lock(hlock);

	map_enum(&m_context, cb, udata);
	m_mutex.unlock(hm);
}
