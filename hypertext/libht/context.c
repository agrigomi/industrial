#include <string.h>
#include "context.h"

/* allocate memory for hypertext context */
_ht_context_t *ht_create_context(_mem_alloc_t *p_alloc, _mem_free_t *p_free) {
	_ht_context_t *r = NULL;

	if(p_alloc) {
		if((r = p_alloc(sizeof(_ht_context_t)))) {
			memset(r, 0, sizeof(_ht_context_t));
			r->pf_mem_alloc = p_alloc;
			r->pf_mem_free = p_free;
		}
	}

	return r;
}

/* initialize context with content and size of hypertext */
void ht_init_context(_ht_context_t *p_htc, void *p_content, unsigned long sz_content) {
	p_htc->ht_content.p_content = p_content;
	p_htc->ht_content.sz_content = sz_content;

	/* try to detect encoding by BOM */
	/*...*/
}

/* destroy (deallocate) context */
void ht_destroy_context(_ht_context_t *p_htc) {
	if(p_htc->pf_mem_free)
		p_htc->pf_mem_free(p_htc, sizeof(_ht_context_t));
}
