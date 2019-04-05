#include <string.h>
#include "context.h"

typedef struct {
	const unsigned char	*bom;
	unsigned char		sz_bom;
	unsigned char		type;
	_read_t			*pf_read;
} _bom_t;

static unsigned int read_utf8(_ht_content_t *p_htc, unsigned long *pos) {
	unsigned int r = 0;
	unsigned long cpos = p_htc->c_pos;
	unsigned char *content = p_htc->p_content;

	*pos = cpos;
	if(cpos < p_htc->sz_content) {
		unsigned char c = *(content + cpos);

		r = c;
		cpos++;

		while((c & 0xc0) == 0xc0 && cpos < p_htc->sz_content) {
			r <<= 8;
			c <<= 1;
			r |= (unsigned char)*(content + cpos);
			cpos++;
		}

		p_htc->c_pos = cpos;
	}

	return r;
}

static unsigned int read_utf16_be(_ht_content_t *p_htc, unsigned long *pos) {
	unsigned int r = 0;
	unsigned char *content = p_htc->p_content;
	unsigned long cpos = p_htc->c_pos;

	*pos = cpos;
	if(cpos < (p_htc->sz_content - 2)) {
		unsigned short c = *(unsigned short *)(content + cpos);

		r = (p_htc->machine_order == MACHINE_ORDER_LE) ? __builtin_bswap16(c) : c;
		p_htc->c_pos += 2;
	}

	return r;
}

static unsigned int read_utf16_le(_ht_content_t *p_htc, unsigned long *pos) {
	unsigned int r = 0;
	unsigned char *content = p_htc->p_content;
	unsigned long cpos = p_htc->c_pos;

	*pos = cpos;
	if(cpos < (p_htc->sz_content - 2)) {
		unsigned short c = *(unsigned short *)(content + cpos);

		r = (p_htc->machine_order == MACHINE_ORDER_BE) ? __builtin_bswap16(c) : c;
		p_htc->c_pos += 2;
	}

	return r;
}

static unsigned int read_utf32_be(_ht_content_t *p_htc, unsigned long *pos) {
	unsigned int r = 0;
	unsigned char *content = p_htc->p_content;
	unsigned long cpos = p_htc->c_pos;

	*pos = cpos;
	if(cpos < (p_htc->sz_content - 4)) {
		unsigned int c = *(unsigned int *)(content + cpos);

		r = (p_htc->machine_order == MACHINE_ORDER_LE) ? __builtin_bswap32(c) : c;
		p_htc->c_pos += 4;
	}

	return r;
}

static unsigned int read_utf32_le(_ht_content_t *p_htc, unsigned long *pos) {
	unsigned int r = 0;
	unsigned char *content = p_htc->p_content;
	unsigned long cpos = p_htc->c_pos;

	*pos = cpos;
	if(cpos < (p_htc->sz_content - 4)) {
		unsigned int c = *(unsigned int *)(content + cpos);

		r = (p_htc->machine_order == MACHINE_ORDER_BE) ? __builtin_bswap32(c) : c;
		p_htc->c_pos += 4;
	}

	return r;
}

static _bom_t _g_bom_[] = {
	{ (const unsigned char *)"\xef\xbb\xbf",	3,	E_UTF_8,	read_utf8 },
	{ (const unsigned char *)"\x00\x00\xfe\xff",	4,	E_UTF_32_BE,	read_utf32_be },
	{ (const unsigned char *)"\xff\xfe\x00\x00",	4,	E_UTF_32_LE,	read_utf32_le },
	{ (const unsigned char *)"\xfe\xff",		2,	E_UTF_16_BE,	read_utf16_be },
	{ (const unsigned char *)"\xff\xfe",		2,	E_UTF_16_LE,	read_utf16_le },
	{ NULL,						0,	0,		NULL }
};

/* allocate memory for hypertext context */
_ht_context_t *ht_create_context(_mem_alloc_t *p_alloc, _mem_free_t *p_free, void *udata) {
	_ht_context_t *r = NULL;

	if(p_alloc) {
		if((r = p_alloc(sizeof(_ht_context_t), udata))) {
			memset(r, 0, sizeof(_ht_context_t));
			r->pf_mem_alloc = p_alloc;
			r->pf_mem_free = p_free;
			r->udata = udata;
		}
	}

	return r;
}

/* initialize context with content and size of hypertext */
void ht_init_context(_ht_context_t *p_htc, void *p_content, unsigned long sz_content) {
	p_htc->ht_content.p_content = p_content;
	p_htc->ht_content.sz_content = sz_content;

	/* should be considered in utf8 by default */
	p_htc->pf_read = read_utf8;
	p_htc->ht_content.encoding = E_UTF_8;

	if(p_content && sz_content > 4) {
		int n=0;

		/* try to detect encoding by BOM */
		while(_g_bom_[n].bom) {
			if(memcmp(_g_bom_[n].bom, p_content, _g_bom_[n].sz_bom) == 0) {
				p_htc->pf_read = _g_bom_[n].pf_read;
				p_htc->ht_content.c_pos = _g_bom_[n].sz_bom;
				p_htc->ht_content.encoding = _g_bom_[n].type;
				break;
			}

			n++;
		}

		/* detect machine order */
		volatile short word = (MACHINE_ORDER_BE << 8) | MACHINE_ORDER_LE;
		p_htc->ht_content.machine_order = *((char *)&word);
	}
}

void ht_reset_context(_ht_context_t *p_htc) {
	memset(&p_htc->ht_content, 0, sizeof(_ht_content_t));
	p_htc->pf_read = NULL;
}

unsigned long ht_position(_ht_context_t *p_htc) {
	return p_htc->ht_content.c_pos;
}

unsigned char *ht_ptr(_ht_context_t *p_htc) {
	return p_htc->ht_content.p_content + p_htc->ht_content.c_pos;
}

int ht_compare(_ht_context_t *p_htc, unsigned char *p1, unsigned char *p2, unsigned int size) {
	int r = 0;
	unsigned int i = 0;
	unsigned char en1 = E_UTF_8, en2 = E_UTF_8;
	unsigned int c1 = 0, c2 = 0;

	/* obtain p1 encoding */
	if(p1 >= p_htc->ht_content.p_content && p1 < (p_htc->ht_content.p_content + p_htc->ht_content.sz_content))
		/* p1 belongs to current document */
		en1 = p_htc->ht_content.encoding;
	/* obtain p2 encoding */
	if(p2 >= p_htc->ht_content.p_content && p2 < (p_htc->ht_content.p_content + p_htc->ht_content.sz_content))
		/* p2 belongs to current document */
		en2 = p_htc->ht_content.encoding;

	while(i < size) {
		switch(en1) {
			case E_UTF_8:
				c1 = *(p1 + i);
				break;
			case E_UTF_16_BE:
				c1 = *(((unsigned short *)p1) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
					c1 = __builtin_bswap32(c1);
				break;
			case E_UTF_16_LE:
				c1 = *(((unsigned short *)p1) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
					c1 = __builtin_bswap32(c1);
				break;
			case E_UTF_32_BE:
				c1 = *(((unsigned int *)p1) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
					c1 = __builtin_bswap32(c1);
				break;
			case E_UTF_32_LE:
				c1 = *(((unsigned int *)p1) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
					c1 = __builtin_bswap32(c1);
				break;
		}
		switch(en2) {
			case E_UTF_8:
				c2 = *(p2 + i);
				break;
			case E_UTF_16_BE:
				c2 = *(((unsigned short *)p2) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
					c2 = __builtin_bswap32(c2);
				break;
			case E_UTF_16_LE:
				c2 = *(((unsigned short *)p2) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
					c2 = __builtin_bswap32(c2);
				break;
			case E_UTF_32_BE:
				c2 = *(((unsigned int *)p2) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
					c2 = __builtin_bswap32(c2);
				break;
			case E_UTF_32_LE:
				c2 = *(((unsigned int *)p2) + i);
				if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
					c2 = __builtin_bswap32(c2);
				break;
		}

		if((r = c1 - c2) != 0)
			break;
		i++;
	}

	return r;
}

/* return number of symbols between two pointers */
unsigned int ht_symbols(_ht_context_t *p_htc, unsigned char *p1, unsigned char *p2) {
	unsigned int r = 0;

	if((p1 >= p_htc->ht_content.p_content &&
				p1 < (p_htc->ht_content.p_content + p_htc->ht_content.sz_content)) &&
			(p2 >= p_htc->ht_content.p_content &&
				p2 < (p_htc->ht_content.p_content + p_htc->ht_content.sz_content))) {

		r = (p1 < p2) ? p2 - p1 : p1 - p2;

		switch(p_htc->ht_content.encoding) {
			case E_UTF_16_LE:
			case E_UTF_16_BE:
				r /= 2;
				break;
			case E_UTF_32_LE:
			case E_UTF_32_BE:
				r /= 4;
				break;
		}
	}

	return r;
}

unsigned int ht_bytes(_ht_context_t *p_htc, unsigned int num_symbols) {
	unsigned int r = num_symbols;

	switch(p_htc->ht_content.encoding) {
		case E_UTF_16_LE:
		case E_UTF_16_BE:
			r *= 2;
			break;
		case E_UTF_32_LE:
		case E_UTF_32_BE:
			r *= 4;
			break;
	}

	return r;
}

/* read symbol */
unsigned int ht_read(_ht_context_t *p_htc, /* context */
			unsigned char *p, /* input pointer */
			unsigned char **ppn /* output pointer (to next symbol */
			) {
	unsigned int r = 0;
	unsigned char en = E_UTF_8;

	if(p >= p_htc->ht_content.p_content && p < (p_htc->ht_content.p_content + p_htc->ht_content.sz_content))
		/* p belongs to current document */
		en = p_htc->ht_content.encoding;

	switch(en) {
		case E_UTF_8: {
			unsigned int i = 0;
			unsigned char c = *(p + i);

			r = c;
			i++;

			while((c & 0xc0) == 0xc0) {
				r <<= 8;
				c <<= 1;
				r |= (unsigned char)*(p + i);
				i++;
			}

			*ppn = p + i;
		} break;
		case E_UTF_16_BE:
			r = *(unsigned short *)p;
			if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
				r =  __builtin_bswap32(r);
			*ppn = p + 2;
			break;
		case E_UTF_16_LE:
			r = *(unsigned short *)p;
			if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
				r =  __builtin_bswap32(r);
			*ppn = p + 2;
			break;
		case E_UTF_32_LE:
			r = *(unsigned int *)p;
			if(p_htc->ht_content.machine_order == MACHINE_ORDER_BE)
				r =  __builtin_bswap32(r);
			*ppn = p + 4;
			break;
		case E_UTF_32_BE:
			r = *(unsigned int *)p;
			if(p_htc->ht_content.machine_order == MACHINE_ORDER_LE)
				r =  __builtin_bswap32(r);
			*ppn = p + 4;
			break;
	}

	return r;
}

/* destroy (deallocate) context */
void ht_destroy_context(_ht_context_t *p_htc) {
	if(p_htc->pf_mem_free)
		p_htc->pf_mem_free(p_htc, sizeof(_ht_context_t), p_htc->udata);
}
