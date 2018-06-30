#include <string.h>
#include "context.h"

typedef struct {
	const unsigned char	*bom;
	unsigned char		sz_bom;
	unsigned char		type;
	_read_t			*pf_read;
} _bom_t;

static unsigned int read_utf8(_ht_content_t *p_htc) {
	unsigned int r = 0;
	unsigned long pos = p_htc->c_pos;
	unsigned char *content = p_htc->p_content;

	if(pos < p_htc->sz_content) {
		unsigned char c = *(content + pos);

		r = c;
		pos++;

		while((c & 0xc0) == 0xc0 && pos < p_htc->sz_content) {
			r <<= 8;
			c <<= 1;
			r |= (unsigned char)*(content + pos);
			pos++;
		}

		p_htc->c_pos = pos;
	}

	return r;
}

static unsigned int read_utf16_be(_ht_content_t *p_htc) {
	unsigned int r = 0;
	unsigned short *content = (unsigned short *)p_htc->p_content;
	unsigned long pos = p_htc->c_pos;

	if(pos < (p_htc->sz_content - 2)) {
		unsigned short c = *(content + pos);

		r = (p_htc->machine_order == MACHINE_ORDER_LE) ? __builtin_bswap16(c) : c;
		p_htc->c_pos += 2;
	}

	return r;
}

static unsigned int read_utf16_le(_ht_content_t *p_htc) {
	unsigned int r = 0;
	unsigned short *content = (unsigned short *)p_htc->p_content;
	unsigned long pos = p_htc->c_pos;

	if(pos < (p_htc->sz_content - 2)) {
		unsigned short c = *(content + pos);

		r = (p_htc->machine_order == MACHINE_ORDER_BE) ? __builtin_bswap16(c) : c;
		p_htc->c_pos += 2;
	}

	return r;
}

static unsigned int read_utf32_be(_ht_content_t *p_htc) {
	unsigned int r = 0;
	unsigned int *content = (unsigned int *)p_htc->p_content;
	unsigned long pos = p_htc->c_pos;

	if(pos < (p_htc->sz_content - 4)) {
		unsigned int c = *(content + pos);

		r = (p_htc->machine_order == MACHINE_ORDER_LE) ? __builtin_bswap32(c) : c;
		p_htc->c_pos += 4;
	}

	return r;
}

static unsigned int read_utf32_le(_ht_content_t *p_htc) {
	unsigned int r = 0;
	unsigned int *content = (unsigned int *)p_htc->p_content;
	unsigned long pos = p_htc->c_pos;

	if(pos < (p_htc->sz_content - 4)) {
		unsigned int c = *(content + pos);

		r = (p_htc->machine_order == MACHINE_ORDER_BE) ? __builtin_bswap32(c) : c;
		p_htc->c_pos += 4;
	}

	return r;
}

static _bom_t _g_bom_[] = {
	{ (const unsigned char *)"\xef\xbb\xbf",	3,	E_UTF_8,	read_utf8 },
	{ (const unsigned char *)"\xfe\xff",		2,	E_UTF_16_BE,	read_utf16_be },
	{ (const unsigned char *)"\xff\xfe",		2,	E_UTF_16_LE,	read_utf16_le },
	{ (const unsigned char *)"\x00\x00\xfe\xff",	4,	E_UTF_32_BE,	read_utf32_be },
	{ (const unsigned char *)"\xff\xfe\x00\x00",	4,	E_UTF_32_LE,	read_utf32_le },
	{ NULL,						0,	0,		NULL }
};

/* allocate memory for hypertext context */
_ht_context_t *ht_create_context(_mem_alloc_t *p_alloc, _mem_free_t *p_free) {
	_ht_context_t *r = NULL;

	if(p_alloc) {
		if((r = p_alloc(sizeof(_ht_context_t)))) {
			memset(r, 0, sizeof(_ht_context_t));
			r->pf_mem_alloc = p_alloc;
			r->pf_mem_free = p_free;
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

unsigned long ht_position(_ht_context_t *p_htc) {
	return p_htc->ht_content.c_pos;
}

/* destroy (deallocate) context */
void ht_destroy_context(_ht_context_t *p_htc) {
	if(p_htc->pf_mem_free)
		p_htc->pf_mem_free(p_htc, sizeof(_ht_context_t));
}
