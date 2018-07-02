#include <string.h>
#include "context.h"
#include "xml.h"

/* allocate memory for XML context */
_xml_context_t *xml_create_context(_mem_alloc_t *p_malloc, _mem_free_t *p_free) {
	_xml_context_t *r = NULL;

	if(p_malloc && p_free) {
		if((r = p_malloc(sizeof(_xml_context_t)))) {
			memset(r, 0, sizeof(_xml_context_t));

			/* create hypertext context */
			if(!(r->p_htc = ht_create_context(p_malloc, p_free))) {
				p_free(r, sizeof(_xml_context_t));
				r = NULL;
			}
		}
	}

	return r;
}

