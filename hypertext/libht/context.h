#ifndef __HT_CONTEXT_H__
#define __HT_CONTEXT_H__

#define MACHINE_ORDER_LE	1
#define MACHINE_ORDER_BE	2

#define E_UTF_8		0
#define E_UTF_16_BE	1
#define E_UTF_16_LE	2
#define E_UTF_32_BE	3
#define E_UTF_32_LE	4

typedef struct { /* hypertext content info. */
	unsigned char	*p_content; /* hypertext content */
	unsigned long	sz_content; /* sizeof hypertext content */
	unsigned long	c_pos; /* current position in content */
	unsigned char	encoding; /* content ancoding */
	unsigned char	machine_order;
} _ht_content_t;

typedef void *_mem_alloc_t(unsigned int nbytes, void *udata);
typedef void _mem_free_t(void *ptr, unsigned int nbytes, void *udata);
typedef unsigned int _read_t(_ht_content_t *p_ht_content, unsigned long *pos);

typedef struct { /* context of hypertext parser */
	_ht_content_t	ht_content; /* hypertext content info. */
	_mem_alloc_t	*pf_mem_alloc; /* pinter to memory allocation */
	_mem_free_t	*pf_mem_free; /* pointer to memory free */
	_read_t		*pf_read; /* pointer to read symbol */
	void		*udata;
} _ht_context_t;

#ifdef __cplusplus
extern "C" {
#endif
/* allocate memory for hypertext context */
_ht_context_t *ht_create_context(_mem_alloc_t *, _mem_free_t *, void *);
/* initialize context with content and size of hypertext */
void ht_init_context(_ht_context_t *, void *, unsigned long);
/* Reset context */
void ht_reset_context(_ht_context_t *);
unsigned long ht_position(_ht_context_t *);
unsigned char *ht_ptr(_ht_context_t *);
/* compare memory with taking care about document encoding */
int ht_compare(_ht_context_t *, unsigned char *, unsigned char *, unsigned int);
/* return number of symbols between two pointers */
unsigned int ht_symbols(_ht_context_t *, unsigned char *, unsigned char *);
unsigned int ht_bytes(_ht_context_t *, unsigned int num_symbols);
/* read symbol */
unsigned int ht_read(_ht_context_t *, /* context */
			unsigned char *, /* input pointer */
			unsigned char ** /* output pointer (to next symbol) */
			);
/* destroy (deallocate) context */
void ht_destroy_context(_ht_context_t *);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
