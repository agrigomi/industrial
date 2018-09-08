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
		p_mcxt->pp_list = p_mcxt->pf_mem_alloc(p_mcxt->capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata);
		p_mcxt->records = p_mcxt->collisions = 0;
	}
}

static _bool add_record(_map_rec_hdr_t *p_rec, _map_rec_hdr_t **pp_map, _u32 capacity, _u32 *collisions) {
	_bool r = _false;
	_u32 idx = hash(p_rec->key, sizeof(p_rec->key)) % capacity;
	_map_rec_hdr_t *p_crec = pp_map[idx];

	if(!p_crec) {
		pp_map[idx] = p_rec;
		r = _true;
	} else {
		if(memcmp(p_crec->key, p_rec->key, HASH_SIZE) == 0)
			return r;

		while(p_crec->next) {
			p_crec = p_crec->next;
			if(memcmp(p_crec->key, p_rec->key, HASH_SIZE) == 0)
				return r;
		}

		/* collision */
		p_crec->next = p_rec;
		*collisions = (*collisions) + 1;
		r = _true;
	}

	return r;
}

static _bool remap(_map_context_t *p_mcxt) {
	_bool r = _false;

	if(p_mcxt->pf_mem_alloc && p_mcxt->pf_mem_free && p_mcxt->capacity) {
		_u32 new_capacity = p_mcxt->capacity * 2;
		_map_rec_hdr_t **pp_new_map = p_mcxt->pf_mem_alloc(new_capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata);
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

			p_mcxt->pf_mem_free(p_mcxt->pp_list, p_mcxt->capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata);
			p_mcxt->pp_list = pp_new_map;
			p_mcxt->capacity = new_capacity;
			p_mcxt->collisions = collisions;
			r = _true;
		}
	}

	return r;
}

static _map_rec_hdr_t *get_record(_map_context_t *p_mcxt,
				void *key,
				_u32 sz_key,
				_u8 *hash_key,
				_u32 *idx,
				_map_rec_hdr_t **pp_prev) {
	_map_rec_hdr_t *r = NULL;

	if(p_mcxt->pf_hash)
		p_mcxt->pf_hash((_u8 *)key, sz_key, hash_key, p_mcxt->udata);
	else
		memcpy(hash_key, key, (sz_key < HASH_SIZE) ? sz_key : HASH_SIZE);

	*idx = hash(hash_key, HASH_SIZE) % p_mcxt->capacity;
	if((r = p_mcxt->pp_list[*idx])) {
		do {
			if(memcmp(r->key, hash_key, HASH_SIZE) == 0)
				break;
			*pp_prev = r;
		} while((r = r->next));
	}

	return r;
}

void *map_get(_map_context_t *p_mcxt, void *key, _u32 sz_key, _u32 *sz_data) {
	void *r = NULL;
	_u8 hash_key[HASH_SIZE]="";
	_map_rec_hdr_t *p_prev = NULL;
	_u32 idx;
	_map_rec_hdr_t *p_rec = get_record(p_mcxt, key, sz_key, hash_key, &idx, &p_prev);

	if(p_rec) {
		*sz_data = p_rec->sz_rec;
		r = (p_rec + 1);
	}

	return r;
}

void *map_add(_map_context_t *p_mcxt, void *key, _u32 sz_key, void *data, _u32 sz_data) {
	void *r = NULL;
	_u8 hash_key[HASH_SIZE]="";
	_map_rec_hdr_t *p_prev = NULL;
	_u32 idx;
	_map_rec_hdr_t *p_rec = get_record(p_mcxt, key, sz_key, hash_key, &idx, &p_prev);

	if(p_rec)
		r = (p_rec + 1);
	else {
		if(p_mcxt->pf_mem_alloc && p_mcxt->pf_mem_free) {
			if((p_rec = p_mcxt->pf_mem_alloc(sizeof(_map_rec_hdr_t) + sz_data, p_mcxt->udata))) {
				r = (p_rec + 1);
				memcpy(r, data, sz_data);
				memcpy(p_rec->key, hash_key, sizeof(p_rec->key));
				p_rec->sz_rec = sz_data;
				p_rec->next = NULL;

				if(add_record(p_rec, p_mcxt->pp_list, p_mcxt->capacity, &p_mcxt->collisions)) {
					p_mcxt->records++;
					if(p_mcxt->records >= p_mcxt->capacity)
						remap(p_mcxt);
				} else {
					p_mcxt->pf_mem_free(p_rec, sizeof(_map_rec_hdr_t) + sz_data, p_mcxt->udata);
					r = NULL;
				}
			}
		}
	}

	return r;
}

void map_del(_map_context_t *p_mcxt, void *key, _u32 sz_key) {
	_u8 hash_key[HASH_SIZE]="";
	_map_rec_hdr_t *p_prev = NULL;
	_u32 idx;
	_map_rec_hdr_t *p_rec = get_record(p_mcxt, key, sz_key, hash_key, &idx, &p_prev);

	if(p_rec) {
		if(p_mcxt->pf_mem_free) {
			if(p_prev)
				p_prev->next = p_rec->next;
			else
				p_mcxt->pp_list[idx] = p_rec->next;
			p_mcxt->pf_mem_free(p_rec, sizeof(_map_rec_hdr_t) + p_rec->sz_rec, p_mcxt->udata);
			p_mcxt->records--;
		}
	}
}

void map_clr(_map_context_t *p_mcxt) {
	_u32 i = 0;

	if(p_mcxt->pf_mem_free) {
		while(i < p_mcxt->capacity) {
			_map_rec_hdr_t *p_rec = p_mcxt->pp_list[i];

			if(p_rec) {
				_map_rec_hdr_t *p_next = NULL;

				do {
					p_next = p_rec->next;
					p_mcxt->pf_mem_free(p_rec, sizeof(_map_rec_hdr_t) + p_rec->sz_rec, p_mcxt->udata);
				} while((p_rec = p_next));

				p_mcxt->pp_list[i] = NULL;
			}

			i++;
		}

		p_mcxt->records = p_mcxt->collisions = 0;
	}
}

void map_destroy(_map_context_t *p_mcxt) {
	map_clr(p_mcxt);
	if(p_mcxt->records == 0) {
		p_mcxt->pf_mem_free(p_mcxt->pp_list, p_mcxt->capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata);
		p_mcxt->pp_list = NULL;
	}
}
