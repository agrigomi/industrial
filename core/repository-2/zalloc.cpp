#include <malloc.h>
#include <assert.h>
#include "private.h"
#include "zone.h"

static _mutex_t _g_zmutex_;
static bool _g_is_init_ = false;

static _mutex_handle_t zlock(_mutex_handle_t hlock, void *udata) {
	return _g_zmutex_.lock(hlock);
}

static void zunlock(_mutex_handle_t hlock, void *udata) {
	_g_zmutex_.unlock(hlock);
}

static void *zpage_alloc(_u32 num, _u64 limit, void *udata) {
	return malloc(num * ZONE_PAGE_SIZE);
}

static void zpage_free(void *ptr, _u32 num, void *udata) {
	free(ptr);
}

static _zone_context_t _g_zcontext_ = {
	zpage_alloc, zpage_free,
	zlock, zunlock,
	NULL, ZONE_DEFAULT_LIMIT, NULL
};

bool zinit(void) {
	bool r = false;

	if(!(r = _g_is_init_))
		r = _g_is_init_ = (!zone_init(&_g_zcontext_)) ? true : false;

	return r;
}

bool is_zinit(void) {
	return _g_is_init_;
}

void *zalloc(_u32 size) {
	return zone_alloc(&_g_zcontext_, size, ZONE_DEFAULT_LIMIT);
}

void zfree(void *ptr, _u32 size) {
#ifndef NDEBUG
	assert(!zone_free(&_g_zcontext_, ptr, size));
#else
	zone_free(&_g_zcontext_, ptr, size);
#endif
}

bool zverify(void *ptr, _u32 size) {
	return (zone_verify(&_g_zcontext_, ptr, size)) ? true : false;
}

void zdestroy(void) {
	if(_g_is_init_) {
		zone_destroy(&_g_zcontext_);
		_g_is_init_ = false;
	}
}
