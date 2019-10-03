#include <string.h>
#include "private.h"

linked_list::linked_list() {
	zinit();

	memset(&m_context, 0, sizeof(_ll_context_t));
	m_context.p_alloc = [](_u32 size, _ulong limit, void *p_udata)->void* {
		return zalloc(size);
	};
	m_context.p_free = [](void *ptr, _u32 size, void *p_udata) {
		zfree(ptr, size);
	};
	m_context.p_lock = [](_u64 hlock, void *p_udata)->_u64 {
		linked_list *pthis = (linked_list *)p_udata;
		return pthis->lock(hlock);
	};
	m_context.p_unlock = [](_u64 hlock, void *p_udata) {
		linked_list *pthis = (linked_list *)p_udata;
		pthis->unlock(hlock);
	};
	m_context.p_udata = this;
	ll_init(&m_context, LL_MODE_VECTOR, 2, 0xffffffffffffffffLLU);
}

linked_list::~linked_list() {
	ll_uninit(&m_context);
}

_mutex_handle_t linked_list::lock(_mutex_handle_t hlock) {
	return m_mutex.lock(hlock);
}

void linked_list::unlock(_mutex_handle_t hlock) {
	m_mutex.unlock(hlock);
}

void *linked_list::add(void *ptr, _u32 size, _mutex_handle_t hlock) {
	return ll_add(&m_context, ptr, size, hlock);
}

void linked_list::del(_mutex_handle_t hlock) {
	ll_del(&m_context, hlock);
}

void linked_list::col(_u8 col, _mutex_handle_t hlock) {
	ll_col(&m_context, col, hlock);
}

void linked_list::clr(_mutex_handle_t hlock) {
	ll_clr(&m_context, hlock);
}

_u32 linked_list::cnt(_mutex_handle_t hlock) {
	return ll_cnt(&m_context, hlock);
}

bool linked_list::sel(void *rec, _mutex_handle_t hlock) {
	return (ll_sel(&m_context, rec, hlock)) ? true : false;
}

bool linked_list::mov(void *rec, _u8 col, _mutex_handle_t hlock) {
	return (ll_mov(&m_context, rec, col, hlock)) ? true : false;
}

void *linked_list::first(_u32 *p_size, _mutex_handle_t hlock) {
	return ll_first(&m_context, p_size, hlock);
}

void *linked_list::next(_u32 *p_size, _mutex_handle_t hlock) {
	return ll_next(&m_context, p_size, hlock);
}

void *linked_list::current(_u32 *p_size, _mutex_handle_t hlock) {
	return ll_current(&m_context, p_size, hlock);
}

void linked_list::destroy(void) {
	ll_uninit(&m_context);
}
