/* ring buffer algorithm header */

#ifndef __RB_ALG_H__
#define __RB_ALG_H__

#include "dtype.h"

#define INVALID_PULL_POSITION	0xffffffff

typedef struct {
	void *(*mem_alloc)(_u32 sz, void *udata);
	void (*mem_free)(void *ptr, _u32 sz, void *udata);
	void (*mem_set)(void *ptr, _u8 pattern, _u32 size, void *udata);
	void (*mem_cpy)(void *dst, void *src, _u32 size, void *udata);
	_u64 (*lock)(_u64 hlock, void *udata);
	void (*unlock)(_u64 hlock, void *udata);

	void *rb_addr;	/* address of ring buffer */
	_u32 rb_size;	/* size of ring buffer in bytes */
	_u32 rb_first;	/* base position (points first) */
	_u32 rb_last;	/* end position (points last) */
	_u32 rb_pull;	/* position of current pull or INVALID_PULL_POSITION */
	void *rb_udata;
}_rb_context_t;

#ifdef __cplusplus
extern "C" {
#endif
void  rb_init(_rb_context_t *pcxt, _u32 capacity, void *udata);
void  rb_destroy(_rb_context_t *pcxt);
void  rb_push(_rb_context_t *pcxt, void *data, _u16 sz);
void *rb_pull(_rb_context_t *pcxt, _u16 *psz);
void  rb_reset_pull(_rb_context_t *pcxt);
#ifdef __cplusplus
}
#endif
#endif

