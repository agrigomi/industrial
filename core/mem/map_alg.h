#ifndef __MAP_ALG_H__
#define __MAP_ALG_H__

#include "dtype.h"

#define HASH_SIZE	20

typedef struct map_rec_hdr _map_rec_hdr_t;
struct map_rec_hdr {
	_u32 sz_rec; /* record size */
	_map_rec_hdr_t *next; /* pointer to next record in case of collision */
	_u8 key[HASH_SIZE];
}__attribute__((packed));

typedef void *_mem_alloc_t(_u32 size, void *udata);
typedef void _mem_free_t(void *ptr, _u32 size, void *udata);
typedef void _hash_t(void *data, _u32 sz_data, _u8 *result, void *udata);

typedef struct {
	_u32 records;
	_u32 collisions;
	_u32 capacity;
	_mem_alloc_t *pf_mem_alloc;
	_mem_free_t *pf_mem_free;
	_hash_t *pf_hash;
	_map_rec_hdr_t **pp_list;
	void *udata;
}_map_context_t;

#define MAPENUM	void*

#ifdef __cplusplus
extern "C" {
#endif
/* Initialize hash table context */
_bool map_init(_map_context_t *p_mcxt);
/* Add record to hash table and returns pointer to data */
void *map_add(_map_context_t *p_mcxt, void *key, _u32 sz_key, void *data, _u32 sz_data);
/* Same as map_add, but overwrite record data, if already exists */
void *map_set(_map_context_t *p_mcxt, void *key, _u32 sz_key, void *data, _u32 sz_data);
/* Get record by key */
void *map_get(_map_context_t *p_mcxt, void *key, _u32 sz_key, _u32 *sz_data);
/* delete record by key */
void map_del(_map_context_t *p_mcxt, void *key, _u32 sz_key);
/* clear entire map */
void map_clr(_map_context_t *p_mcxt);
/* Clear entire map and destroy context */
void map_destroy(_map_context_t *p_mcxt);
MAPENUM map_enum_open(_map_context_t *p_mcxt_);
void *map_enum_first(MAPENUM h, _u32 *sz_data);
void *map_enum_next(MAPENUM h, _u32 *sz_data);
void map_enum_del(MAPENUM h);
void map_enum_close(MAPENUM h);

// Advanced enumeration
#define MAP_ENUM_CONTINUE	1
#define MAP_ENUM_BREAK		2
#define MAP_ENUM_DELETE		3

void map_enum(_map_context_t *, _s32 (*)(void *, _u32, void *), void *);

#ifdef __cplusplus
}
#endif
#endif
