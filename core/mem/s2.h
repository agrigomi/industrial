#ifndef __S2_H__
#define __S2_H__

#include "dtype.h"

#define S2_NODES	7
#define S2_MIN_ALLOC	16 /* min. amount of bytes */

typedef _u64	_s2_hlock_t;

typedef void *_mem_alloc_t(_u32 npages, _ulong limit, void *p_udata);
typedef void _mem_free_t(void *ptr, _u32 npages, void *p_udata);
typedef void _mem_cpy_t(void *dst, void *src, _ulong sz);
typedef void _mem_set_t(void *ptr, _u8 pattern, _ulong sz);
typedef _s2_hlock_t _lock_t(_s2_hlock_t hlock, void *p_udata);
typedef void _unlock_t(_s2_hlock_t hlock, void *p_udata);

typedef struct {
	_u32	osz;		/* object size */
	_u64	ptr[S2_NODES];/* pointer array */
	_u32	level	:4;	/* recursion level*/
	_u32	count	:28;	/* objects count */
}__attribute__((packed)) _s2_t;

typedef struct {
	_u32	naobj;	/* number of active objects */
	_u32	nrobj;	/* number of reserved objects */
	_u32	ndpg;	/* number of data pages */
	_u32	nspg;	/* number of s2 pages */
}_s2_status_t;

typedef struct {
	_u32		page_size;
	_mem_alloc_t	*p_mem_alloc;
	_mem_free_t	*p_mem_free;
	_mem_cpy_t	*p_mem_cpy;
	_mem_set_t	*p_mem_set;
	_lock_t		*p_lock;
	_unlock_t	*p_unlock;
	void		*p_udata;
	_s2_t		*p_s2;
}_s2_context_t;

#ifdef __cplusplus
extern "C" {
#endif
_u8 s2_init(_s2_context_t *p_scxt);
void *s2_alloc(_s2_context_t *p_scxt, _u32 size, _ulong limit);
void s2_free(_s2_context_t *p_scxt, void *ptr, _u32 size);
_bool s2_verify(_s2_context_t *p_scxt, void *ptr, _u32 size);
void s2_status(_s2_context_t *p_scxt, _s2_status_t *p_sstatus);
void s2_destroy(_s2_context_t *p_scxt);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

