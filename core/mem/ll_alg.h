#ifndef __LL_ALG_H__
#define __LL_ALG_H__

#include "dtype.h"

#define LL_MODE_VECTOR	1
#define LL_MODE_RING	2

typedef void *_ll_alloc_t(_u32 size, _ulong limit, void *p_udata);
typedef void _ll_free_t(void *ptr, _u32 size, void *p_udata);
typedef _u64 _ll_lock_t(_u64 hlock, void *p_udata);
typedef void _ll_unlock_t(_u64 hlock, void *p_udata);

typedef struct ll_item_hdr _ll_item_hdr_t;
struct ll_item_hdr {
	void		*cxt; /* pointer to _ll_context_t */
	_u32		size; /* data size */
	_u8		col;
	_ll_item_hdr_t *prev;	/* prev. item */
	_ll_item_hdr_t *next;	/* next item */
}__attribute__((packed));

typedef struct {
	_ll_item_hdr_t	*p_current;
	_ll_item_hdr_t	*p_first;
	_ll_item_hdr_t	*p_last;
	_u32		count;
	_u32		current;
}_ll_state_t;

typedef struct {
	_u8		mode; /* algorithm mode */
	_ll_alloc_t	*p_alloc;
	_ll_free_t	*p_free;
	_ll_lock_t	*p_lock;
	_ll_unlock_t	*p_unlock;
	void		*p_udata;
	_ulong		addr_limit;
	_u8		ccol;
	_u8 		ncol;
	_ll_state_t	*state;
}_ll_context_t;

#ifdef __cplusplus
extern "C" {
#endif
void ll_init(_ll_context_t *p_cxt, _u8 mode, _u8 ncol, _ulong addr_limit);
void ll_uninit(_ll_context_t *p_cxt);

/* get record by index */
void *ll_get(_ll_context_t *p_cxt, _u32 index, _u32 *p_size, _u64 hlock);
/* add new record at end of list */
void *ll_add(_ll_context_t *p_cxt, void *p_data, _u32 size, _u64 hlock);
/* insert record */
void *ll_ins(_ll_context_t *p_cxt, _u32 index, void *p_data, _u32 size, _u64 hlock);
/* remove record by index */
void  ll_rem(_ll_context_t *p_cxt, _u32 index, _u64 hlock);
/* delete current record */
void  ll_del(_ll_context_t *p_cxt, _u64 hlock);
/* remove all records */
void  ll_clr(_ll_context_t *p_cxt, _u64 hlock);
/* number of records */
_u32  ll_cnt(_ll_context_t *p_cxt, _u64 hlock);
/* select column */
void  ll_col(_ll_context_t *p_cxt, _u8 col, _u64 hlock);
/* select 'p_data' sa current record (if it's a part of storage, return 1) */
_u8  ll_sel(_ll_context_t *p_cxt, void *p_data, _u64 hlock);
/* move between columns */
_u8  ll_mov(_ll_context_t *p_cxt, void *p_data, _u8 col, _u64 hlock);
/* get next (from current) record */
void *ll_next(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock);
/* pointer to current record */
void *ll_current(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock);
/* get pointer to first record */
void *ll_first(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock);
/* get pointer to last record */
void *ll_last(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock);
/* get pointer to prev. (from current) record */
void *ll_prev(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock);
/* ring mode specific (last-->first-->seccond ...) */
void  ll_roll(_ll_context_t *p_cxt, _u64 hlock);
#ifdef __cplusplus
}
#endif
#endif

