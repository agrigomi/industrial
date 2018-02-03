/* ring buffer algorithm implementation */

#include "rb_alg.h"

typedef struct {
	_u16 size;	/* message size */
	_u8  data[];	/* message data */
}_rb_msg_t;

static _u64 rb_lock(_rb_context_t *pcxt, _u64 hlock) {
	_u64 r = 0;

	if(pcxt->lock)
		r = pcxt->lock(hlock, pcxt->rb_udata);
	return r;
}

static void rb_unlock(_rb_context_t *pcxt, _u64 hlock) {
	if(pcxt->unlock)
		pcxt->unlock(hlock, pcxt->rb_udata);
}

void rb_init(_rb_context_t *pcxt, _u32 capacity, void *udata) {
	if(pcxt->mem_alloc && !pcxt->rb_addr) {
		if((pcxt->rb_addr = pcxt->mem_alloc(capacity, udata))) {
			pcxt->rb_size = capacity;
			pcxt->rb_first = pcxt->rb_last = pcxt->rb_pull = 0;
			pcxt->rb_udata = udata;
			pcxt->mem_set(pcxt->rb_addr, 0, capacity, udata);
		}
	}
}

void  rb_destroy(_rb_context_t *pcxt) {
	if(pcxt->mem_free && pcxt->rb_addr) {
		_u64 hlock = rb_lock(pcxt, 0);
		pcxt->mem_free(pcxt->rb_addr, pcxt->rb_size, pcxt->rb_udata);
		pcxt->rb_addr = NULL;
		pcxt->rb_size = 0;
		rb_unlock(pcxt, hlock);
	}
}

enum {
	P_FIRST = 1,
	P_LAST,
	P_PULL
};

static void skip(_rb_context_t *pcxt, _u8 c) {
	_rb_msg_t *p = NULL;

	switch(c) {
		case P_FIRST:
			p = pcxt->rb_addr + pcxt->rb_first;
			if((pcxt->rb_first + p->size) < pcxt->rb_size)
				pcxt->rb_first += p->size;
			else
				pcxt->rb_first = 0;
			break;
		case P_LAST:
			p = pcxt->rb_addr + pcxt->rb_last;
			if((pcxt->rb_last + p->size) < pcxt->rb_size)
				pcxt->rb_last += p->size;
			else
				pcxt->rb_last = 0;
			break;
		case P_PULL:
			if(pcxt->rb_pull != INVALID_PULL_POSITION) {
				p = pcxt->rb_addr + pcxt->rb_pull;
				_rb_msg_t *p_last = pcxt->rb_addr+pcxt->rb_last;
				if(pcxt->rb_pull != pcxt->rb_last + p_last->size) {
					if((pcxt->rb_pull + p->size) < pcxt->rb_size)
						pcxt->rb_pull += p->size;
					else
						pcxt->rb_pull = 0;
				}
			}
			break;
	}
}

void rb_push(_rb_context_t *pcxt, void *data, _u16 sz) {
	_u64 hlock = rb_lock(pcxt, 0);

	if(pcxt->rb_addr) {
		_rb_msg_t *p_last = pcxt->rb_addr + pcxt->rb_last;
		_u32 npos = pcxt->rb_last + p_last->size;
		_rb_msg_t *p_new = pcxt->rb_addr + npos;
		_u16 nsz = sz + sizeof(_u16);

		if(pcxt->rb_first <= pcxt->rb_last && (pcxt->rb_last + p_last->size + nsz) <= pcxt->rb_size) {
			/* |f|_______|s|___|s|_____|l|_____----------------|	*/ 
			/*                                 |n|_______------| 	*/
			;
			/* |f|_______|s|___|s|_____|s|_____|l|_______------| 	*/
		}
		if(pcxt->rb_first < pcxt->rb_last && (pcxt->rb_last + p_last->size + nsz) > pcxt->rb_size) {
			/* |f|_______|s|___|s|_____|s|_____|l|_______------|	*/ 
			/*                                           |n|_______|*/
			/* |f|_______|s|___|s|_____|s|_____|l|_____________| 	*/
			p_last->size += pcxt->rb_size - pcxt->rb_last;
			/* |l|_______|f|___|s|_____|s|_____|s|_____________| 	*/
			skip(pcxt, P_FIRST);
			skip(pcxt, P_LAST);
			p_last = pcxt->rb_addr + pcxt->rb_last;
			npos = pcxt->rb_last;
			p_new = pcxt->rb_addr + npos;
		}
		if(pcxt->rb_first > pcxt->rb_last && (pcxt->rb_last + p_last->size + nsz) >= pcxt->rb_first) {
			/* |l|_______|f|___|s|_____|s|_____|s|_____________| 	*/
			/*           |n|_______|				*/
			/* |l|_______|s|___|f|_____|s|_____|s|_____________| 	*/
			/* |l|_______|s|___|s|_____|f|_____|s|_____________| 	*/
			while(pcxt->rb_first && (pcxt->rb_last + p_last->size + nsz) >= pcxt->rb_first)
				skip(pcxt, P_FIRST);
		}
		if(pcxt->rb_first > pcxt->rb_last && (pcxt->rb_last + p_last->size + nsz) < pcxt->rb_first) {
			/* |l|_______|s|___|s|_____|f|_____|s|_____________| 	*/
			/*           |n|_______|				*/
			/* |s|_______|l|_______|---|f|_____|s|_____________|	*/
			;
		}

		p_new->size = nsz;
		pcxt->mem_cpy(p_new->data, data, sz, pcxt->rb_udata);
		if(p_new != p_last)
			skip(pcxt, P_LAST);
		if(pcxt->rb_pull == INVALID_PULL_POSITION)
			pcxt->rb_pull = pcxt->rb_last;
	}

	rb_unlock(pcxt, hlock);
}

void *rb_pull(_rb_context_t *pcxt, _u16 *psz) {
	void * r = NULL;
	_u64 hlock = rb_lock(pcxt, 0);
	
	if(pcxt->rb_addr) {
		if(pcxt->rb_pull != INVALID_PULL_POSITION) {
			_rb_msg_t *pp = pcxt->rb_addr + pcxt->rb_pull;
			r = pp->data;
			*psz = pp->size;
			_u32 cp = pcxt->rb_pull;
			skip(pcxt, P_PULL);
			if(cp == pcxt->rb_last)
				/* end of the pull buffer */
				pcxt->rb_pull = INVALID_PULL_POSITION;
		}
	}

	rb_unlock(pcxt, hlock);
	return r;
}

void rb_reset_pull(_rb_context_t *pcxt) {
	_u64 hlock = rb_lock(pcxt, 0);
	pcxt->rb_pull = pcxt->rb_first;
	rb_unlock(pcxt, hlock);
}

