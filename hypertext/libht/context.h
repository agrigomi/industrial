#ifndef __HT_CONTEXT_H__
#define __HT_CONTEXT_H__

#define MACHINE_ORDER_LE	1
#define MACHINE_ORDER_BE	2

typedef struct { /* hypertext content info. */
	void 		*p_content; /* hypertext content */
	unsigned long	sz_content; /* sizeof hypertext content */
	unsigned long	c_pos; /* current position in content */
	unsigned char	machine_order;
} _ht_content_t;

typedef void *_mem_alloc_t(unsigned int nbytes);
typedef void _mem_free_t(void *ptr, unsigned int nbytes);
typedef unsigned int _read_t(_ht_content_t *p_ht_content);

typedef struct { /* context of hypertext parser */
	_ht_content_t	ht_content; /* hypertext content info. */
	_mem_alloc_t	*pf_mem_alloc; /* pinter to memory allocation */
	_mem_free_t	*pf_mem_free; /* pointer to memory free */
	_read_t		*pf_read; /* pointer to read symbol */
} _ht_context_t;

/* allocate memory for hypertext context */
_ht_context_t *ht_create_context(_mem_alloc_t *, _mem_free_t *);
/* initialize context with content and size of hypertext */
void ht_init_context(_ht_context_t *, void *, unsigned long);
/* destroy (deallocate) context */
void ht_destroy_context(_ht_context_t *);

#endif