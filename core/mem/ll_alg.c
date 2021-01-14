#include "ll_alg.h"


static _u64 ll_lock(_ll_context_t *p_cxt, _u64 hlock) {
	return (p_cxt->p_lock)?p_cxt->p_lock(hlock, p_cxt->p_udata):0;
}

static void ll_unlock(_ll_context_t *p_cxt, _u64 hlock) {
	if(p_cxt->p_unlock)
		p_cxt->p_unlock(hlock, p_cxt->p_udata);
}

static void _set(_u8 *ptr, _u8 x, _u32 sz) {
	_u32 i = 0;

	for(; i < sz; i++)
		*(ptr + i) = x;
}

_u8 ll_init(_ll_context_t *p_cxt, _u8 mode, _u8 ncol, _ulong addr_limit) {
	_u8 r = 0;

	if(!p_cxt->state) {
		_u64 lock = ll_lock(p_cxt, 0);
		_u32 ssz = ncol * sizeof(_ll_state_t);

		p_cxt->mode = mode;
		p_cxt->ncol = ncol;
		p_cxt->addr_limit = addr_limit;
		p_cxt->ccol = 0;
		if((p_cxt->state = (_ll_state_t *)p_cxt->p_alloc(ssz, addr_limit, p_cxt->p_udata))) {
			_set((_u8 *)p_cxt->state, 0, ssz);
			r = 1;
		}

		ll_unlock(p_cxt, lock);
	}

	return r;
}

void ll_uninit(_ll_context_t *p_cxt) {
	if(p_cxt->state) {
		_u64 lock = ll_lock(p_cxt, 0);
		_u32 i = 0;

		for(; i < p_cxt->ncol; i++) {
			ll_col(p_cxt, i, lock);
			ll_clr(p_cxt, lock);
		}
		p_cxt->p_free(p_cxt->state, p_cxt->ncol * sizeof(_ll_state_t), p_cxt->p_udata);
		p_cxt->state = 0;
		p_cxt->ncol = 0;
		p_cxt->ccol = 0;
		ll_unlock(p_cxt, lock);
	}
}

_u32 ll_cnt(_ll_context_t *p_cxt, _u64 hlock) {
	_u32 r = 0;
	_u64 hl = ll_lock(p_cxt, hlock);
	r = p_cxt->state[p_cxt->ccol].count;
	ll_unlock(p_cxt, hl);
	return r;
}

static void _cpy(_u8 *dst,_u8 *src,_u32 sz) {
	_u32 _sz = sz;
	_u32 i = 0;

	while(_sz) {
		*(dst + i) = *(src + i);
		i++;
		_sz--;
	}
}

_ll_item_hdr_t *_get(_ll_context_t *p_cxt, _u32 index) {
	_u32 i = 0;
	_ll_item_hdr_t *p = 0;

	if(index < p_cxt->state[p_cxt->ccol].count) {
		if((p = p_cxt->state[p_cxt->ccol].p_current)) {
			i = p_cxt->state[p_cxt->ccol].current;

			while(i != index && i < p_cxt->state[p_cxt->ccol].count) {
				if(i > index) { /* move back */
					p = p->prev;
					i--;
					if(!i)
						break;
				}

				if(i < index) {	/* move forward */
					p = p->next;
					i++;
				}
			}

			if(i == index) {
				/* found the true index
				   make this index current */
				p_cxt->state[p_cxt->ccol].current = i;
				p_cxt->state[p_cxt->ccol].p_current = p;
			}
		}
	}

	return p;
}

void *ll_get(_ll_context_t *p_cxt, _u32 index, _u32 *p_size, _u64 hlock) {
	void *r = 0;
	_ll_item_hdr_t *p = 0;
	_u64 hm = ll_lock(p_cxt, hlock);

	p = _get(p_cxt, index);
	if(p) {
		*p_size = p->size;
		r = p + 1;
	}

	ll_unlock(p_cxt, hm);
	return r;
}

// alloc new empty record
void *ll_new(_ll_context_t *p_cxt, _u32 size, _u64 hlock) {
	void *r = 0;
	_u32 sz = size + sizeof(_ll_item_hdr_t);
 	_ll_item_hdr_t *p = 0; /* prev */

	if(p_cxt->p_alloc)
		p = (_ll_item_hdr_t *)p_cxt->p_alloc(sz, p_cxt->addr_limit, p_cxt->p_udata);

	if(p) {
		/* clear record */
		_set((_u8 *)p, 0, sz);

		p->cxt = p_cxt;
		p->size= size;
		p->col = p_cxt->ccol;

		_u64 hm = ll_lock(p_cxt, hlock);

		if(p_cxt->state[p_cxt->ccol].p_last)
			p_cxt->state[p_cxt->ccol].p_last->next = p;

		p->prev = p_cxt->state[p_cxt->ccol].p_last;
		if(p_cxt->mode == LL_MODE_RING)
			p->next = p_cxt->state[p_cxt->ccol].p_first;
		else
			p->next = 0;

		p_cxt->state[p_cxt->ccol].p_last = p_cxt->state[p_cxt->ccol].p_current = p;

		if(!p_cxt->state[p_cxt->ccol].p_first)
			p_cxt->state[p_cxt->ccol].p_first = p;

		/* make the new item as current */
		p_cxt->state[p_cxt->ccol].current = p_cxt->state[p_cxt->ccol].count;
		/* increase items counter */
		p_cxt->state[p_cxt->ccol].count++;

		ll_unlock(p_cxt, hm);

		/* move the pointer to user data area
			(skip the item header) */
		r = (p + 1);
	}

	return r;
}

void *ll_add(_ll_context_t *p_cxt, void *p_data, _u32 size, _u64 hlock) {
	void *r = 0;
 	_ll_item_hdr_t *_p = 0; /* prev */
	_u32 sz = size + sizeof(_ll_item_hdr_t);


	if(p_cxt->p_alloc)
		_p = (_ll_item_hdr_t *)p_cxt->p_alloc(sz, p_cxt->addr_limit, p_cxt->p_udata);

	if(_p) {
		/* clear memory region */
		_u8 *ptr = (_u8 *)_p;
		_u32 i = 0;

		for(; i < sz; i++)
			*(ptr + i) = 0;

		_p->cxt = p_cxt;
		_p->size= size;
		_p->col = p_cxt->ccol;

		_u64 hm = ll_lock(p_cxt, hlock);

		if(p_cxt->state[p_cxt->ccol].p_last)
			p_cxt->state[p_cxt->ccol].p_last->next = _p;

		_p->prev = p_cxt->state[p_cxt->ccol].p_last;
		if(p_cxt->mode == LL_MODE_RING)
			_p->next = p_cxt->state[p_cxt->ccol].p_first;
		else
			_p->next = 0;

		/* copy user data to newly allocated memory */
		_u8 *dst = (_u8 *)_p;
		_u8 *src = (_u8 *)p_data;
		_cpy(dst + sizeof(_ll_item_hdr_t), src, size);
		/******************************************/

		p_cxt->state[p_cxt->ccol].p_last = p_cxt->state[p_cxt->ccol].p_current = _p;

		if(!p_cxt->state[p_cxt->ccol].p_first)
			p_cxt->state[p_cxt->ccol].p_first = _p;

		/* make the new item as current */
		p_cxt->state[p_cxt->ccol].current = p_cxt->state[p_cxt->ccol].count;
		/* increase items counter */
		p_cxt->state[p_cxt->ccol].count++;

		ll_unlock(p_cxt, hm);

		/* move the pointer to user data area
			(skip the item header) */
		r = (_p + 1);
	}

	return r;
}

void *ll_ins(_ll_context_t *p_cxt, _u32 index, void *p_data, _u32 size, _u64 hlock) {
	void *r = 0;
	_ll_item_hdr_t *p_prev = NULL; /* prev */
	_ll_item_hdr_t *p_new = NULL; /* new */
	_ll_item_hdr_t *p_cur = NULL; /* current */
	_u32 sz = size + sizeof(_ll_item_hdr_t);

	if(index < p_cxt->state[p_cxt->ccol].count) {
		if(p_cxt->p_alloc)
			p_new = (_ll_item_hdr_t *)p_cxt->p_alloc(sz, p_cxt->addr_limit, p_cxt->p_udata);

		if(p_new) {
			/* clear memory region */
			_u8 *ptr = (_u8 *)p_new;
			_u32 i = 0;

			for(; i < sz; i++)
				*(ptr + i) = 0;

			_u64 hm = ll_lock(p_cxt, hlock);

			p_cur = _get(p_cxt, index);
			if(p_cur) {
				p_new->cxt = p_cxt;
				p_new->size= size;
				p_new->col = p_cxt->ccol;

				/* initialize the new header */
				p_new->prev = p_new->next = 0;

				/* copy user data to new allocated area */
				_u8 *dst = (_u8 *)p_new;
				_u8 *src = (_u8 *)p_data;
				_cpy(dst + sizeof(_ll_item_hdr_t), src, size);
				/****************************************/

				if(index > 0) {
					p_prev = _get(p_cxt, index - 1);
					if(p_prev)
						p_prev->next = p_new;
				} else
					/* replace the first item */
					p_cxt->state[p_cxt->ccol].p_first = p_new;

				p_new->prev = p_prev;
				p_new->next = p_cur;
				p_cur->prev = p_new;

				p_cxt->state[p_cxt->ccol].p_current = p_new;
				p_cxt->state[p_cxt->ccol].current = index;
				p_cxt->state[p_cxt->ccol].count++;

				/* skip header (return pointer to user data area) */
				r = (p_new + 1);
			} else {
				if(p_cxt->p_free)
					p_cxt->p_free(p_new, sz, p_cxt->p_udata);
			}

			ll_unlock(p_cxt, hm);
		}
	}

	return r;
}

void  ll_rem(_ll_context_t *p_cxt, _u32 index, _u64 hlock) {
	_u64 hm = ll_lock(p_cxt, hlock);

	_ll_item_hdr_t *p_cur = _get(p_cxt, index);
	if(p_cur && p_cxt->p_free) {
		if(p_cur->prev)
			/* releate prev to next */
			p_cur->prev->next = p_cur->next;

		if(p_cur->next) {
			p_cxt->state[p_cxt->ccol].p_current = p_cur->next;

			/* releate next to prev */
			p_cur->next->prev = p_cur->prev;
		}

		if(p_cur == p_cxt->state[p_cxt->ccol].p_first) {
			p_cxt->state[p_cxt->ccol].p_current =
					p_cxt->state[p_cxt->ccol].p_first = p_cur->next;
			if(p_cxt->mode == LL_MODE_RING)
				p_cxt->state[p_cxt->ccol].p_last->next =
					p_cxt->state[p_cxt->ccol].p_current;
		}

		if(p_cur == p_cxt->state[p_cxt->ccol].p_last) {
			p_cxt->state[p_cxt->ccol].p_current =
					p_cxt->state[p_cxt->ccol].p_last = p_cur->prev;
			if(p_cxt->mode == LL_MODE_RING)
				p_cxt->state[p_cxt->ccol].p_current->next =
					p_cxt->state[p_cxt->ccol].p_first;
			p_cxt->state[p_cxt->ccol].current--;
		}

		p_cxt->state[p_cxt->ccol].count--;

		/* release memory */
		_u32 sz = p_cur->size + sizeof(_ll_item_hdr_t);
		p_cxt->p_free(p_cur, sz, p_cxt->p_udata);
	}

	ll_unlock(p_cxt, hm);
}

void  ll_del(_ll_context_t *p_cxt, _u64 hlock) {
	_u64 hm = ll_lock(p_cxt, hlock);

	_ll_item_hdr_t *p_cur = p_cxt->state[p_cxt->ccol].p_current;
	if(p_cur && p_cxt->p_free) {
		_ll_item_hdr_t *p_prev = p_cur->prev, *p_next = p_cur->next;

		if(p_prev)
			/* releate prev to next */
			p_prev->next = p_cur->next;

		if(p_next) {
			/* releate next to prev */
			p_next->prev = p_cur->prev;
			p_cxt->state[p_cxt->ccol].p_current = p_next;
		} else
			p_cxt->state[p_cxt->ccol].p_current = p_prev;

		if(p_cur == p_cxt->state[p_cxt->ccol].p_first) {
			p_cxt->state[p_cxt->ccol].p_current =
					p_cxt->state[p_cxt->ccol].p_first = p_next;
			if(p_cxt->mode == LL_MODE_RING)
				p_cxt->state[p_cxt->ccol].p_last->next =
					p_cxt->state[p_cxt->ccol].p_current;
		}

		if(p_cur == p_cxt->state[p_cxt->ccol].p_last) {
			p_cxt->state[p_cxt->ccol].p_current = p_cxt->state[p_cxt->ccol].p_last = p_prev;
			if(p_cxt->mode == LL_MODE_RING)
				p_cxt->state[p_cxt->ccol].p_current->next = p_cxt->state[p_cxt->ccol].p_first;
			p_cxt->state[p_cxt->ccol].current--;
		}

		p_cur->cxt = 0; /* do not belong to this list any more */
		p_cxt->state[p_cxt->ccol].count--;

		/* release memory */
		_u32 sz = p_cur->size + sizeof(_ll_item_hdr_t);
		p_cxt->p_free(p_cur, sz, p_cxt->p_udata);
	}

	ll_unlock(p_cxt, hm);
}

void  ll_clr(_ll_context_t *p_cxt, _u64 hlock) {
	_ll_item_hdr_t *p = p_cxt->state[p_cxt->ccol].p_first;
	_u32 i = 0;
	_u64 hm = ll_lock(p_cxt, hlock);

	for(; i < p_cxt->state[p_cxt->ccol].count; i++) {
		if(p) {
			_ll_item_hdr_t *_p = p->next;
			_u32 sz = p->size + sizeof(_ll_item_hdr_t);
			p_cxt->p_free(p, sz, p_cxt->p_udata);
			p = _p;
			p_cxt->state[p_cxt->ccol].p_first = p;
		} else
			break;
	}

	p_cxt->state[p_cxt->ccol].p_current =
			p_cxt->state[p_cxt->ccol].p_first =
			p_cxt->state[p_cxt->ccol].p_last = 0;
	p_cxt->state[p_cxt->ccol].count = p_cxt->state[p_cxt->ccol].current = 0;

	ll_unlock(p_cxt, hm);
}

void ll_col(_ll_context_t *p_cxt, _u8 ccol, _u64 hlock) {
	if(ccol < p_cxt->ncol) {
		_u64 hm = ll_lock(p_cxt, hlock);
		p_cxt->ccol = ccol;
		ll_unlock(p_cxt, hm);
	}
}

_u8 ll_sel(_ll_context_t *p_cxt, void *p_data, _u64 hlock) {
	_u8 r = 0;
	_ll_item_hdr_t *p_hdr = (_ll_item_hdr_t *)p_data;
	p_hdr -= 1;
	if(p_hdr->cxt == p_cxt && p_hdr->col == p_cxt->ccol) {
		_u64 hm = ll_lock(p_cxt, hlock);
		p_cxt->state[p_hdr->col].p_current = p_hdr;
		r = 1;
		ll_unlock(p_cxt, hm);
	}
	return r;
}

_u8 ll_mov(_ll_context_t *p_cxt, void *p_data, _u8 col, _u64 hlock) {
	_u8 r = 0;
	_ll_item_hdr_t *p_hdr = (_ll_item_hdr_t *)p_data;
	_u64 hm = ll_lock(p_cxt, hlock);

	p_hdr -= 1;
	if(p_hdr->cxt == p_cxt && col != p_hdr->col && col < p_cxt->ncol) {
		_ll_state_t *p_ds = &p_cxt->state[col]; /* destination column */
		_ll_state_t *p_ss = &p_cxt->state[p_hdr->col]; /* source column */
		/* unlink from source column */
		_ll_item_hdr_t *p_src_prev = p_hdr->prev;
		_ll_item_hdr_t *p_src_next = p_hdr->next;
		if(p_src_prev)
			p_src_prev->next = p_src_next;
		if(p_src_next)
			p_src_next->prev = p_src_prev;
		if(p_ss->p_first == p_hdr)
			p_ss->p_first = p_src_next;
		if(p_ss->p_last == p_hdr)
			p_ss->p_last = p_src_prev;
		if(p_ss->p_current == p_hdr) {
			if(p_src_next)
				p_ss->p_current = p_src_next;
			else if(p_src_prev)
				p_ss->p_current = p_src_prev;
			else
				p_ss->p_current = p_ss->p_first;
		}
		p_ss->count--;
		/* link in destination column */
		if(p_ds->count) {
			if(p_ds->p_last) {
				p_ds->p_last->next = p_hdr;
				p_hdr->prev = p_ds->p_last;
				if(p_cxt->mode == LL_MODE_RING)
					p_hdr->next = p_ds->p_first;
				else
					p_hdr->next = 0;
				p_ds->p_last = p_hdr;
			} else if(p_ds->p_first) {
				p_ds->p_first->prev = p_hdr;
				p_hdr->next = p_ds->p_first;
				if(p_cxt->mode == LL_MODE_RING)
					p_hdr->prev = p_ds->p_last;
				else
					p_hdr->prev = 0;
				p_ds->p_first = p_hdr;
			} else /* !!! panic */
				goto _mov_done_;
		} else {
			p_ds->p_first = p_ds->p_last = p_ds->p_current = p_hdr;
			if(p_cxt->mode == LL_MODE_RING)
				p_hdr->next = p_ds->p_first;
			else
				p_hdr->prev = p_hdr->next = 0;
		}
		p_hdr->col = col;
		p_ds->count++;
		r = 1;
	}
_mov_done_:
	ll_unlock(p_cxt, hm);

	return r;
}

void ll_roll(_ll_context_t *p_cxt, _u64 hlock) {
	if(p_cxt->mode == LL_MODE_RING) {
		_u64 hm = ll_lock(p_cxt, hlock);
		_ll_item_hdr_t *pf = p_cxt->state[p_cxt->ccol].p_first;
		_ll_item_hdr_t *pl = p_cxt->state[p_cxt->ccol].p_last;
		_ll_item_hdr_t *pc = pf->next;
		if(pf != pl) { /* more than one item */
			p_cxt->state[p_cxt->ccol].p_current = p_cxt->state[p_cxt->ccol].p_first = pc;
			pl->next = pf;
			pf->prev = p_cxt->state[p_cxt->ccol].p_last;
			pf->next = pc;
			pc->prev = pl;
			p_cxt->state[p_cxt->ccol].p_last = pf;
		}
		ll_unlock(p_cxt, hm);
	}
}

void *ll_next(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock) {
	void *r = 0;
	_u64 hm = ll_lock(p_cxt, hlock);
	_u8 ccol = p_cxt->ccol;
	_ll_item_hdr_t *p = p_cxt->state[ccol].p_current;
	if(p && p->next) {
		p_cxt->state[ccol].p_current = p->next;
		r = (p_cxt->state[ccol].p_current + 1);
		*p_size = p_cxt->state[ccol].p_current->size;
		p_cxt->state[ccol].current++;
	}
	ll_unlock(p_cxt, hm);
	return r;
}

void *ll_current(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock) {
	void *r =0;
	_u64 hm = ll_lock(p_cxt, hlock);
	_ll_item_hdr_t *p = p_cxt->state[p_cxt->ccol].p_current;
	if(p) {
		r = p+1;
		*p_size = p->size;
	}
	ll_unlock(p_cxt, hm);
	return r;
}

void *ll_first(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock) {
	void *r = 0;
	_u64 hm = ll_lock(p_cxt, hlock);
	_u8 ccol = p_cxt->ccol;
	_ll_item_hdr_t *p = p_cxt->state[ccol].p_first;
	if(p) {
		p_cxt->state[ccol].p_current = p;
		r = p+1;
		*p_size = p->size;
		p_cxt->state[ccol].current = 0;
	}

	ll_unlock(p_cxt, hm);
	return r;
}

void *ll_last(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock) {
	void *r = 0;
 	_u64 hm = ll_lock(p_cxt, hlock);
	_u8 ccol = p_cxt->ccol;
	_ll_item_hdr_t *p = p_cxt->state[ccol].p_last;
	if(p) {
		p_cxt->state[ccol].p_current = p;
		r = p+1;
		*p_size = p->size;
		p_cxt->state[ccol].current = p_cxt->state[ccol].count - 1;
	}

	ll_unlock(p_cxt, hm);
	return r;
}

void *ll_prev(_ll_context_t *p_cxt, _u32 *p_size, _u64 hlock) {
	void *r = 0;
	_u64 hm = ll_lock(p_cxt, hlock);
	_u8 ccol = p_cxt->ccol;
	_ll_item_hdr_t *p = p_cxt->state[ccol].p_current;
	if(p && p->prev) {
		p_cxt->state[ccol].p_current = p->prev;
		r = (p_cxt->state[ccol].p_current + 1);
		*p_size = p_cxt->state[ccol].p_current->size;
		p_cxt->state[ccol].current--;
	}

	ll_unlock(p_cxt, hm);
	return r;
}

