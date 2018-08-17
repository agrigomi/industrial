#include <string.h>
#include "context.h"
#include "xml.h"

#define INITIAL_TAG_LIST	16
#define INITIAL_TDEF_ARRAY	16

/* state bitmask */
#define SCOPE_OPEN	1	/* < */
#define STROPHE		2	/* ' */
#define QUOTES		4	/* " */
#define SLASH		8	/* / */
#define SYMBOL		16
#define EXCLAM		32	/* ! */
#define DASH		64	/* - */
#define COMMENT		128	/* <!-- */
#define IGNORE		256
#define EQUAL		512	/* = */
#define ESCAPE		1024	/* \ */
#define PI		2048	/* <? */

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
static _tag_def_t *find_tdef(_xml_context_t *p_xc, unsigned char *p_tname, unsigned int sz) {
	_tag_def_t * r = NULL;

	if(p_xc->pp_tdef) {
		unsigned int n = 0;

		while(n < p_xc->num_tdefs) {
			if(p_xc->pp_tdef[n]) {
				if(ht_compare(p_xc->p_htc, p_tname,
						(unsigned char *)p_xc->pp_tdef[n]->p_tag_name, sz) == 0) {
					r = p_xc->pp_tdef[n];
					break;
				}
			}
			n++;
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

static _xml_err_t _xml_parse(_xml_context_t *p_xc, _ht_tag_t *p_parent_tag) {
	_xml_err_t r = XML_PARSE_ERROR;
	unsigned int state = 0;
	_ht_content_t *p_hc = &p_xc->p_htc->ht_content;
	_ht_tag_t *p_ctag = NULL;
	unsigned long pos = ht_position(p_xc->p_htc);
	unsigned char *ptr_tag_name = NULL;
	unsigned long sz_tag_name = 0;
	unsigned char *ptr_tag_params = NULL;
	unsigned long sz_tag_params = 0;
	unsigned long pos_scope_open = 0;
	unsigned int c = 0;
	unsigned int _c = 0;

	while((c = p_xc->p_htc->pf_read(p_hc, &pos))) {
		if(c == '>') {
			if(!(state & (QUOTES | STROPHE | ESCAPE))) {
				if(state & SCOPE_OPEN) {
					if(state & COMMENT) {
						if((state & DASH) && !(state & SYMBOL)) {
							state &= ~(COMMENT | DASH | SCOPE_OPEN);
							ptr_tag_name = ptr_tag_params = NULL;
							sz_tag_name = sz_tag_params = pos_scope_open = 0;
						}
					} else if((state & IGNORE) && _c == '?') {
							if(state & PI) {
								/* create processing instructions tag */
								if((p_ctag = xml_create_tag(p_xc, p_parent_tag))) {
									p_ctag->p_name = ptr_tag_name;
									p_ctag->sz_name = sz_tag_name;
									p_ctag->p_parameters = p_ctag->p_content = ptr_tag_params;
									p_ctag->sz_parameters = p_ctag->sz_content = sz_tag_params;
									p_ctag->sz_end_tag = 2;
									p_ctag->p_tdef = find_tdef(p_xc, ptr_tag_name, sz_tag_name);
								} else {
									r = XML_MEMORY_ERROR;
									break;
								}
								state &= ~PI;
							}
							ptr_tag_name = ptr_tag_params = NULL;
							sz_tag_name = sz_tag_params = pos_scope_open = 0;
							state &= ~(IGNORE | SCOPE_OPEN);
					} else {
						if(!(state & (COMMENT | IGNORE | ESCAPE))) {
							if(ptr_tag_name && !sz_tag_name)
								sz_tag_name = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_name);
							if(ptr_tag_params && !sz_tag_params)
								sz_tag_params = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_params);

							_tag_def_t *p_tdef = find_tdef(p_xc, ptr_tag_name, sz_tag_name);
							unsigned int tflags = (p_tdef) ? p_tdef->flags : 0;

							if(state & SLASH) {
								/* close tag */
								if(ht_compare(p_xc->p_htc, ptr_tag_name,
										p_parent_tag->p_name, p_parent_tag->sz_name) == 0) {
									/* close parent tag */
									p_parent_tag->sz_end_tag = ht_symbols(p_xc->p_htc,
											ht_ptr(p_xc->p_htc),
											p_hc->p_content + pos_scope_open);
									p_parent_tag->sz_content = ht_symbols(p_xc->p_htc,
											p_parent_tag->p_content,
											p_hc->p_content + pos_scope_open);
									r = XML_OK;
									break;
								} else {
									if(!(state & SYMBOL) && _c == '/') {
										/* close one line tag */
										if((p_ctag = xml_create_tag(p_xc, p_parent_tag))) {
											p_ctag->p_name = ptr_tag_name;
											p_ctag->sz_name = sz_tag_name;
											p_ctag->p_parameters = ptr_tag_params;
											p_ctag->sz_parameters = sz_tag_params;
											p_ctag->sz_end_tag = 2;
											p_ctag->p_tdef = p_tdef;
										} else {
											r = XML_MEMORY_ERROR;
											break;
										}
									} else { /* parse error */
										r = XML_PARSE_ERROR;
										p_xc->err_pos = pos;
										break;
									}
								}
								state &= ~SLASH;
							} else {
								/* open tag */
								if((p_ctag = xml_create_tag(p_xc, p_parent_tag))) {
									p_ctag->p_name = ptr_tag_name;
									p_ctag->sz_name = sz_tag_name;
									p_ctag->p_content = ht_ptr(p_xc->p_htc);
									p_ctag->p_parameters = ptr_tag_params;
									p_ctag->sz_parameters = sz_tag_params;
									p_ctag->p_tdef = p_tdef;

									if(!(tflags & TF_ONELINE)) {
										if((r = _xml_parse(p_xc, p_ctag)) != XML_OK)
											break;
									}
								} else {
									r = XML_MEMORY_ERROR;
									break;
								}
							}
							ptr_tag_name = ptr_tag_params = NULL;
							sz_tag_name = sz_tag_params = pos_scope_open = 0;
							state &= ~SCOPE_OPEN;
						}
					}
				} else {
					p_xc->err_pos = pos;
					break;
				}
				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '<') {
			if(!(state & (QUOTES | STROPHE | COMMENT | IGNORE | ESCAPE))) {
				if(!(state & SCOPE_OPEN)) {
					state |= SCOPE_OPEN;
					pos_scope_open = pos;
				} else {
					p_xc->err_pos = pos;
					break;
				}
				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '/') {
			if(!(state & (QUOTES | STROPHE | COMMENT | IGNORE | ESCAPE))) {
				if(state & SCOPE_OPEN) {
					if(ptr_tag_name && !sz_tag_name)
						sz_tag_name = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_name);
					if(ptr_tag_params && !sz_tag_params)
						sz_tag_params = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_params);
					state |= SLASH;
				}
				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '\'') {
			if(!(state & (COMMENT | IGNORE | ESCAPE)))
				state ^= STROPHE;
			state &= ~(SYMBOL | ESCAPE);
		} else if(c == '"') {
			if(!(state & (COMMENT | IGNORE | ESCAPE)))
				state ^= QUOTES;
			state &= ~(SYMBOL | ESCAPE);
		} else if(c == ' ' || c == '\t' || c == '\n') {
			if(!(state & (QUOTES | STROPHE | COMMENT | IGNORE | ESCAPE))) {
				if(state & SCOPE_OPEN) {
					if(state & SYMBOL) {
						if(ptr_tag_name && !ptr_tag_params) {
							ptr_tag_params = ht_ptr(p_xc->p_htc);
							sz_tag_name = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_name);
							sz_tag_params = 0;
							if(state & PI)
								state |= IGNORE;
						}
					}
				}
				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '!') {
			if(!(state & (QUOTES | STROPHE | COMMENT | IGNORE | ESCAPE))) {
				if((state & SCOPE_OPEN) && _c == '<') {
					if(!(state & SYMBOL))
						state |= EXCLAM;
				}

				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '-') {
			if(!(state & (QUOTES | STROPHE | IGNORE | ESCAPE))) {
				if((state & SCOPE_OPEN) && !(state & SYMBOL)) {
					if(state & EXCLAM) {
						if((state & DASH) && _c == '-') {
							/* second dash */
							state |= COMMENT;
							state &= ~EXCLAM;
						}
					}
					state |= DASH;
				}
				state &= ~(ESCAPE);
			}
		} else if(c == '?') {
			if(!(state & (QUOTES | STROPHE | COMMENT | ESCAPE))) {
				if((state & SCOPE_OPEN)) {
					if(_c == '<' && !(state & (SYMBOL | DASH | EXCLAM))) {
						if(!(state & PI))
							state |= PI;
					} else {
						if(ptr_tag_params && (state & PI))
							sz_tag_params = ht_symbols(p_xc->p_htc, (p_hc->p_content + pos), ptr_tag_params);
					}
				}
				state &= ~(SYMBOL | ESCAPE);
			}
		} else if(c == '\\') {
			if(!(state & ESCAPE) && _c != '\\')
				state |= ESCAPE;
			else
				state &= ~ESCAPE;
			state &= ~SYMBOL;
		} else {
			if(!(state & (QUOTES|STROPHE))) {
				if((state & SCOPE_OPEN) && !(state & (SYMBOL | COMMENT | IGNORE | ESCAPE))) {
					if(!ptr_tag_name) {
						ptr_tag_name = p_hc->p_content + pos;
						ptr_tag_params = 0;
						sz_tag_name = 0;
						sz_tag_params = 0;
					}
				}
				state |= SYMBOL;
				state &= ~(EXCLAM | DASH | ESCAPE);
			}
		}

		_c = c;
	}

	if(!c)
		r = XML_OK;

	return r;
}

static unsigned char *root_tag_name = (unsigned char*)"ht_root";

_xml_err_t xml_parse(_xml_context_t *p_xc, /* XML context */
			unsigned char *p_xml_content, /* XML content */
			unsigned long sz_xml_content /* size of XML content */
			) {
	_xml_err_t r = XML_PARSE_ERROR;
	_ht_context_t *phtc = p_xc->p_htc;

	if(phtc && phtc->ht_content.p_content == NULL) {
		ht_init_context(phtc, p_xml_content, sz_xml_content);

		if(!p_xc->p_root)
			p_xc->p_root = xml_create_tag(p_xc, NULL);

		if(p_xc->p_root) {
			p_xc->p_root->p_name = root_tag_name;
			p_xc->p_root->sz_name = strlen((char *)root_tag_name);
			p_xc->p_root->p_content = p_xml_content;
			p_xc->p_root->sz_content = sz_xml_content;
			r = _xml_parse(p_xc, p_xc->p_root);
		}
	}

	return r;
}

static _ht_tag_t *find_tag(_xml_context_t *p_xc, const char *tname,
			unsigned int sz_tname,
			_ht_tag_t *p_tparent, unsigned int index) {
	_ht_tag_t *r = NULL;
	_ht_tag_t *p_tstart = (p_tparent) ? p_tparent : p_xc->p_root;

	if(p_tstart) {
		unsigned int i = 0;
		unsigned int idx = 0;

		for(; i < p_tstart->num_childs; i++) {
			if(p_tstart->pp_childs[i]) {
				if(tname && sz_tname) {
					/* by name and index */
					if(sz_tname == p_tstart->pp_childs[i]->sz_name) {
						if(ht_compare(p_xc->p_htc, (unsigned char *)tname,
								p_tstart->pp_childs[i]->p_name,
								sz_tname) == 0) {
							if(idx == index) {
								r = p_tstart->pp_childs[i];
								break;
							}
							idx++;
						}
					}
				} else {
					/* by index only */
					if(idx == index) {
						r = p_tstart->pp_childs[i];
						break;
					}
					idx++;
				}
			}
		}
	}

	return r;
}

_ht_tag_t *xml_select(_xml_context_t *p_xc,
			const char *xpath,
			_ht_tag_t *p_start_point, /* start tag (can be NULL) */
			unsigned int index) {
	_ht_tag_t *r = p_start_point;
	const char *tname = NULL;
	unsigned int sz_tname = 0;
	unsigned int l = (xpath) ? strlen(xpath) : 0;
	unsigned int i = 0;

	for(i = 0; i < l; i++) {
		if(xpath[i] == '/') {
			if(tname) {
				sz_tname = (xpath + i) - tname;
				if((r = find_tag(p_xc, tname, sz_tname, r, index))) {
					tname = NULL;
					sz_tname = 0;
				} else
					return r;
			}
		} else {
			if(!tname)
				tname = xpath + i;
		}
	}

	sz_tname = (tname) ? ((xpath + i) - tname) : 0;
	r = find_tag(p_xc, tname, sz_tname, r, index);

	return r;
}

/* get parameter value */
unsigned char *xml_tag_parameter(_xml_context_t *p_xc, _ht_tag_t *p_tag,
			const char *pname, unsigned int *sz) {
	unsigned char *r = NULL;
	unsigned char *p_var = NULL;
	unsigned char *p_val = NULL;
	unsigned char sz_var = 0;
	unsigned char sz_val = 0;
	unsigned int state = 0;

	if(p_tag->p_parameters && p_tag->sz_parameters) {
		unsigned char *p_end = p_tag->p_parameters + p_tag->sz_parameters;
		unsigned char *ptr = p_tag->p_parameters;
		unsigned char *_ptr = NULL;
		unsigned int c = 0;
		unsigned int _c = 0;

		while(ptr < p_end) {
			_ptr = ptr; /* backup the current pointer */
			_c = c; /* backup the current symbol */

			if(!(c = ht_read(p_xc->p_htc, ptr, &ptr)))
				break;

			if(c == '"') {
				if(!(state & (ESCAPE | STROPHE))) {
					if(state & QUOTES) {
						/* closed */
						if(p_var && sz_var && p_val && !sz_val) {
							/* get value size and check variable name */
							sz_val = ht_symbols(p_xc->p_htc, _ptr, p_val);
							if(ht_compare(p_xc->p_htc, p_var, (unsigned char *)pname, sz_var) == 0) {
								r = p_val;
								break;
							}
							p_var = p_val = NULL;
							sz_var = sz_val = 0;
							state = 0;
							continue;
						} else
							/* syntax error */
							break;
					}
					state ^= QUOTES;
				}
				state &= ~(SYMBOL | ESCAPE);
			} else if(c == '\'') {
				if(!(state & (ESCAPE | QUOTES))) {
					if(state & STROPHE) {
						/* closed */
						if(p_var && sz_var && p_val && !sz_val) {
							/* get value size and check variable name */
							sz_val = ht_symbols(p_xc->p_htc, _ptr, p_val);
							if(ht_compare(p_xc->p_htc, p_var, (unsigned char *)pname, sz_var) == 0) {
								r = p_val;
								break;
							}
							p_var = p_val = NULL;
							sz_var = sz_val = 0;
							state = 0;
							continue;
						} else
							/* syntax error */
							break;
					}
					state ^= STROPHE;
				}
				state &= ~(SYMBOL | ESCAPE);
			} else if(c == '=') {
				if(!(state & (STROPHE | QUOTES | ESCAPE))) {
					if(state & EQUAL)
						break;
					if(p_var && !sz_var && (state & SYMBOL))
						sz_var = ht_symbols(p_xc->p_htc, _ptr, p_var);
					state |= EQUAL;
				}
				state &= ~(SYMBOL | ESCAPE);
			} else if(c == ' ') {
				if(!(state & (STROPHE | QUOTES | ESCAPE | EQUAL))) {
					if(p_var && !sz_var && (state & SYMBOL))
						sz_var = ht_symbols(p_xc->p_htc, _ptr, p_var);
				}
				state &= ~(SYMBOL | ESCAPE);
			} else if(c == '\\') {
				if(!(state & ESCAPE) && _c != '\\')
					state |= ESCAPE;
				else
					state &= ~ESCAPE;
				state &= ~SYMBOL;
			} else {
				if(!(state & (SYMBOL | ESCAPE))) {
					if(!(state & (STROPHE | QUOTES))) {
						if(!p_var && !(state & EQUAL)) {
							p_var = _ptr;
							sz_var = 0;
							p_val = NULL;
							sz_val = 0;
						}
					} else {
						if(!p_val && (state & EQUAL) && p_var && sz_var) {
							p_val = _ptr;
							sz_val = 0;
						}
					}
				}
				state |= SYMBOL;
				state &= ~ESCAPE;
			}
		}
	}

	*sz = sz_val;

	return r;
}

/* get tag content */
unsigned char *xml_tag_content(_ht_tag_t *p_tag, unsigned int *sz) {
	*sz = p_tag->sz_content;
	return p_tag->p_content;
}

static void xml_destroy_tag(_xml_context_t *p_xc, _ht_tag_t *p_tag) {
	unsigned int i = 0;

	for(; i < p_tag->num_childs; i++) {
		if(p_tag->pp_childs[i]) {
			xml_destroy_tag(p_xc, p_tag->pp_childs[i]);
			/* deallocate child tag */
			p_xc->p_htc->pf_mem_free(p_tag->pp_childs[i], sizeof(_ht_tag_t));
		}
	}

	/* deallocate array for child tags */
	p_xc->p_htc->pf_mem_free(p_tag->pp_childs, p_tag->num_childs * sizeof(_ht_tag_t *));
}

void xml_destroy_context(_xml_context_t *p_xc) {
	_ht_context_t *p_htc = p_xc->p_htc;

	if(p_xc->p_root) /* release root tag */
		xml_destroy_tag(p_xc, p_xc->p_root);

	if(p_xc->pp_tdef) /* release tag definitions */
		p_htc->pf_mem_free(p_xc->pp_tdef, p_xc->num_tdefs * sizeof(_tag_def_t *));

	/* deallocate XML context */
	p_htc->pf_mem_free(p_xc, sizeof(_xml_context_t));
	/* destroy hypertext context */
	ht_destroy_context(p_htc);
}

void xml_add_tdef(_xml_context_t *p_xc, /* XML context */
		_tag_def_t *p_tdef /* array of tag definitions */
		) {
	unsigned int i = 0;

	for(; i < p_xc->num_tdefs; i++) {
		if(p_xc->pp_tdef[i] == NULL)
			break;
	}

	if(i == p_xc->num_tdefs) {
		/* need to expand */
		unsigned int new_blen = (INITIAL_TDEF_ARRAY + p_xc->num_tdefs) * sizeof(_tag_def_t *);
		_tag_def_t **p_new = p_xc->p_htc->pf_mem_alloc(new_blen);

		if(p_new) {
			memset(p_new, 0, new_blen);
			if(p_xc->pp_tdef) {
				memcpy(p_new, p_xc->pp_tdef, p_xc->num_tdefs * sizeof(_tag_def_t *));
				p_xc->p_htc->pf_mem_free(p_xc->pp_tdef, p_xc->num_tdefs * sizeof(_tag_def_t *));
			}

			p_xc->pp_tdef = p_new;
			i = p_xc->num_tdefs;
			p_xc->num_tdefs += INITIAL_TDEF_ARRAY;
		} else
			return;
	}

	p_xc->pp_tdef[i] = p_tdef;
}

void xml_remove_tdef(_xml_context_t *p_xc, /* XML context */
			_tag_def_t *p_tdef /* array of tag definitions. */
			) {
	unsigned int i = 0;

	for(; i < p_xc->num_tdefs; i++) {
		if(p_xc->pp_tdef[i] == p_tdef) {
			p_xc->pp_tdef[i] = NULL;
			break;
		}
	}
}
