#include <string.h>
#include "json.h"

#define INITIAL_SIZE	16;

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

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb);
static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray);
static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb);
static _json_err_t parse_string(_json_context_t *p_jcxt, _json_string_t *p_jstr);
static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum);

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

	/*...*/

	return r;
}

static _json_pair_t *add_object_pair(_json_context_t *p_jcxt, _json_object_t *p_jobj, _json_pair_t *p_jpair) {
	_json_pair_t *r = NULL;

	/*...*/

	return r;
}

static _json_err_t parse_number(_json_context_t *p_jcxt, _json_number_t *p_jnum) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_string(_json_context_t *p_jcxt, _json_string_t *p_jstr) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_array(_json_context_t *p_jcxt, _json_array_t *p_jarray) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_value(_json_context_t *p_jcxt, _json_value_t *p_jvalue) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

static _json_err_t parse_object(_json_context_t *p_jcxt, _json_object_t *p_jogb) {
	_json_err_t r = JSON_OK;

	/*...*/

	return r;
}

/* Pasrse JSON content */
_json_err_t json_parse(_json_context_t *p_jcxt, /* JSON context */
			unsigned char *p_content,
			unsigned long content_size) {
	_json_err_t r = JSON_OK;

	if(p_jcxt->p_htc) {
		ht_init_context(p_jcxt->p_htc, p_content, content_size);
		r = parse_object(p_jcxt, &p_jcxt->root);
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

