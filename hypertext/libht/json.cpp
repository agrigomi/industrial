#include "iHT.h"
#include "json.h"

class cJSONValue: public _i_json_value_t, _json_value_t {
public:
	cJSONValue() {}
	~cJSONValue() {}

	_u8 type(void) {
		return jvt;
	}

	_i_json_value_t *by_index(_u32 index) {
		_i_json_value_t *r = 0;
		_json_value_t *_r = 0;

		if(jvt == JVT_OBJECT && index < object.num)
			_r = &object.pp_pairs[index]->value;
		else if(jvt == JVT_ARRAY && index < array.num)
			_r = array.pp_values[index];

		if(_r)
			r = (_i_json_value_t *)(cJSONValue *)_r;

		return r;
	}

	_cstr_t data(_u32 *size) {
		_cstr_t r = 0;

		if(jvt == JVT_STRING) {
			r = string.data;
			*size = string.size;
		} else if(jvt == JVT_NUMBER) {
			r = number.data;
			*size = number.size;
		}

		return r;
	}
};
