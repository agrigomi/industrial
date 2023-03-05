#include <string.h>
#include <stdlib.h>
#include "json.h"

#define INITIAL_SIZE	16

_json_context_t *json_create_context(_mem_alloc_t *pf_alloc, _mem_free_t *pf_free, void *udata) {
	_json_context_t *r = NULL;

	if(pf_alloc && pf_free) {
		if((r = (_json_context_t *)pf_alloc(sizeof(_json_context_t), udata))) {
			memset(r, 0, sizeof(_json_context_t));
			r->udata = udata;
			/* create hypertext context */
			if(!(r->p_htc = ht_create_context(pf_alloc, pf_free, udata))) {
				pf_free(r, sizeof(_json_context_t), udata);
				r = NULL;
			}
		}
	}

	return r;
}

static void destroy_object(_json_context_t *p_jcxt, _json_object_t *p_jobj);
static void destroy_array(_json_context_t *p_jcxt, _json_array_t *p_jarray);

static void destroy_array(_json_context_t *p_jcxt, _json_array_t *p_jarray) {
	unsigned int i = 0;

	if(p_jarray) {
		for(; i < p_jarray->num; i++) {
			_json_value_t *p_jvalue = p_jarray->pp_values[i];

			if(p_jvalue) {
				if(p_jvalue->jvt == JSON_ARRAY)
					destroy_array(p_jcxt, &p_jvalue->array);
				else if(p_jvalue->jvt == JSON_OBJECT)
					destroy_object(p_jcxt, &p_jvalue->object);

				p_jcxt->p_htc->pf_mem_free(p_jvalue, sizeof(_json_value_t), p_jcxt->udata);
			}
		}
	}
}

static void destroy_object(_json_context_t *p_jcxt, _json_object_t *p_jobj) {
	unsigned int i = 0;

	if(p_jobj) {
		for(; i < p_jobj->num; i++) {
			_json_pair_t *p_jpair = p_jobj->pp_pairs[i];

			if(p_jpair) {
				if(p_jpair->value.jvt == JSON_OBJECT)
					destroy_object(p_jcxt, &p_jpair->value.object);
				else if(p_jpair->value.jvt == JSON_ARRAY)
					destroy_array(p_jcxt, &p_jpair->value.array);

				p_jcxt->p_htc->pf_mem_free(p_jpair, sizeof(_json_pair_t), p_jcxt->udata);
			}
		}
	}
}

void json_reset_context(_json_context_t *p_jcxt) {
	destroy_object(p_jcxt, &p_jcxt->root);
	ht_reset_context(p_jcxt->p_htc);
}

void json_destroy_context(_json_context_t *p_jcxt) {
	_ht_context_t *p_htc = p_jcxt->p_htc;

	destroy_object(p_jcxt, &p_jcxt->root);
	p_htc->pf_mem_free(p_jcxt, sizeof(_json_context_t), p_jcxt->udata);
	ht_destroy_context(p_htc);
}

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb, unsigned int *C);
static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray, unsigned int *C);
static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb, unsigned int *C);
static _json_err_t parse_string_name(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C, unsigned long cpos);
static _json_err_t parse_string_value(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C);
static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum, unsigned int *C, unsigned long cpos);
static _json_err_t parse_value(_json_context_t *p_jcxt, _json_value_t *p_jvalue, unsigned int *C, unsigned long cpos);
static _json_err_t parse_const(_json_context_t *p_jcxt, _json_value_t *p_jvalue, unsigned int *C, unsigned long cpos);

static _json_err_t alloc_array_values(_json_context_t *p_jcxt, _json_array_t *p_jarray) {
	_json_err_t r = JSON_OK;
	_ht_context_t *p_htc = p_jcxt->p_htc;
	_json_value_t **pp_old_jv = p_jarray->pp_values;
	unsigned int new_size = p_jarray->num + INITIAL_SIZE;
	_json_value_t **pp_new_jv = (_json_value_t **)p_jcxt->p_htc->pf_mem_alloc(new_size * sizeof(_json_value_t *), p_jcxt->udata);

	if(pp_new_jv) {
		memset(pp_new_jv, 0, new_size * sizeof(_json_value_t *));
		if(pp_old_jv) {
			/* copy to new one */
			memcpy(pp_new_jv, pp_old_jv, p_jarray->num * sizeof(_json_value_t *));
			/* release old one */
			p_htc->pf_mem_free(pp_old_jv, p_jarray->num * sizeof(_json_value_t *), p_jcxt->udata);
		}
		p_jarray->num = new_size;
		p_jarray->pp_values = pp_new_jv;
	} else
		r = JSON_MEMORY_ERROR;

	return r;
}

static _json_err_t alloc_object_pairs(_json_context_t *p_jcxt, _json_object_t *p_jobj) {
	_json_err_t r = JSON_OK;
	_ht_context_t *p_htc = p_jcxt->p_htc;
	_json_pair_t **pp_old_jp = p_jobj->pp_pairs;
	unsigned int new_size = p_jobj->num + INITIAL_SIZE;
	_json_pair_t **pp_new_jp = (_json_pair_t **)p_htc->pf_mem_alloc(new_size * sizeof(_json_pair_t *), p_jcxt->udata);

	if(pp_new_jp) {
		memset(pp_new_jp, 0, new_size * sizeof(_json_pair_t *));
		if(pp_old_jp) {
			/* copy to new one */
			memcpy(pp_new_jp, pp_old_jp, p_jobj->num * sizeof(_json_pair_t *));
			/* release old one */
			p_htc->pf_mem_free(pp_old_jp, p_jobj->num * sizeof(_json_pair_t *), p_jcxt->udata);
		}
		p_jobj->num = new_size;
		p_jobj->pp_pairs = pp_new_jp;
	} else
		r = JSON_MEMORY_ERROR;

	return r;
}

static _json_value_t *add_array_value(_json_context_t *p_jcxt, _json_array_t *p_jarray, _json_value_t *p_jvalue) {
	_json_value_t *r = NULL;
	unsigned int i = 0;

_add_array_value_:
	for(; i < p_jarray->num; i++) {
		if(p_jarray->pp_values[i] == NULL) {
			if((p_jarray->pp_values[i] = r = p_jcxt->p_htc->pf_mem_alloc(sizeof(_json_value_t), p_jcxt->udata)))
				memcpy(r, p_jvalue, sizeof(_json_value_t));
			break;
		}
	}

	if(i == p_jarray->num && r == NULL) {
		if(alloc_array_values(p_jcxt, p_jarray) == JSON_OK)
			goto _add_array_value_;
	}

	return r;
}

static _json_pair_t *add_object_pair(_json_context_t *p_jcxt, _json_object_t *p_jobj, _json_pair_t *p_jpair) {
	_json_pair_t *r = NULL;
	unsigned int i = 0;

_add_object_pair_:
	for(; i < p_jobj->num; i++) {
		if(p_jobj->pp_pairs[i] == NULL) {
			if((p_jobj->pp_pairs[i] = r = p_jcxt->p_htc->pf_mem_alloc(sizeof(_json_pair_t), p_jcxt->udata)))
				memcpy(r, p_jpair, sizeof(_json_pair_t));
			break;
		}
	}

	if(i == p_jobj->num && r == NULL) {
		if(alloc_object_pairs(p_jcxt, p_jobj) == JSON_OK)
			goto _add_object_pair_;
	}

	return r;
}

static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum, unsigned int *C, unsigned long cpos) {
	_json_err_t r = JSON_OK;
	unsigned int c = *C;
	unsigned int i = 0;
	unsigned long pos = cpos;
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	unsigned char nt = 0;

#define NUM_OCT	1
#define NUM_DEC	2
#define NUM_HEX	3

	if(p_jnum->data == NULL)
		p_jnum->data = (char *)p_hc->p_content + pos;

	if(c == '0')
		nt = NUM_OCT;

	do {
		if(c >= '0' && c <= '9') {
			if(!nt)
				nt = NUM_DEC;
		} else if((c == 'x' || c == 'X') && i == 1 && nt == NUM_OCT)
			nt = NUM_HEX;
		else if((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
			if(nt != NUM_HEX) {
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
			if(nt)
				break;
		} else if((c == '+' || c == '-') && !nt) {
			;
		} else if(c == '.' && (nt == NUM_DEC || nt == NUM_OCT)) {
			;
		} else {
			p_jcxt->err_pos = pos;
			r = JSON_PARSE_ERROR;
			break;
		}
		i++;
	} while((c = p_jcxt->p_htc->pf_read(p_hc, &pos)));

	if(r == JSON_OK)
		p_jnum->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jnum->data,
					(unsigned char *)p_hc->p_content + pos);
	*C = c;

	return r;
}

static _json_err_t parse_string_name(_json_context_t *p_jcxt, _json_string_t *p_jstr, unsigned int *C, unsigned long cpos) {
	_json_err_t r = JSON_OK;
	unsigned int c = *C;
	unsigned char flags = 0;
	unsigned long pos = cpos;
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
#define STRNAME_QUOTES	(1<<0)
#define STRNAME_STROPHE	(1<<1)
#define STRNAME_SYMBOL	(1<<3)

	if(c == '\'') {
		flags |= STRNAME_STROPHE;
		c = p_jcxt->p_htc->pf_read(p_hc, &pos);
	} else if(c == '"') {
		flags |= STRNAME_QUOTES;
		c = p_jcxt->p_htc->pf_read(p_hc, &pos);
	} else if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <='z') || c == '_')
		flags |= STRNAME_SYMBOL;
	else {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = pos;
		return r;
	}

	if(p_jstr->data == NULL)
		p_jstr->data = (char *)p_hc->p_content + pos;

	do {
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
	} while((c = p_jcxt->p_htc->pf_read(p_hc, &pos)));

	if(r == JSON_OK)
		p_jstr->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jstr->data,
					(unsigned char *)p_hc->p_content + pos);
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

	if(p_jstr->data == NULL)
		p_jstr->data = (char *)ht_ptr(p_jcxt->p_htc);

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		if(c == term && _c != '\\')
			break;

		_c = c;
	}

	if(r == JSON_OK)
		p_jstr->size = ht_symbols(p_jcxt->p_htc,
					(unsigned char *)p_jstr->data,
					(unsigned char *)p_hc->p_content + pos);
	*C = c;

	return r;
}

static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray, unsigned int *C) {
	_json_err_t r = JSON_OK;
	unsigned int c = *C;
	unsigned long pos = ht_position(p_jcxt->p_htc);
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	_json_value_t jvalue;

	if(c != '[') {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = pos;
		return r;
	}

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		memset(&jvalue, 0, sizeof(_json_value_t));

		if((r = parse_value(p_jcxt, &jvalue, &c, pos)) == JSON_OK) {
			if(jvalue.jvt == 0 &&
					(c == ',' || c == ' ' || c == '\r' ||
					 c == '\n' || c == '\t'))
				;
			else if(jvalue.jvt == 0 && c == ']')
				break;
			else if(jvalue.jvt == JSON_STRING && (c == '"' || c == '\''))
				;
			else if(jvalue.jvt == JSON_ARRAY && c == ']')
				;
			else if(jvalue.jvt == JSON_OBJECT && c == '}')
				;
			else if((jvalue.jvt == JSON_NUMBER ||
				 jvalue.jvt == JSON_TRUE ||
				 jvalue.jvt == JSON_FALSE ||
				 jvalue.jvt == JSON_NULL) &&
					(c == ',' || c == ' ' || c == '\r' ||
					 c == '\n' || c == '\t'))
				;
			else if((jvalue.jvt == JSON_NUMBER  ||
				 jvalue.jvt == JSON_TRUE ||
				 jvalue.jvt == JSON_FALSE ||
				 jvalue.jvt == JSON_NULL) && c == ']') {
				if(!add_array_value(p_jcxt, p_jarray, &jvalue))
					r = JSON_MEMORY_ERROR;
				break;
			} else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = ht_position(p_jcxt->p_htc);
				break;
			}
			if(jvalue.jvt && !add_array_value(p_jcxt, p_jarray, &jvalue)) {
				r = JSON_MEMORY_ERROR;
				break;
			}
		} else {
			p_jcxt->err_pos = ht_position(p_jcxt->p_htc);
			break;
		}

		if(c == ']')
			break;
	}

	*C = c;

	return r;
}

static _json_err_t parse_const(_json_context_t *p_jcxt, _json_value_t *p_jvalue, unsigned int *C, unsigned long cpos) {
	_json_err_t r = JSON_OK;
	unsigned int c = *C;
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	unsigned long pos = cpos;
	unsigned int sz = 0;

	while(c >= 'a' && c <= 'z') {
		c = p_jcxt->p_htc->pf_read(p_hc, &pos);
		sz++;
	}

	if(ht_compare(p_jcxt->p_htc, (unsigned char *)"true", p_hc->p_content + cpos, sz) == 0)
		p_jvalue->jvt = JSON_TRUE;
	else if(ht_compare(p_jcxt->p_htc, (unsigned char *)"false", p_hc->p_content + cpos, sz) == 0)
		p_jvalue->jvt = JSON_FALSE;
	else if(ht_compare(p_jcxt->p_htc, (unsigned char *)"null", p_hc->p_content + cpos, sz) == 0)
		p_jvalue->jvt = JSON_NULL;
	else {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = cpos;
	}

	*C = c;

	return r;
}

static _json_err_t parse_value(_json_context_t *p_jcxt, _json_value_t *p_jvalue, unsigned int *C, unsigned long cpos) {
	_json_err_t r = JSON_OK;
	unsigned int c = *C;
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	unsigned long pos = cpos;

	do {
		/* looking for something after ':' to (',' or '}' or ']') */
		if(c == '"' || c == '\'') {
			p_jvalue->jvt = JSON_STRING;
			r = parse_string_value(p_jcxt, &p_jvalue->string, &c);
			break;
		} else if(c == '[') {
			p_jvalue->jvt = JSON_ARRAY;
			r = parse_array(p_jcxt, &p_jvalue->array, &c);
			break;
		} else if(c == '{') {
			p_jvalue->jvt = JSON_OBJECT;
			r = parse_object(p_jcxt, &p_jvalue->object, &c);
			break;
		} else if((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
			p_jvalue->jvt = JSON_NUMBER;
			r = parse_number(p_jcxt, &p_jvalue->number, &c, pos);
			break;
		} else if(c >= 'a' && c <= 'z') {
			r = parse_const(p_jcxt, p_jvalue, &c, pos);
			break;
		} else if(c == ',' || c == '}' || c == ']')
			break;
		else if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
			;
		else {
			r = JSON_PARSE_ERROR;
			p_jcxt->err_pos = pos;
			break;
		}
	} while((c = p_jcxt->p_htc->pf_read(p_hc, &pos)));

	*C = c;

	return r;
}

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jobj, unsigned int *C) {
	_json_err_t r = JSON_OK;
	unsigned int c = (C) ? *C : '{';
	unsigned long pos = ht_position(p_jcxt->p_htc);
	_ht_content_t *p_hc = &p_jcxt->p_htc->ht_content;
	_json_pair_t jpair;
	unsigned char flags = 0;

#define JPAIR_NAME	(1<<0)
#define JPAIR_VALUE	(1<<1)

	if(c != '{') {
		r = JSON_PARSE_ERROR;
		p_jcxt->err_pos = pos;
		return r;
	}

	memset(&jpair, 0, sizeof(_json_pair_t));

	while((c = p_jcxt->p_htc->pf_read(p_hc, &pos))) {
		if(!(flags & JPAIR_NAME) && !(flags & JPAIR_VALUE)) {
			if(c == '"' || c == '\'') {
				flags |= JPAIR_NAME;
				if((r = parse_string_name(p_jcxt, &jpair.name, &c, pos)) != JSON_OK)
					break;
				if(c != '"' && c != '\'') {
					r = JSON_PARSE_ERROR;
					p_jcxt->err_pos = pos;
					break;
				}
			} else if((c >= 'A' && c <= 'Z') ||
					(c >= 'a' && c <= 'z') ||
					c == '_') {
				flags |= JPAIR_NAME;
				if((r = parse_string_name(p_jcxt, &jpair.name, &c, pos)) != JSON_OK)
					break;
				if(c == ':')
					goto _pair_value_;
			} else if(c == ',' || c == '{' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
				;
			else if( c == '}')
				break;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else if((flags & JPAIR_NAME) && !(flags & JPAIR_VALUE)) {
			if(c == ':') {
_pair_value_:
				flags |= JPAIR_VALUE;
				/* dummy read */
				c = p_jcxt->p_htc->pf_read(p_hc, &pos);
				if((r = parse_value(p_jcxt, &jpair.value, &c, pos)) != JSON_OK)
					break;
				if(jpair.value.jvt == 0 && c == ',')
					flags = 0;
				else if(jpair.value.jvt == JSON_STRING && (c == '"' || c == '\''))
					;
				else if(jpair.value.jvt == JSON_ARRAY && c == ']')
					;
				else if(jpair.value.jvt == JSON_OBJECT && c == '}')
					;
				else if((jpair.value.jvt == JSON_NUMBER ||
					 jpair.value.jvt == JSON_TRUE ||
					 jpair.value.jvt == JSON_FALSE ||
					 jpair.value.jvt == JSON_NULL) &&
						(c == ' ' || c == '\r' ||
						 c == '\n' || c == '\t'))
					;
				else if((jpair.value.jvt == JSON_NUMBER ||
					 jpair.value.jvt == JSON_TRUE ||
					 jpair.value.jvt == JSON_FALSE ||
					 jpair.value.jvt == JSON_NULL) && c == ',')
					flags = 0;
				else if((jpair.value.jvt == JSON_NUMBER ||
					 jpair.value.jvt == JSON_TRUE ||
					 jpair.value.jvt == JSON_FALSE ||
					 jpair.value.jvt == JSON_NULL) && c == '}') {
					if(!add_object_pair(p_jcxt, p_jobj, &jpair))
						r = JSON_MEMORY_ERROR;
					break;
				} else {
					r = JSON_PARSE_ERROR;
					p_jcxt->err_pos = ht_position(p_jcxt->p_htc);
					break;
				}
				if(jpair.value.jvt && !add_object_pair(p_jcxt, p_jobj, &jpair)) {
					r = JSON_MEMORY_ERROR;
					break;
				}

				memset(&jpair, 0, sizeof(_json_pair_t));
			} else if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
				;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		} else if((flags & JPAIR_NAME) && (flags & JPAIR_VALUE)) {
			if(c == ',') {
				flags = 0;
				memset(&jpair, 0, sizeof(_json_pair_t));
			} else if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
				;
			else if(c == '}')
				break;
			else {
				r = JSON_PARSE_ERROR;
				p_jcxt->err_pos = pos;
				break;
			}
		}
	}

	if(C)
		*C = c;

	return r;
}

/* Pasrse JSON content */
_json_err_t json_parse(_json_context_t *p_jcxt, /* JSON context */
			const unsigned char *p_content, /* text content */
			unsigned long content_size /* size of text content in bytes */
			) {
	_json_err_t r = JSON_OK;

	if(p_jcxt->p_htc) {
		json_reset_context(p_jcxt);
		ht_init_context(p_jcxt->p_htc, (void *)p_content, content_size);
		r = parse_object(p_jcxt, &p_jcxt->root, NULL);
	} else
		r = JSON_PARSE_ERROR;

	return r;
}

static _json_value_t *object_pair_by_name(_ht_context_t *p_htc, _json_object_t *p_jobj, const char *name, unsigned int sz_name) {
	_json_value_t *r = NULL;
	unsigned int i = 0;
	_json_pair_t *p_jpair = NULL;

	for(; i < p_jobj->num; i++) {
		if((p_jpair = p_jobj->pp_pairs[i])) {
			if(p_jpair->name.size == sz_name && p_jpair->name.data) {
				if(ht_compare(p_htc, (unsigned char *)p_jpair->name.data, (unsigned char *)name, sz_name) == 0) {
					r = &p_jpair->value;
					break;
				}
			}
		}
	}

	return r;
}

static _json_value_t *array_element_by_index(_json_array_t *p_jarray, unsigned int index) {
	_json_value_t *r = NULL;

	if(index < p_jarray->num)
		r = p_jarray->pp_values[index];

	return r;
}

_json_value_t *json_select(_json_context_t *p_jcxt,
			const char *jpath,
			_json_object_t *p_start_point /* Can be NULL */
			) {
	_json_value_t *r = NULL;
	_json_value_t *tmp = NULL;
	_json_object_t *p_start = (p_start_point) ? p_start_point : &p_jcxt->root;
	unsigned int i = 0;
	unsigned int l = strlen(jpath) + 1;
	const char *str = NULL;
	unsigned int sz = 0;
	char c = 0;

	for(; i < l; i++) {
		c = *(jpath + i);

		if(c == '.' || c == '/') {
			if(str) {
				if((tmp = object_pair_by_name(p_jcxt->p_htc, p_start, str, sz))) {
					if(tmp->jvt == JSON_OBJECT)
						p_start = &tmp->object;
				} else
					break;
			}

			sz = 0;
			str = NULL;
		} else if(c == 0) {
			if(str)
				r = object_pair_by_name(p_jcxt->p_htc, p_start, str, sz);
			else if(tmp)
				r = tmp;
			break;
		} else if(c == '[') {
			if(str) {
				if((tmp = object_pair_by_name(p_jcxt->p_htc, p_start, str, sz))) {
					if(tmp->jvt != JSON_ARRAY && tmp->jvt != JSON_OBJECT)
						break;
				} else
					break;
			} else
				break;

			str = NULL;
			sz = 0;
		} else if(c == ']') {
			if(str) {
				unsigned int aidx = atoi(str);

				if(tmp->jvt == JSON_ARRAY) {
					if((tmp = array_element_by_index(&tmp->array, aidx))) {
						if(tmp->jvt == JSON_OBJECT)
							p_start = &tmp->object;
						else {
							r = tmp;
							break;
						}
					}
				} else if(tmp->jvt == JSON_OBJECT) {
					_json_pair_t *pair = json_object_pair(&tmp->object, aidx);

					if(pair) {
						tmp = &pair->value;
						if(tmp->jvt == JSON_OBJECT)
							p_start = &tmp->object;
						else {
							r = tmp;
							break;
						}
					}
				}
			}

			str = NULL;
			sz = 0;
		} else if(c < 0x20) {
			;
		} else {
			if(!str)
				str = jpath + i;
			sz++;
		}
	}

	return r;
}

_json_value_t *json_array_element(_json_array_t *p_jarray, unsigned int index) {
	return array_element_by_index(p_jarray, index);
}

_json_pair_t *json_object_pair(_json_object_t *p_jobj, unsigned int index) {
	_json_pair_t *r = NULL;

	if(index < p_jobj->num)
		r = p_jobj->pp_pairs[index];

	return r;
}

_json_value_t *json_object_value(_json_object_t *p_jobj, unsigned int index) {
	_json_value_t *r = NULL;
	_json_pair_t *p_jpair = json_object_pair(p_jobj, index);

	if(p_jpair)
		r = &p_jpair->value;

	return r;
}
