#include "iBase.h"
#include "iRepository.h"
#include "iMemory.h"
#include "iHT.h"
#include "json.h"


class cJSON: public iJSON {
private:
	iHeap *mpi_heap;
public:
	BASE(cJSON, "cJSON", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL)))
					r = true;
				break;
			case OCTL_UNINIT:
				_gpi_repo_->object_release(mpi_heap);
				r = true;
				break;
		}

		return r;
	}

	// Create parser context
	HTCONTEXT create_context(void) {
		_json_context_t *r = json_create_context([](_u32 size, void *udata)->void* { // alloc
			void *r = 0;
			cJSON *pobj = (cJSON *)udata;

			if(pobj->mpi_heap)
				r = pobj->mpi_heap->alloc(size);
			return r;
		}, [](void *ptr, _u32 size, void *udata) { // free
			cJSON *pobj = (cJSON *)udata;

			if(pobj->mpi_heap)
				pobj->mpi_heap->free(ptr, size);
		}, this);

		return r;
	}
	// destroy parser context
	void destroy_context(HTCONTEXT htc) {
		json_destroy_context((_json_context_t *)htc);
	}
	// Parse (indexing) hypertext content
	bool parse(HTCONTEXT htc, // parser context
			_cstr_t content, // Content of hypertext
			_ulong size // Size of hypertext content
			) {
		return (json_parse((_json_context_t *)htc, (const unsigned char *)content, size) == JSON_OK) ? true : false;
	}
	// Compare strins (according content encoding)
	_s32 compare(HTCONTEXT htc,
			_str_t str1, // string 1
			_str_t str2, // string 2
			_u32 sz// size
			) {
		_json_context_t *p_jcxt = (_json_context_t *)htc;

		return ht_compare(p_jcxt->p_htc, (unsigned char *)str1, (unsigned char *)str2, sz);
	}
	// Select value
	HTVALUE select(HTCONTEXT htc,
			_cstr_t jpath, // path to value
			HTVALUE start // Start point (can be NULL)
			) {
		_json_object_t *p_jobj = 0;
		_json_value_t *p_jv = (_json_value_t *)start;

		if(p_jv) {
			if(p_jv->jvt == JSON_OBJECT)
				p_jobj = &p_jv->object;
		}

		return json_select((_json_context_t *)htc, jpath, p_jobj);
	}

	_u8 type(HTVALUE hjv) {
		_u8 r = 0;
		_json_value_t *p_jv = (_json_value_t *)hjv;

		switch(p_jv->jvt) {
			case JSON_STRING:
				r = JVT_STRING;
				break;
			case JSON_NUMBER:
				r = JVT_NUMBER;
				break;
			case JSON_OBJECT:
				r = JVT_OBJECT;
				break;
			case JSON_ARRAY:
				r = JVT_ARRAY;
				break;
		}

		return r;
	}

	_cstr_t data(HTVALUE hjv, _u32 *size) { // for JVT_STRING and JVT_NUMBER only
		_cstr_t r = 0;
		_json_value_t *p_jv = (_json_value_t *)hjv;

		if(p_jv->jvt == JSON_STRING)
			r = p_jv->string.data;
		else if(p_jv->jvt == JSON_NUMBER)
			r = p_jv->number.data;

		return r;
	}

	HTVALUE by_index(HTVALUE hjv, _u32 index) { // for JVT_ARRAY and JVT_OBJECT only
		_json_value_t *r = 0;
		_json_value_t *p_jv = (_json_value_t *)hjv;

		if(p_jv->jvt == JSON_ARRAY)
			r = json_array_element(&p_jv->array, index);
		else if(p_jv->jvt == JSON_OBJECT)
			r = json_object_value(&p_jv->object, index);

		return r;
	}
};

static cJSON _g_json_;
