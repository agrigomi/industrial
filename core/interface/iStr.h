#ifndef __I_STR_H__
#define __I_STR_H__

#include "iBase.h"

#define I_STR	"iStr"

class iStr: public iBase {
public:
	INTERFACE(iStr, I_STR);

	virtual void mem_cpy(void *dst, void *src, _u32 sz)=0;
	virtual _s32 mem_cmp(void *p1, void *p2, _u32 sz)=0;
	virtual void mem_set(void *ptr, _u8 pattern, _u32 sz)=0;
	virtual _u32 str_len(_cstr_t str)=0;
	virtual _s32 str_cmp(_cstr_t str1, _cstr_t str2)=0;
	virtual _s32 str_ncmp(_cstr_t str1, _cstr_t str2, _u32 sz)=0;
	virtual _u32 str_cpy(_str_t dst, _cstr_t src, _u32 sz_max)=0;
	virtual _u32 vsnprintf(_str_t buf, _u32 size, _cstr_t fmt, va_list args)=0;
	virtual _u32 snprintf(_str_t buf, _u32 size, _cstr_t fmt, ...)=0;
	virtual _s32 find_string(_cstr_t str1,_cstr_t str2)=0;
	virtual _s32 nfind_string(_str_t text, _u32 text_sz, _cstr_t sub_str)=0;
	virtual _str_t toupper(_str_t s)=0;
	virtual _u32 str2i(_cstr_t str, _s32 sz)=0;
	virtual void trim_left(_str_t str)=0;
	virtual void trim_right(_str_t str)=0;
	virtual void clrspc(_str_t str)=0;
	virtual _u8 div_str(_cstr_t str,_str_t p1,_u32 sz_p1,_str_t p2,_u32 sz_p2,_cstr_t div)=0;
	virtual _u8 div_str_ex(_cstr_t str, _str_t p1, _u32 sz_p1, _str_t p2, _u32 sz_p2, _cstr_t div, _s8 start_ex, _s8 stop_ex)=0;
	virtual _u32 wildcmp(_cstr_t string,_cstr_t wild)=0;
	virtual _str_t itoa(_s32 n, _str_t s, _u8 b)=0;
	virtual _str_t uitoa(_u32 n, _str_t s, _u8 b)=0;
	virtual _str_t ulltoa(_u64 n, _str_t s, _u8 b)=0;
};

#endif
