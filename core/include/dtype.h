#ifndef __MGTYPE__
#define __MGTYPE__

typedef unsigned long long	_u64;
typedef signed long long	_s64;
typedef unsigned int		_u32;
typedef signed int		_s32;
typedef unsigned short		_u16;
typedef short			_s16;
typedef unsigned char		_u8;
typedef char			_s8;
typedef _s8*			_str_t;
typedef _s8			_char_t;
typedef const char * 		_cstr_t;
typedef unsigned long		_ulong;
typedef signed long		_slong;
typedef unsigned int		_bool;

#define _false	(_bool)0
#define _true	(_bool)1

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef union {
	_u64	_qw;	/* quad word */
	struct {
 		_u32	_ldw;	/* lo dword */
		_u32	_hdw;	/* hi dword */
	};
}__attribute__((packed)) _u64_t;

#ifndef va_list
#define va_list		__builtin_va_list
#endif
#ifndef va_start
#define va_start(v,l)	__builtin_va_start(v,l)
#endif
#ifndef va_end
#define va_end(v)	__builtin_va_end(v)
#endif
#ifndef va_arg
#define va_arg(v,l)	__builtin_va_arg(v,l)
#endif

#endif
