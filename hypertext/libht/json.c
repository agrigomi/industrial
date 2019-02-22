#include <string.h>
#include "json.h"

_json_context_t *json_create_context(_mem_alloc_t *pf_alloc, _mem_free_t *pf_free) {
	_json_context_t *r = NULL;

	if(pf_alloc && pf_free) {
		if((r = (_json_context_t *)pf_alloc(sizeof(_json_context_t)))) {
			memset(r, 0, sizeof(_json_context_t));

			/* create hypertext context */
			if(!(r->p_htc = ht_create_context(pf_alloc, pf_free))) {
				pf_free(r, sizeof(_json_context_t));
				r = NULL;
			}
		}
	}

	return r;
}

static void destroy_object(_json_context_t *p_jxc, _json_object_t *p_jobj) {
	/*...*/
}

void json_reset_context(_json_context_t *p_jxc) {
	destroy_object(p_jxc, p_jxc->p_root);
	ht_reset_context(p_jxc->p_htc);
}

