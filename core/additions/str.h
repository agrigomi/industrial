#ifndef __STR__
#define __STR__

#include "dtype.h"


#define INVALID_STRING_POSITION	0xffffffff

#ifdef __cplusplus
extern "C" {
#endif

void _mem_cpy(void *dst, void *src, _u32 sz);
_s32 _mem_cmp(void *p1, void *p2, _u32 sz);
void _mem_set(void *ptr, _u8 pattern, _u32 sz);
_u32 _str_len(_cstr_t str);
_s32 _str_cmp(_cstr_t str1, _cstr_t str2);
_s32 _str_ncmp(_cstr_t str1, _cstr_t str2, _u32 sz);
_u32 _str_cpy(_str_t dst, _cstr_t src, _u32 sz_max);
_u32 _vsnprintf(_str_t buf, _u32 size, _cstr_t fmt, va_list args);
_u32 _snprintf(_str_t buf, _u32 size, _cstr_t fmt, ...);
_s32 _nfind_string(_str_t text, _u32 text_sz, _cstr_t sub_str);
_s32 _find_string(_cstr_t str1,_cstr_t str2);
_str_t _toupper(_str_t s);
_u32 _str2i(_cstr_t str, _s32 sz);
void _trim_left(_str_t str);
void _trim_right(_str_t str);
void _clrspc(_str_t str);
_u8  _div_str(_cstr_t str,_str_t p1,_u32 sz_p1,_str_t p2,_u32 sz_p2,_cstr_t div);
_u8 _div_str_ex(_cstr_t str, _str_t p1, _u32 sz_p1, _str_t p2, _u32 sz_p2, _cstr_t div, _s8 start_ex, _s8 stop_ex);
_u32 _wildcmp(_cstr_t string, _cstr_t wild);
_str_t _itoa(_s32 n, _str_t s, _u8 b);
_str_t _uitoa(_u32 n, _str_t s, _u8 b);
_str_t _ulltoa(_u64 n, _str_t s, _u8 b);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
