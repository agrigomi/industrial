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

_bool map_init(_map_context_t *p_mcxt) {
	_bool r = _false;

	if(p_mcxt->pf_mem_alloc && p_mcxt->capacity && !p_mcxt->pp_list) {
		/* initial allocation of data array */
		if((p_mcxt->pp_list = p_mcxt->pf_mem_alloc(p_mcxt->capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata))) {
			memset(p_mcxt->pp_list, 0, p_mcxt->capacity * sizeof(_map_rec_hdr_t *));
			p_mcxt->records = p_mcxt->collisions = 0;
			r = _true;
		}
	}

	return r;
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
		_u32 new_capacity = (p_mcxt->capacity * 2) + 1;
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

	if(p_mcxt->pp_list && key && sz_key) {
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
		*sz_data = p_rec->sz_rec -1;
		r = (p_rec + 1);
	}

	return r;
}

static _map_rec_hdr_t *alloc_record(_map_context_t *p_mcxt, _u8 *hash_key, void *data, _u32 sz_data) {
	_map_rec_hdr_t *p_rec = NULL;

	if(p_mcxt->pf_mem_alloc && p_mcxt->pf_mem_free) {
		_u32 data_size = sz_data + 1; // the last byte, should be 0 always
		_u32 sz_rec = sizeof(_map_rec_hdr_t) + data_size;
		void *p_data = NULL;

		if((p_rec = p_mcxt->pf_mem_alloc(sz_rec, p_mcxt->udata))) {
			memset(p_rec, 0, sz_rec);
			p_data = (p_rec + 1);
			memcpy(p_data, data, sz_data);
			memcpy(p_rec->key, hash_key, sizeof(p_rec->key));
			p_rec->sz_rec = data_size;
			p_rec->next = NULL;
		}
	}

	return p_rec;
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
		if((p_rec = alloc_record(p_mcxt, hash_key, data, sz_data))) {
			if(add_record(p_rec, p_mcxt->pp_list, p_mcxt->capacity, &p_mcxt->collisions)) {
				p_mcxt->records++;
				if(p_mcxt->records >= p_mcxt->capacity)
					remap(p_mcxt);
				r = (p_rec + 1);
			} else {
				p_mcxt->pf_mem_free(p_rec, sizeof(_map_rec_hdr_t) + sz_data, p_mcxt->udata);
				r = NULL;
			}
		}
	}

	return r;
}

void *map_set(_map_context_t *p_mcxt, void *key, _u32 sz_key, void *data, _u32 sz_data) {
	void *r = NULL;
	_u8 hash_key[HASH_SIZE]="";
	_map_rec_hdr_t *p_rec = NULL;

	map_del(p_mcxt, key, sz_key);

	if(p_mcxt->pf_hash)
		p_mcxt->pf_hash((_u8 *)key, sz_key, hash_key, p_mcxt->udata);
	else
		memcpy(hash_key, key, (sz_key < HASH_SIZE) ? sz_key : HASH_SIZE);

	if((p_rec = alloc_record(p_mcxt, hash_key, data, sz_data))) {
		if(add_record(p_rec, p_mcxt->pp_list, p_mcxt->capacity, &p_mcxt->collisions)) {
			p_mcxt->records++;
			if(p_mcxt->records >= p_mcxt->capacity)
				remap(p_mcxt);
			r = (p_rec + 1);
		} else {
			p_mcxt->pf_mem_free(p_rec, sizeof(_map_rec_hdr_t) + sz_data, p_mcxt->udata);
			r = NULL;
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

	if(p_mcxt->pf_mem_free && p_mcxt->pp_list && p_mcxt->records) {
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
	if(p_mcxt->pp_list) {
		map_clr(p_mcxt);
		if(p_mcxt->records == 0) {
			p_mcxt->pf_mem_free(p_mcxt->pp_list, p_mcxt->capacity * sizeof(_map_rec_hdr_t *), p_mcxt->udata);
			p_mcxt->pp_list = NULL;
		}
	}
}

typedef struct {
	_map_context_t *p_mcxt;
	_u32 aidx; /* array index */
	_u32 uidx; /* user index */
	_map_rec_hdr_t crec; /* copy of current record */
	void *p_data; /* pointer to current record data */
} _map_enum_t;

MAPENUM map_enum_open(_map_context_t *p_mcxt) {
	MAPENUM r = NULL;

	if(p_mcxt->pf_mem_alloc) {
		_map_enum_t *pe = (_map_enum_t *)p_mcxt->pf_mem_alloc(sizeof(_map_enum_t), p_mcxt->udata);
		if(pe) {
			pe->p_mcxt = p_mcxt;
			pe->aidx = pe->uidx = 0;
			memset(&pe->crec, 0, sizeof(_map_rec_hdr_t));
			pe->p_data = NULL;
			r = pe;
		}
	}

	return r;
}

void *map_enum_first(MAPENUM h, _u32 *sz_data) {
	void *r = NULL;
	_map_enum_t *pe = (_map_enum_t *)h;
	_map_rec_hdr_t *p_rec = NULL;

	if(pe && pe->p_mcxt) {
		if(pe->p_mcxt->records && pe->p_mcxt->pp_list) {
			pe->aidx = pe->uidx = 0;
			while(pe->aidx < pe->p_mcxt->capacity) {
				if((p_rec = pe->p_mcxt->pp_list[pe->aidx]))
					break;
				pe->aidx++;
			}

			if(p_rec) {
				memcpy(&pe->crec, p_rec, sizeof(_map_rec_hdr_t));
				r = pe->p_data = (p_rec + 1);
				*sz_data = p_rec->sz_rec -1;
			}
		}
	}

	return r;
}

void *map_enum_next(MAPENUM h, _u32 *sz_data) {
	void *r = NULL;
	_map_enum_t *pe = (_map_enum_t *)h;
	_map_rec_hdr_t *p_rec = NULL;

	if(pe && pe->p_mcxt && pe->p_data && pe->p_mcxt->pp_list) {
		if(pe->crec.next) {
			r = pe->p_data = (pe->crec.next + 1);
			memcpy(&pe->crec, pe->crec.next, sizeof(_map_rec_hdr_t));
			*sz_data = pe->crec.sz_rec -1;
			pe->uidx++;
		} else {
			while(p_rec == NULL && pe->aidx < pe->p_mcxt->capacity-1) {
				pe->aidx++;
				p_rec = pe->p_mcxt->pp_list[pe->aidx];
			}

			if(p_rec) {
				memcpy(&pe->crec, p_rec, sizeof(_map_rec_hdr_t));
				r = pe->p_data = (p_rec + 1);
				*sz_data = p_rec->sz_rec -1;
				pe->uidx++;
			} else {
				memset(&pe->crec, 0, sizeof(_map_rec_hdr_t));
				pe->p_data = NULL;
			}
		}
	}

	return r;
}

void map_enum_del(MAPENUM h) {
	_map_enum_t *pe = (_map_enum_t *)h;
	_map_rec_hdr_t *p_prev = NULL;
	_map_rec_hdr_t *p_rec = NULL;

	if(pe && pe->p_mcxt && pe->p_mcxt->pp_list) {
		if((p_rec = pe->p_mcxt->pp_list[pe->aidx])) {
			while(p_rec && (p_rec + 1) != pe->p_data) {
				p_prev = p_rec;
				p_rec = p_rec->next;
			}

			if(p_rec && (p_rec + 1) == pe->p_data && pe->p_mcxt->pf_mem_free) {
				if(p_prev)
					p_prev->next = p_rec->next;
				else
					pe->p_mcxt->pp_list[pe->aidx] = p_rec->next;

				if(p_rec->next) {
					memcpy(&pe->crec, p_rec->next, sizeof(_map_rec_hdr_t));
					pe->p_data = p_rec->next + 1;
				} else {
					pe->p_data = NULL;
					memset(&pe->crec, 0, sizeof(_map_rec_hdr_t));
				}
				pe->p_mcxt->pf_mem_free(p_rec, p_rec->sz_rec + sizeof(_map_rec_hdr_t), pe->p_mcxt->udata);
				pe->p_mcxt->records--;
			}
		}
	}
}

void map_enum_close(MAPENUM h) {
	_map_enum_t *pe = (_map_enum_t *)h;

	if(pe && pe->p_mcxt) {
		if(pe->p_mcxt->pf_mem_free)
			pe->p_mcxt->pf_mem_free(pe, sizeof(_map_enum_t), pe->p_mcxt->udata);
	}
}

void map_enum(_map_context_t *p_mcxt, _s32 (*pcb)(void *, _u32, void *), void *udata) {
	_map_enum_t me;
	void *data = NULL;
	_u32 size = 0;
	_s32 op = 0;

	memset(&me, 0, sizeof(_map_enum_t));
	me.p_mcxt = p_mcxt;

	data = map_enum_first(&me, &size);

	while(data) {
		op = pcb(data, size, udata);

		if(op == MAP_ENUM_BREAK)
			break;
		else if(op == MAP_ENUM_DELETE)
			map_enum_del(&me);

		data = map_enum_next(&me, &size);
	}
}

