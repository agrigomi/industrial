#include <malloc.h>
#include <assert.h>
#include "private.h"
#include "zone.h"

static _mutex_t _g_zmutex_;

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
	return (zone_init(&_g_zcontext_)) ? true : false;
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
	zone_destroy(&_g_zcontext_);
}
