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

typedef void *_mem_alloc_t(_u32 size);
typedef void _mem_free_t(void *ptr, _u32 size);
typedef void _hash_t(void *data, _u32 sz_data, _u8 *result);

typedef struct {
	_u32 records;
	_u32 collisions;
	_u32 capacity;
	_mem_alloc_t *pf_mem_alloc;
	_mem_free_t *pf_mem_free;
	_hash_t *pf_hash;
	_map_rec_hdr_t **pp_list;
}_map_context_t;

void map_init(_map_context_t *p_mcxt);
void *map_add(_map_context_t *p_mcxt, void *key, _u32 sz_key, void *data, _u32 sz_data);
void *map_get(_map_context_t *p_mcxt, void *key, _u32 sz_key, _u32 *sz_data);
void map_del(_map_context_t *p_mcxt, void *key, _u32 sz_key);
void map_clr(_map_context_t *p_mcxt);
void map_destroy(_map_context_t *p_mcxt);

#endif
