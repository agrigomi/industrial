#include <string.h>
#include "context.h"
#include "xml.h"

#define INITIAL_TAG_LIST	16

/* state bitmask */
#define SCOPE_OPEN	1	/* < */
#define STROPHE		2	/* ' */
#define QUOTES		4	/* " */
#define SLASH		8	/* / */
#define TAG_OPEN	16	/* <name */
#define TAG_PARAMS	32	/* <name ... */
#define SYMBOL		64

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

/* find tag definitian */
static _tag_def_t *find_tdef(_xml_context_t *p_xc, unsigned long pos, unsigned int sz) {
	_tag_def_t * r = NULL;

	if(p_xc->pp_tdef) {
		unsigned char *p_doc = p_xc->p_htc->ht_content.p_content;
		unsigned long sz_doc = p_xc->p_htc->ht_content.sz_content;

		if((pos + sz)  < sz_doc) {
			unsigned char *ptr = p_doc + pos;
			unsigned int n = 0;

			while(n < p_xc->num_tdef) {
				if(p_xc->pp_tdef[n]) {
					if(ht_compare(p_xc->p_htc, ptr,
							(unsigned char *)p_xc->pp_tdef[n]->p_tag_name, sz) == 0) {
						r = p_xc->pp_tdef[n];
						break;
					}
				}
				n++;
			}
		}
	}

	return r;
}

static _ht_tag_t *xml_create_tag(_xml_context_t *p_xc, _ht_tag_t *p_parent) {
	_ht_tag_t *r = NULL;

	if(p_parent) {
		if(p_parent->pp_childs) {
			int n = 0;
_find_empty_:
			while(n < p_parent->num_childs) {
				if(p_parent->pp_childs[n] == NULL)
					break;
				n++;
			}

			if(n == p_parent->num_childs) {
				/* the tag list must be expanded */
				unsigned int size = p_parent->num_childs + INITIAL_TAG_LIST;
				_ht_tag_t **pp_new_list = p_xc->p_htc->pf_mem_alloc(size * sizeof(_ht_tag_t *));

				if(pp_new_list) {
					memset(pp_new_list, 0, size * sizeof(_ht_tag_t *));
					memcpy(pp_new_list, p_parent->pp_childs, p_parent->num_childs * sizeof(_ht_tag_t *));
					p_xc->p_htc->pf_mem_free(p_parent->pp_childs, p_parent->num_childs * sizeof(_ht_tag_t *));
					p_parent->pp_childs = pp_new_list;
					p_parent->num_childs = size;
					goto _find_empty_;
				}
			} else {
				/* alloc tag in position n of parent list */
				if((r = p_parent->pp_childs[n] = p_xc->p_htc->pf_mem_alloc(sizeof(_ht_tag_t))))
					memset(r, 0, sizeof(_ht_tag_t));
			}
		} else {
			/* alloc tag list */
			if((p_parent->pp_childs = p_xc->p_htc->pf_mem_alloc(INITIAL_TAG_LIST * sizeof(_ht_tag_t *)))) {
				memset(p_parent->pp_childs, 0, INITIAL_TAG_LIST * sizeof(_ht_tag_t *));
				p_parent->num_childs = INITIAL_TAG_LIST;
				/* alloc tag in position 0 of parent list */
				if((r = p_parent->pp_childs[0] = p_xc->p_htc->pf_mem_alloc(sizeof(_ht_tag_t))))
					memset(r, 0, sizeof(_ht_tag_t));
			}
		}
	} else {
		/* alloc tag */
		if((r = p_xc->p_htc->pf_mem_alloc(sizeof(_ht_tag_t))))
			memset(r, 0, sizeof(_ht_tag_t));
	}

	return r;
}

static _xml_err_t _xml_parse(_xml_context_t *p_xc, unsigned int state, _ht_tag_t *p_parent_tag) {
	_xml_err_t r = XML_PARSE_ERROR;
	_ht_content_t *p_hc = &p_xc->p_htc->ht_content;
	_ht_tag_t *p_ctag = NULL;
	unsigned long pos = ht_position(p_xc->p_htc);
	unsigned char *ptr_tag_name = NULL;
	unsigned char *ptr_tag_params = NULL;
	unsigned int c = 0;

	while((c = p_xc->p_htc->pf_read(p_hc, &pos))) {
		if(c == '>') {
			if(!(state & (QUOTES|STROPHE))) {
				if(state & SCOPE_OPEN) {
					if(state & SLASH) {
						/* close tag */
						if(ht_compare(p_xc->p_htc, p_parent_tag->p_name, ptr_tag_name, p_parent_tag->sz_name) == 0) {
							/* close parent tag */
							p_parent_tag->sz_content = (p_parent_tag->p_content + pos) - p_parent_tag->p_content; /*???*/
							r = XML_OK;
							break;
						} else {
							/* close one line tag */
							/*...*/
						}
						state &= ~SLASH;
					} else {
						/* open tag */
						if((p_ctag = xml_create_tag(p_xc, p_parent_tag))) {
							p_ctag->p_name = ptr_tag_name;
							p_ctag->sz_name = (p_hc->p_content + pos) - ptr_tag_name;
							p_ctag->p_content = ht_ptr(p_xc->p_htc);
							p_ctag->p_parameters = ptr_tag_params;
							p_ctag->sz_parameters = (p_hc->p_content + pos) - ptr_tag_params;
							/*...*/
							if((r = _xml_parse(p_xc, 0, p_ctag)) == XML_OK)
								p_ctag->sz_content = ht_ptr(p_xc->p_htc) - p_ctag->p_content;
							else
								break;
						} else
							break;
					}
				} else {
					p_xc->err_pos = pos;
					break;
				}
				state &= ~(SCOPE_OPEN|SYMBOL);
			}
		} else if(c == '<') {
			if(!(state & (QUOTES|STROPHE))) {
				/* fix position and pointer for tag name */
				ptr_tag_name = ht_ptr(p_xc->p_htc);
				/*...*/
				state |= SCOPE_OPEN;
				state &= ~SYMBOL;
			}
		} else if(c == '/') {
			if(!(state & (QUOTES|STROPHE))) {
				if(state & SCOPE_OPEN) {
					/* fix position and pointer for tag name */
					ptr_tag_name = ht_ptr(p_xc->p_htc);
					state |= SLASH;
				}
			}
		} else if(c == '\'')
			state ^= STROPHE;
		else if(c == '"')
			state ^= QUOTES;

		/*...*/
	}

	return r;
}

static unsigned char *root_tag_name = (unsigned char*)"ht_root";

_xml_err_t xml_parse(_xml_context_t *p_xc, /* XML context */
			unsigned char *p_xml_content, /* XML content */
			unsigned long sz_xml_content /* size of XML content */
			) {
	_xml_err_t r = XML_PARSE_ERROR;
	_ht_context_t *phtc = p_xc->p_htc;

	if(phtc) {
		ht_init_context(phtc, p_xml_content, sz_xml_content);

		if(!p_xc->p_root)
			p_xc->p_root = xml_create_tag(p_xc, NULL);

		if(p_xc->p_root) {
			p_xc->p_root->p_name = root_tag_name;
			p_xc->p_root->sz_name = strlen((char *)root_tag_name);
			p_xc->p_root->p_content = p_xml_content;
			p_xc->p_root->sz_content = sz_xml_content;
			r = _xml_parse(p_xc, 0, p_xc->p_root);
		}
	}

	return r;
}

