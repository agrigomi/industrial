#include <string.h>
#include "json.h"

#define INITIAL_SIZE	16

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

static void destroy_object(_json_context_t *p_jcxt, _json_object_t *p_jobj) {
	/*...*/
}

void json_reset_context(_json_context_t *p_jcxt) {
	destroy_object(p_jcxt, &p_jcxt->root);
	ht_reset_context(p_jcxt->p_htc);
}

void json_destroy_context(_json_context_t *p_jcxt) {
	_ht_context_t *p_htc = p_jcxt->p_htc;

	destroy_object(p_jcxt, &p_jcxt->root);
	p_htc->pf_mem_free(p_jcxt, sizeof(_json_context_t));
	ht_destroy_context(p_htc);
}

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb, unsigned int *C);
static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray, unsigned int *C);
static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb, unsigned int *C);
static _json_err_t parse_string_name(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C);
static _json_err_t parse_string_value(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C);
static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum, unsigned int *C);

static _json_err_t alloc_array_values(_json_context_t *p_jcxt, _json_array_t *p_jarray) {
	_json_err_t r = JSON_OK;
	_ht_context_t *p_htc = p_jcxt->p_htc;
	_json_value_t **pp_old_jv = p_jarray->pp_values;
	unsigned int new_size = p_jarray->size + INITIAL_SIZE;
	_json_value_t **pp_new_jv = (_json_value_t **)p_jcxt->p_htc->pf_mem_alloc(new_size * sizeof(_json_value_t *));

	if(pp_new_jv) {
		memset(pp_new_jv, 0, new_size * sizeof(_json_value_t *));
		if(pp_old_jv) {
			/* copy to new one */
			memcpy(pp_new_jv, pp_old_jv, p_jarray->size * sizeof(_json_value_t *));
			/* release old one */
			p_htc->pf_mem_free(pp_old_jv, p_jarray->size * sizeof(_json_value_t *));
		}
		p_jarray->size = new_size;
		p_jarray->pp_values = pp_new_jv;
	} else
		r = JSON_MEMORY_ERROR;

	return r;
}

static _json_err_t alloc_object_pairs(_json_context_t *p_jcxt, _json_object_t *p_jobj) {
	_json_err_t r = JSON_OK;
	_ht_context_t *p_htc = p_jcxt->p_htc;
	_json_pair_t **pp_old_jp = p_jobj->pp_pairs;
	unsigned int new_size = p_jobj->size + INITIAL_SIZE;
	_json_pair_t **pp_new_jp = (_json_pair_t **)p_htc->pf_mem_alloc(new_size * sizeof(_json_pair_t *));

	if(pp_new_jp) {
		if(pp_old_jp) {
			memset(pp_new_jp, 0, new_size * sizeof(_json_pair_t *));
			/* copy to new one */
			memcpy(pp_new_jp, pp_old_jp, p_jobj->size * sizeof(_json_pair_t *));
			/* release old one */
			p_htc->pf_mem_free(pp_old_jp, p_jobj->size * sizeof(_json_pair_t *));
		}
		p_jobj->size = new_size;
		p_jobj->pp_pairs = pp_new_jp;
	} else
		r = JSON_MEMORY_ERROR;

	return r;
}

static _json_value_t *add_array_value(_json_context_t *p_jcxt, _json_array_t *p_jarray, _json_value_t *p_jvalue) {
	_json_value_t *r = NULL;
	unsigned int i = 0;

_add_array_value_:
	for(; i < p_jarray->size; i++) {
		if(p_jarray->pp_values[i] == NULL) {
			if((p_jarray->pp_values[i] = r = p_jcxt->p_htc->pf_mem_alloc(sizeof(_json_value_t))))
				memcpy(r, p_jvalue, sizeof(_json_value_t));
			break;
		}
	}

	if(i == p_jarray->size && r == NULL) {
		if(alloc_array_values(p_jcxt, p_jarray) == JSON_OK)
			goto _add_array_value_;
	}

	return r;
}

static _json_pair_t *add_object_pair(_json_context_t *p_jcxt, _json_object_t *p_jobj, _json_pair_t *p_jpair) {
	_json_pair_t *r = NULL;
	unsigned int i = 0;

_add_object_pair_:
	for(; i < p_jobj->size; i++) {
		if(p_jobj->pp_pairs[i] == NULL) {
			if((p_jobj->pp_pairs[i] = r = p_jcxt->p_htc->pf_mem_alloc(sizeof(_json_pair_t))))
				memcpy(r, p_jpair, sizeof(_json_pair_t));
			break;
		}
	}

	if(i == p_jobj->size && r == NULL) {
		if(alloc_object_pairs(p_jcxt, p_jobj) == JSON_OK)
			goto _add_object_pair_;
	}

	return r;
}

static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum, unsigned int *C) {
	_json_err_t r = JSON_OK;
	unsigned int c = 0;
	unsigned int _c = *C;
	unsigned long pos = ht_position(p_jcxt->p_htc);
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	unsigned char flags = 0;
#define NUM_DEC (1<<0)
#define NUM_HEX	(1<<1)

	if(p_jnum->data == NULL)
		p_jnum->data = (char *)ht_ptr(p_jcxt->p_htc);

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		if(c >= '0' && c <= '9') {
			/* decimal digit */
			if(c != '0')
				flags |= NUM_DEC;
		} else if((c == 'x' || c == 'X') && _c == '0' && !(flags & (NUM_DEC | NUM_HEX)))
			/* expect HEX digits */
			flags |= NUM_HEX;
		else if((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
			if(!(flags & NUM_HEX)) {
				p_jcxt->err_pos = pos;
				r = JSON_PARSE_ERROR;
				break;
			}
		} else if(c == ' ' ||
				c == ',' ||
				c == '\n' ||
				c == '\t' ||
				c == '\r' ||
				c == ']' ||
				c == '}') {
			if(flags & (NUM_HEX | NUM_DEC))
				break;
		} else if((c == '+' || c == '-') && !(flags & (NUM_HEX | NUM_DEC))) {
			;
		} else if(c == '.' && !(flags & NUM_HEX) && (flags & NUM_DEC)) {
			;
		} else {
			p_jcxt->err_pos = pos;
			r = JSON_PARSE_ERROR;
			break;
		}
		_c = c;
	}

	if(r == JSON_OK)
		p_jnum->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jnum->data,
					(unsigned char *)p_jnum->data + pos);
	*C = c;

	return r;
}

static _json_err_t parse_string_name(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C) {
	_json_err_t r = JSON_OK;
	unsigned int c = 0;
	unsigned int _c = *C;
	unsigned char flags = 0;
	unsigned long pos = ht_position(p_jcxt->p_htc);
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
#define STRNAME_QUOTES	(1<<0)
#define STRNAME_STROPHE	(1<<1)
#define STRNAME_SYMBOL	(1<<3)

	if(_c == '\'')
		flags |= STRNAME_STROPHE;
	else if(_c == '"')
		flags |= STRNAME_QUOTES;
	else if((_c >= 'A' && _c <= 'Z') || (_c >= 'a' && _c <='z'))
		flags |= STRNAME_SYMBOL;
	else {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = pos;
		return r;
	}

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		if(c >= 'A' && c <= 'Z')
			;
		else if(c >= 'a' && c <= 'z')
			;
		else if(c == '-' || c == '_')
			;
		else if(c == '\'') {
			if(flags & STRNAME_STROPHE)
				break;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else if(c == '"') {
			if(flags & STRNAME_QUOTES)
				break;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else if(c >= '0' && c <= '9') {
			if(!(flags & STRNAME_SYMBOL)) {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else if(c == ':' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if((flags & STRNAME_SYMBOL) && !(flags & (STRNAME_QUOTES | STRNAME_STROPHE)))
				break;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else {
			r = JSON_PARSE_ERROR;
			p_jcxt->err_pos = pos;
			break;
		}

		flags |= STRNAME_SYMBOL;
		_c = c;
	}

	if(r == JSON_OK)
		p_jstr->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jstr->data,
					(unsigned char *)p_jstr->data + pos);
	*C = c;

	return r;
}

static _json_err_t parse_string_value(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C) {
	_json_err_t r = JSON_OK;
	unsigned int c = 0;
	unsigned int _c = *C;
	unsigned long pos = ht_position(p_jcxt->p_htc);
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	unsigned char term = '"';

	if(_c == '\'' || _c == '"')
		term = _c;
	else {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = pos;
		return r;
	}

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		if(c == term && _c != '\\')
			break;

		_c = c;
	}

	if(r == JSON_OK)
		p_jstr->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jstr->data,
					(unsigned char *)p_jstr->data + pos);
	*C = c;

	return r;
}

static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray, unsigned int *C) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_value(_json_context_t *p_jcxt, _json_value_t *p_jvalue, unsigned int *C) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb, unsigned int *C) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

/* Pasrse JSON content */
_json_err_t json_parse(_json_context_t *p_jcxt, /* JSON context */
			const unsigned char *p_content, /* text content */
			unsigned long content_size /* size of text content in bytes */
			) {
	_json_err_t r = JSON_OK;

	if(p_jcxt->p_htc) {
		ht_init_context(p_jcxt->p_htc, (void *)p_content, content_size);
		r = parse_object(p_jcxt, &p_jcxt->root, NULL);
	} else
		r = JSON_PARSE_ERROR;

	return r;
}

_json_value_t *json_select(_json_context_t *p_jcxt,
			const char *jpath,
			_json_value_t *p_start_point, /* Can be NULL */
			unsigned int index) {
	_json_value_t *r = NULL;

	/*...*/

	return r;
}

