#include <string.h>
#include "map_alg.h"

/* make long integer from string */
static _ulong hash(_u8 *key, _u32 sz) {
	_ulong hash = 5381;
	_u32 c, n=0;

	while(n < sz) {
		c = key[n];
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		n++;
	}

	return hash;
}

void map_init(_map_context_t *p_mcxt) {
	if(p_mcxt->pf_mem_alloc && p_mcxt->capacity && !p_mcxt->pp_list) {
		/* initial allocation of data array */
		p_mcxt->pp_list = p_mcxt->pf_mem_alloc(p_mcxt->capacity * sizeof(_map_rec_hdr_t *));
		p_mcxt->records = p_mcxt->collisions = 0;
	}
}

static void add_record(_map_rec_hdr_t *p_rec, _map_rec_hdr_t **pp_map, _u32 capacity, _u32 *collisions) {
	_u32 idx = hash(p_rec->key, sizeof(p_rec->key)) % capacity;

	/*...*/
}

static _bool remap(_map_context_t *p_mcxt) {
	_bool r = _false;

	if(p_mcxt->pf_mem_alloc && p_mcxt->pf_mem_free && p_mcxt->capacity) {
		_u32 new_capacity = p_mcxt->capacity * 2;
		_map_rec_hdr_t **pp_new_map = p_mcxt->pf_mem_alloc(new_capacity * sizeof(_map_rec_hdr_t *));
		_u32 collisions = 0;

		if(pp_new_map) {
			_u32 i = 0;
			_map_rec_hdr_t *p_rec = NULL;

			memset(pp_new_map, 0, new_capacity * sizeof(_map_rec_hdr_t *));
			while(i < p_mcxt->capacity) {
				if((p_rec = p_mcxt->pp_list[i])) {
					_map_rec_hdr_t *p_next = NULL;

					do {
						p_next = p_rec->next;
						p_rec->next = NULL;
						add_record(p_rec, pp_new_map, new_capacity, &collisions);
					} while((p_rec = p_next));
				}

				i++;
			}

			p_mcxt->pf_mem_free(p_mcxt->pp_list, p_mcxt->capacity * sizeof(_map_rec_hdr_t *));
			p_mcxt->pp_list = pp_new_map;
			p_mcxt->capacity = new_capacity;
			p_mcxt->collisions = collisions;
			r = _true;
		}
	}

	return r;
}
