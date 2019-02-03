#include "s2.h"

#define RELEASE_BIG_CHUNKS

#define S2_MIN_PAGE	4096
#define DEFAULT_S2_LIMIT	0xffffffffffffffffLLU

static _u64 _allign(_u32 sz, _u32 page_size) {
	_u64 r = (sz < S2_MIN_ALLOC)?S2_MIN_ALLOC:sz;
	if(r < page_size) {
		_u64 m = page_size;
		while(!(m & r))
			m >>= 1;
		_u64 n = m >> 1;
		while(n && !(n & r))
			n >>= 1;
		if(n)
			m <<= 1;

		r = m;
	} else {
		r = sz / page_size;
		r += (sz % page_size)?1:0;
		r *= page_size;
	}
	return r;
}

static _s2_t *s2_entry(_s2_context_t *p_scxt, _u32 sz) {
	_s2_t *r = 0;

	if(p_scxt->p_s2) {
		_u64 _sz = _allign(sz, p_scxt->page_size);
		_u64 b = S2_MIN_ALLOC;
		_u32 pc = p_scxt->page_size / sizeof(_s2_t);
		_u32 p = 0;

		for(; p < pc; p++) {
			if(_sz >= b && _sz < (b<<1)) {
				if((r = p_scxt->p_s2 + p)) {
					if(r->osz == 0) {
						p_scxt->p_mem_set((_u8 *)r, 0, sizeof(_s2_t));
						r->osz = _sz;
						r->count = 0;
					}
				}
				break;
			}
			b <<= 1;
		}
	}
	return r;
}

#define S2_PATTERN	(_u64)0x31caca6bcacacafe
#define S2_PATTERN2	(_u64)0x42dada7cdadadafd

_u8 s2_init(_s2_context_t *p_scxt) {
	_u8 r = 0;

	if(p_scxt->p_s2)
		r = 1;
	else {
		if(p_scxt->p_mem_alloc && p_scxt->page_size >= S2_MIN_PAGE) {
			if((p_scxt->p_s2 = (_s2_t *)p_scxt->p_mem_alloc(1, DEFAULT_S2_LIMIT, p_scxt->p_udata))) {
				p_scxt->p_mem_set((_u8 *)(p_scxt->p_s2), 0, p_scxt->page_size);
				r = 1;
			}
		}
	}
	return r;
}

static _u8 s2_inc_rec_level(_s2_context_t *p_scxt, _s2_t *p_s2) {
	_u8 r = 0;
	/* increase of recursion level */
	_u8 *pnew = (_u8 *)p_scxt->p_mem_alloc(1, DEFAULT_S2_LIMIT, p_scxt->p_udata);
	if(pnew) {
		p_scxt->p_mem_set(pnew, 0, p_scxt->page_size);
		p_scxt->p_mem_cpy(pnew, (_u8 *)p_s2, sizeof(_s2_t));
		_u8 i = 0;
		for(; i < S2_NODES; i++)
			p_s2->ptr[i] = 0;
		p_s2->ptr[0] = (_u64)pnew;
		p_s2->level++;
		p_s2->count = 0;
		r = 1;
	}
	return r;
}

static void *_s2_alloc(_s2_context_t *p_scxt, _s2_t *p_s2, _u32 size, _ulong limit) {
	void *r = 0;
	_u8 i = 0;

	for(; i < S2_NODES; i++) {
_enum_s2_nodes_:
		if(p_s2->ptr[i]) {
			if(p_s2->level) {
				_u32 count = p_scxt->page_size / sizeof(_s2_t);
				_s2_t *ps = (_s2_t *)p_s2->ptr[i];
				_u32 j = 0;
				for(; j < count; j++) {
					if(ps->osz == 0)
						ps->osz = size;
					if(ps->osz == size) {
						if((r = _s2_alloc(p_scxt, ps, size, limit))) {
							i = S2_NODES;
							break;
						}
					}
					ps++;
				}
			} else if((p_s2->ptr[i]+size) < limit) {
				_u32 npages = 1;
				if(p_s2->osz > p_scxt->page_size)
					npages = p_s2->osz / p_scxt->page_size;
				_u32 mem_sz = npages * p_scxt->page_size;
				_u32 nobjs = mem_sz / p_s2->osz;
				_u8 *_ptr = (_u8 *)p_s2->ptr[i];
				_u64 *_rptr = 0;
				_u32 j = 0;
				for(; j < nobjs; j++) {
					_u64 *p64 = (_u64 *)(_ptr + (j * p_s2->osz));
					if(*p64 == S2_PATTERN2 && !_rptr)
						_rptr = p64;
					if(*p64 == S2_PATTERN) {
						r = p64;
						i = S2_NODES;
						*p64 = 0;
						p_s2->count++;
						break;
					}
				}

				if(!r && _rptr) {
					r = _rptr;
					*_rptr = 0;
					p_s2->count++;
					break;
				}
			}
		} else {
			_u32 npages = 1;
			if(p_s2->level == 0 && p_s2->osz > p_scxt->page_size)
				npages = p_s2->osz / p_scxt->page_size;
			_u8 *p = (_u8 *)p_scxt->p_mem_alloc(npages,
					(p_s2->level)?DEFAULT_S2_LIMIT:limit, p_scxt->p_udata);
			if(p) {
				_u32 mem_sz = npages * p_scxt->page_size;
				p_scxt->p_mem_set(p, 0, mem_sz);
				if(p_s2->level == 0) {
					_u32 nobjs = mem_sz / p_s2->osz;
					_u32 j = 0;
					for(; j < nobjs; j++) {
						_u64 *p_obj = (_u64 *)(p + (j * p_s2->osz));
						*p_obj = S2_PATTERN;
					}
				}
				p_s2->ptr[i] = (_u64)p;
				goto _enum_s2_nodes_;
			} else
				i = S2_NODES;
		}
	}
	return r;
}

void *s2_alloc(_s2_context_t *p_scxt, _u32 size, _ulong limit) {
	void *r = 0;
	_s2_t *p_s2 = s2_entry(p_scxt, size);
	if(p_s2) {
		_u64 sz = _allign(size, p_scxt->page_size);
		_s2_hlock_t hlock = p_scxt->p_lock(0, p_scxt->p_udata);
		if(p_s2->level == 0 && sz > p_scxt->page_size && sz != p_s2->osz)
			s2_inc_rec_level(p_scxt, p_s2);
		if(!(r = _s2_alloc(p_scxt, p_s2, sz, limit))) {
			if(s2_inc_rec_level(p_scxt, p_s2))
				r = _s2_alloc(p_scxt, p_s2, sz, limit);
		}
		p_scxt->p_unlock(hlock, p_scxt->p_udata);
	}

	return r;
}

static _bool _s2_verify(_s2_t *p_s2, void *ptr, _u32 page_size, _s2_t **pp_s2, _u64 **p_obj, _u32 *p_idx) {
	_bool r = _false;
	_u8 i = 0;

	for(; i < S2_NODES; i++) {
		if(p_s2->ptr[i]) {
			if(p_s2->level) {
				_u32 count = page_size / sizeof(_s2_t);
				_s2_t *ps = (_s2_t *)p_s2->ptr[i];
				_u32 j = 0;
				for(; j < count; j++) {
					if(ps->count && ps->osz) {
						if((r = _s2_verify(ps, ptr, page_size, pp_s2, p_obj, p_idx))) {
							i = S2_NODES;
							break;
						}
					}
					ps++;
				}
			} else {
				_u32 npages = 1;
				if(p_s2->osz > page_size)
					npages = p_s2->osz / page_size;
				_u32 mem_sz = npages * page_size;
				_u32 nobjs = mem_sz / p_s2->osz;
				_u8 *_ptr = (_u8 *)p_s2->ptr[i];
				_u32 j = 0;
				for(; j < nobjs; j++) {
					_u64 *p64 = (_u64 *)(_ptr + (j * p_s2->osz));
					if(p64 == (_u64 *)ptr) {
						if(*p64 != S2_PATTERN && *p64 != S2_PATTERN2) {
							*p_obj = p64;
							*pp_s2 = p_s2;
							*p_idx = i;
							r = _true;
						}
						i = S2_NODES;
						break;
					}
				}
			}
		}
	}
	return r;
}

void s2_free(_s2_context_t *p_scxt, void *ptr, _u32 size) {
	_s2_t *p_s2 = s2_entry(p_scxt, size);

	if(p_s2) {
		_s2_hlock_t hlock = p_scxt->p_lock(0, p_scxt->p_udata);
		_u64 *p_obj = 0;
		_s2_t *_ps = 0;
		_u32 idx = 0;
		if(_s2_verify(p_s2, ptr, p_scxt->page_size, &_ps, &p_obj, &idx)) {
			_u64 *p = (_u64 *)ptr;
			if(*p == *p_obj) {
				*p_obj = S2_PATTERN2;
#ifdef RELEASE_BIG_CHUNKS
				if(_ps->osz >= p_scxt->page_size) {
					_u32 npages = _ps->osz / p_scxt->page_size;
					npages += (_ps->osz % p_scxt->page_size)?1:0;
					void *_ptr = (void *)_ps->ptr[idx];
					if(ptr == _ptr) {
						p_scxt->p_mem_free(_ptr, npages, p_scxt->p_udata);
						_ps->ptr[idx] = 0;
					}
				}
#endif
				if(_ps->count)
					_ps->count--;
			}
		}
		p_scxt->p_unlock(hlock, p_scxt->p_udata);
	}
}

_bool s2_verify(_s2_context_t *p_scxt, void *ptr, _u32 size) {
	_u8 r = _false;
	_s2_t *p_s2 = s2_entry(p_scxt, size);

	if(p_s2) {
		_s2_hlock_t hlock = p_scxt->p_lock(0, p_scxt->p_udata);
		_u64 *p_obj = 0;
		_s2_t *_ps=0;
		_u32 idx = 0;
		r = _s2_verify(p_s2, ptr, p_scxt->page_size, &_ps, &p_obj, &idx);
		p_scxt->p_unlock(hlock, p_scxt->p_udata);
	}

	return r;
}

static void _s2_status(_s2_t *p_s2, _s2_status_t *p_sst, _u32 page_size) {
	_u8 i = 0;
	for(; i < S2_NODES; i++) {
		if(p_s2->ptr[i]) {
			if(p_s2->level) {
				_u32 count = page_size / sizeof(_s2_t);
				_s2_t *ps = (_s2_t *)p_s2->ptr[i];
				_u32 j = 0;

				p_sst->nspg++;

				for(; j < count; j++) {
					if(ps->osz)
						_s2_status(ps, p_sst, page_size);
					ps++;
				}
			} else {
				_u32 npages = 1;
				if(p_s2->osz > page_size)
					npages = p_s2->osz / page_size;
				p_sst->ndpg += npages;
				_u32 mem_sz = npages * page_size;
				_u32 nobjs = mem_sz / p_s2->osz;
				p_sst->nrobj += nobjs;
				_u8 *ptr = (_u8 *)p_s2->ptr[i];
				_u32 j = 0;
				for(; j < nobjs; j++) {
					_u64 *p64 = (_u64 *)(ptr + (j * p_s2->osz));
					if(*p64 != S2_PATTERN && *p64 != S2_PATTERN2)
						p_sst->naobj++;
				}
			}
		} else
			break;
	}
}

void s2_status(_s2_context_t *p_scxt, _s2_status_t *p_sstatus) {
	p_sstatus->naobj = p_sstatus->nrobj = p_sstatus->ndpg = p_sstatus->nspg = 0;

	if(p_scxt->p_s2) {
		_s2_hlock_t hlock = p_scxt->p_lock(0, p_scxt->p_udata);
		p_sstatus->nspg = 1;
		_u32 count = p_scxt->page_size / sizeof(_s2_t);
		_s2_t *ps = p_scxt->p_s2;
		_u32 i = 0;
		for(; i < count; i++) {
			_s2_status(ps, p_sstatus, p_scxt->page_size);
			ps++;
		}
		p_scxt->p_unlock(hlock, p_scxt->p_udata);
	}
}

static void _destroy_s2(_s2_context_t *p_scxt, _s2_t *p_s2) {
	_u8 i = 0;
	for(; i < S2_NODES; i++) {
		if(p_s2->ptr[i]) {
			if(p_s2->level) {
				_u32 count = p_scxt->page_size / sizeof(_s2_t);
				_s2_t *ps = (_s2_t *)p_s2->ptr[i];
				_u32 j = 0;
				for(; j < count; j++) {
					_destroy_s2(p_scxt, ps);
					ps++;
				}
				p_scxt->p_mem_free((void *)p_s2->ptr[i], 1, p_scxt->p_udata);
			} else {
				_u32 npages = 1;
				if(p_s2->osz > p_scxt->page_size)
					npages = p_s2->osz / p_scxt->page_size;

				p_scxt->p_mem_free((void *)p_s2->ptr[i], npages, p_scxt->p_udata);
			}

			p_s2->ptr[i] = 0;
		}
	}
	p_scxt->p_mem_set((_u8 *)p_s2, 0, sizeof(_s2_t));
}

void s2_destroy(_s2_context_t *p_scxt) {
	if(p_scxt->p_s2) {
		_s2_hlock_t hlock = p_scxt->p_lock(0, p_scxt->p_udata);
		_u32 count = p_scxt->page_size / sizeof(_s2_t);
		_s2_t *ps = p_scxt->p_s2;
		_u32 i = 0;
		for(; i < count; i++) {
			_destroy_s2(p_scxt, ps);
			ps++;
		}
		p_scxt->p_mem_free((void *)p_scxt->p_s2, 1, p_scxt->p_udata);
		p_scxt->p_unlock(hlock, p_scxt->p_udata);
	}
}

