#include "str.h"
#include "iStr.h"

class cStr: public iStr {
public:
	BASE(cStr, "cStr", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	void mem_cpy(void *dst, void *src, _u32 sz) {
		_mem_cpy(dst, src, sz);
	}

	_s32 mem_cmp(void *p1, void *p2, _u32 sz) {
		return _mem_cmp(p1, p2, sz);
	}

	void mem_set(void *ptr, _u8 pattern, _u32 sz) {
		_mem_set(ptr, pattern, sz);
	}

	_u32 str_len(_cstr_t str) {
		return _str_len(str);
	}

	_s32 str_cmp(_cstr_t str1, _cstr_t str2) {
		return _str_cmp(str1, str2);
	}

	_s32 str_ncmp(_cstr_t str1, _cstr_t str2, _u32 sz) {
		return _str_ncmp(str1, str2, sz);
	}

	_u32 str_cpy(_str_t dst, _cstr_t src, _u32 sz_max) {
		return _str_cpy(dst, src,sz_max);
	}

	_u32 vsnprintf(_str_t buf, _u32 size, _cstr_t fmt, va_list args) {
		return _vsnprintf(buf, size, fmt, args);
	}

	_u32 snprintf(_str_t buf, _u32 size, _cstr_t fmt, ...) {
		va_list args;
		_u32 i;

		va_start(args, fmt);
		i = _vsnprintf(buf, size, fmt, args);
		va_end(args);
		return i;
	}

	_s32 find_string(_cstr_t str1,_cstr_t str2) {
		return _find_string(str1, str2);
	}

	_s32 nfind_string(_str_t text, _u32 text_sz, _cstr_t sub_str) {
		return _nfind_string(text, text_sz, sub_str);
	}

	_str_t toupper(_str_t s) {
		return _toupper(s);
	}

	_u32 str2i(_cstr_t str, _s32 sz) {
		return _str2i(str, sz);
	}

	void trim_left(_str_t str) {
		_trim_left(str);
	}

	void trim_right(_str_t str) {
		_trim_right(str);
	}

	void clrspc(_str_t str) {
		_clrspc(str);
	}

	_u8 div_str(_cstr_t str,_str_t p1,_u32 sz_p1,_str_t p2,_u32 sz_p2,_cstr_t div) {
		return _div_str(str, p1, sz_p1, p2, sz_p2, div);
	}

	_u8 div_str_ex(_cstr_t str, _str_t p1, _u32 sz_p1, _str_t p2, _u32 sz_p2, _cstr_t div, _s8 start_ex, _s8 stop_ex) {
		return _div_str_ex(str, p1, sz_p1, p2, sz_p2, div, start_ex, stop_ex);
	}

	_u32 wildcmp(_cstr_t string, _cstr_t wild) {
		return _wildcmp(string, wild);
	}

	_str_t itoa(_s32 n, _str_t s, _u8 b) {
		return _itoa(n, s, b);
	}

	_str_t uitoa(_u32 n, _str_t s, _u8 b) {
		return _uitoa(n, s, b);
	}

	_str_t ulltoa(_u64 n, _str_t s, _u8 b) {
		return _ulltoa(n ,s, b);
	}
};

static cStr _g_str_;
