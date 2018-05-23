#include "str.h"

#define ESCAPE	'\\'

void _mem_cpy(void *_dst, void *_src, _u32 sz) {
	if(sz) {
		_u8 *src = (_u8 *)_src;
		_u8 *dst = (_u8 *)_dst;
		_u32 _sz = sz >> 3;

		if(dst < src) {
			_u64 *_src = (_u64 *)src;
			_u64 *_dst = (_u64 *)dst;

			_u32 i = 0;
			while(i < _sz) {
				*(_dst + i) = *(_src + i);
				i++;
			}

			_u32 j = i << 3;
			while(j < sz) {
				*(dst + j) = *(src + j);
				j++;
			}
		} else {
			_u64 *_src = (_u64 *)(src + sz);
			_u64 *_dst = (_u64 *)(dst + sz);

			_u32 i = 0;
			while(i < _sz) {
				i++;
				*(_dst - i) = *(_src - i);
			}

			_u32 j = sz % 8;
			while(j) {
				j--;
				*(dst + j) = *(src + j);
			}
		}
	}
}

_s32  _mem_cmp(void *_p1, void *_p2, _u32 sz) {
	_s32 r = 0;
	_u32 _sz = sz;
	_u32 i = 0;
	_u8 *p1 = (_u8 *)_p1;
	_u8 *p2 = (_u8 *)_p2;

	while(_sz) {
		if((r = *(p1 + i) - *(p2 + i)))
			break;
		i++;
		_sz--;
	}

	return r;
}

_str_t _toupper(_str_t s) {
	_u32 i = 0;

	while(s[i]) {
		s[i] = (s[i]-32*(s[i]>='a' && s[i]<='z'));
		i++;
	}

	return s;
}

void _mem_set(void *_ptr, _u8 pattern, _u32 sz) {
 	_u32 _sz = sz;
	_u32 i = 0;
	_u8 *ptr = (_u8 *)_ptr;

	while(_sz) {
	 	*(ptr + i) = pattern;
		i++;
		_sz--;
	}
}

_u32 _str_len(_str_t str) {
	_u32 i = 0;

	while(*(str+i))
		i++;

	return i;
}

_s32 _str_cmp(_str_t str1, _str_t str2) {
	_s32 i = 0;

	while(str1[i] && str2[i] && str1[i] == str2[i])
		i++;

	return str1[i] - str2[i];
}

_s32 _str_ncmp(_str_t str1, _str_t str2, _u32 sz) {
	_s32 r = 0;
	_u32 i = 0;
	_u32 _sz = sz;

	while(_sz) {
		r = str1[i] - str2[i];
		if(r)
			break;

		_sz--;
		i++;
	}

	return r;
}

_u32 _str_cpy(_str_t dst, _str_t src, _u32 sz_max) {
	_u32 _sz = sz_max;
	_u32 i = 0;

	while(_sz) {
		dst[i] = src[i];
		if(!src[i])
			break;

		_sz--;
		i++;
	}

	dst[i] = 0;

	return i;
}

#define SPACE_CHAR ' '

_s32 _find_string(_str_t str1,_str_t str2) {
	_u32 i,l,l1;

	l = (_u32)_str_len(str2);
	l1= (_u32)_str_len(str1)+1;

	if(l > l1 || l1 == 0 || l == 0)
		return INVALID_STRING_POSITION;

	i=0;
	l1 -= l;

	while(l1--) {
		if(_mem_cmp((_u8 *)str2,(_u8 *)(str1+i),l) == 0)
			return i;
		i++;
	}

	return INVALID_STRING_POSITION;
}

void _trim_left(_str_t str) {
	_u32 i,l;

	l = (_u32)_str_len(str);

	for(i=0;i<l;i++) {
		if(str[i]!=SPACE_CHAR && str[i]!='\t')
			break;
	}

	if(i == 0) return;

	_mem_cpy((_u8 *)str,(_u8 *)(str+i),(_str_len(str+i)+1));
}

void _trim_right(_str_t str) {
	_u32 i,l;

	l= (_u32)_str_len(str);
	if(l) {
		i=l-1;

		while(str[i]==SPACE_CHAR || str[i]== '\t') {
			str[i]= 0;
			i--;
		}
	}
}

void _clrspc(_str_t str) {
	_trim_right(str);
	_trim_left(str);
}

static _u32 check_str1(_str_t str1,_str_t str2) {
	_u32  i,j,l1,l2;

	l1 = (_u32)_str_len(str1);
	l2 = (_u32)_str_len(str2);

	for(i=0;i<l1;i++) {
		for(j=0;j<l2;j++) {
			if(str1[i] == str2[j])
				return i;
		}
	}

	return INVALID_STRING_POSITION;
}

_u8  _div_str(_str_t str,_str_t p1,_u32 sz_p1,_str_t p2,_u32 sz_p2,_cstr_t div) {
	_u32  i;

	i = check_str1(str,(_str_t)div);
	if(i != INVALID_STRING_POSITION) {
		_mem_cpy((_u8 *)p1,(_u8 *)str,((_u32)i<sz_p1)?(_u32)i:sz_p1);
		p1[((_u32)i<sz_p1)?(_u32)i:sz_p1] = 0;
		_str_cpy(p2,(str+i+1),sz_p2);

		return 1;
	}

	_str_cpy(p1,str,sz_p1);
	p2[0] = 0;

	return 0;
}

_u8 _div_str_ex(_str_t str, _str_t p1, _u32 sz_p1, _str_t p2, _u32 sz_p2, _cstr_t div, _s8 start_ex, _s8 stop_ex) {
	_u32 ex_count=0;
	_u32 i,j,l1,l2;

	l1 = _str_len(str);
	l2 = _str_len((_str_t)div);

	for(i=0;i<l1;i++) {
		if(str[i] == ESCAPE) {
			i++;
		} else {
			if(!ex_count) {
				for(j=0;j<l2;j++) {
					if(str[i] == div[j]) {
						_mem_cpy((_u8 *)p1, (_u8 *)str, (i < (_u32)sz_p1) ? i : sz_p1);
						p1[(i<sz_p1)?i:sz_p1] = 0;
						_str_cpy(p2,(str+i+1),((_s32)sz_p2 < (_s32)_str_len(str+i)) ? sz_p2 : _str_len(str+i));
						return 1;
					}
				}
			}

			if(start_ex == stop_ex) {
				if(ex_count)
					ex_count = 0;
				else
					ex_count = 1;
			} else {
				if(str[i] == start_ex)
					ex_count++;

				if(str[i] == stop_ex)
					ex_count--;
			}
		}
	}

	_str_cpy(p1, str, sz_p1);
	p2[0] = 0;

	return 0;
}

_u32 _wildcmp(_str_t string,_str_t wild) {
	_str_t cp=0;
	_str_t mp=0;

	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?')) {
			return 0;
		}

		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}

			mp = wild;
			cp = string+1;
		} else {
			if ((*wild == *string) || (*wild == '?')) {
				wild++;
				string++;
			} else {
				wild = mp;
				string = cp++;
			}
		}
	}

	while (*wild == '*') {
		wild++;
	}

	return !*wild;
}

_str_t _strrev(_str_t str) {
	_str_t p1, p2;

	if (!str || !*str)
		return str;

	for (p1 = str, p2 = str + _str_len(str) - 1; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}

	return str;
}

_u32 _str2i(_str_t str, _s32 sz) {
	_u32 r = 0;
	_s8 _sz = sz-1;
	_u32 m = 1;

	while(_sz >= 0) {
		if(*(str + _sz) >= '0' && *(str + _sz) <= '9') {
			r += (*(str + _sz) - 0x30) * m;
			m *= 10;
		}

		_sz--;
	}

	return r;
}

static _s8 digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
_str_t _itoa(_s32 n, _str_t s, _u8 b) {
	_u32 i=0;
	_s32  sign;

	if((sign = n) < 0)
		n = -n;

	do {
		s[i++] = digits[n % b];
	} while ((n /= b) > 0);

	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';

	return _strrev(s);
}

_str_t _uitoa(_u32 n, _str_t s, _u8 b) {
	_u32 i=0;

	do {
		s[i++] = digits[n % b];
	} while ((n /= b) > 0);

	s[i] = '\0';

	return _strrev(s);
}

_str_t _ulltoa(_u64 n, _str_t s, _u8 b) {
	_u32 i=0;

	do {
		s[i++] = digits[n % b];
	} while ((n /= b) > 0);

	s[i] = '\0';

	return _strrev(s);
}


_u32 _vsnprintf(_str_t buf, _u32 size, _cstr_t fmt, va_list args) {
	_u32 r = 0;
	_u32 _r = 0;
	_u32 l = _str_len((_str_t)fmt);
	_u8 t = 0;
	_u32 tp = 0;
	_u32 i = 0;
	_s8 lb[256];

	_str_t 	_arg_str;
	_s32   	_arg_s32;
	_u32 	_arg_u32;
	_u64	_arg_u64;
	_s8 	_arg_b;

	for(i = 0; i < l; i++) {
		if(t) {
			switch(fmt[i]) {
				case 'c':
					_r = 1;
					_arg_b = (_s8)va_arg(args, _u32);
					buf[r] = _arg_b;
					r++;
					t = 0;
					break;

				case 's':  /* small letters */
					_r = 0;
					_arg_str = (_str_t)va_arg(args,_str_t);
					_r = _str_len(_arg_str);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)_arg_str, _r);
					r += _r;
					buf[r] = 0;
					t = 0;
					break;

				case 'S':  /* big letters */
					_r = 0;

					_arg_str = (_str_t)va_arg(args,_str_t);
					_r = _str_len(_arg_str);
					if((size - 1 - r) < _r)
						_r = (size -1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)_arg_str, _r);
					buf[r + _r] = 0;
					_toupper(buf+r);
					r += _r;
					t = 0;
					break;

				case 'd':  /* signed int 32 */
					_r = 0;
					_arg_s32 = (_s32)va_arg(args, _s32);
					_itoa(_arg_s32, lb, 10);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				case 'u':  /* unsigned int 32 */
					_r = 0;

					_arg_u32 = (_u32)va_arg(args, _u32);
					_uitoa(_arg_u32, lb, 10);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				case 'l':  /* unsigned int 64 */
					_r = 0;

					_arg_u32 = (_u32)va_arg(args, _u64);
					_ulltoa(_arg_u32, lb, 10);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				case 'x': /* small letters hex v */
					_r = 0;

					_arg_u32 = (_u32)va_arg(args, _u32);
					_uitoa(_arg_u32, lb, 16);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				case 'X':  /* big letters hex */
					_r = 0;

					_arg_u32 = (_u32)va_arg(args, _u32);
					_uitoa(_arg_u32, lb, 16);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					_toupper(buf+r);
					r += _r;
					t = 0;
					break;

				case 'h': /* small letters long long hex */
					_r = 0;

					_arg_u64 = (_u64)va_arg(args, _u64);
					_ulltoa(_arg_u64, lb, 16);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				case 'H':  /* big letters long long hex */
					_r = 0;

					_arg_u64 = (_u64)va_arg(args, _u64);
					_ulltoa(_arg_u64, lb, 16);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					_toupper(buf+r);
					r += _r;
					t = 0;
					break;

				case 'b':  /* binary */
					_r = 0;

					_arg_u64 = (_u64)va_arg(args, _u64);
					_ulltoa(_arg_u64, lb, 2);
					_r = _str_len(lb);
					if((size - 1 - r) < _r)
						_r = (size - 1 - r);

					_mem_cpy((_u8 *)buf+r, (_u8 *)lb, _r);
					buf[r + _r] = 0;
					r += _r;
					t = 0;
					break;

				default:
					buf[r] = fmt[tp];
					r++;
					buf[r] = fmt[i];
					r++;
					buf[r] = 0;
					t = 0;
					break;
			}
		} else {
			if(fmt[i] == '%') {
				t = 1;
				tp = i;
			} else {
				buf[r] = fmt[i];
				r++;
				buf[r] = 0;
			}
		}
	}

	return r;
}


_u32 _snprintf(_str_t buf, _u32 size, _cstr_t fmt, ...) {
	va_list args;
	_u32 i;

	va_start(args, fmt);
	i=_vsnprintf(buf,size,fmt,args);
	va_end(args);
	return i;
}
