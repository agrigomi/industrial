#include <string.h>
#include "iGatn.h"
#include "private.h"

#define INITIAL_BMAP_ARRAY	16

bool response::resize_array(void) {
	bool r = false;
	HBUFFER *hb_array = NULL;
	_u32 nhb_count = m_hbcount + INITIAL_BMAP_ARRAY;

	if((hb_array = (HBUFFER *)mpi_heap->alloc(nhb_count * sizeof(HBUFFER)))) {
		// clean new array
		memset(hb_array, 0, nhb_count * sizeof(HBUFFER));
		if(mp_hbarray) {
			// copy old array to new one
			memcpy(hb_array, mp_hbarray, m_hbcount * sizeof(HBUFFER));
			// release old array
			mpi_heap->free(mp_hbarray, (m_hbcount * sizeof(HBUFFER)));
		}
		// replace with new one
		m_hbcount = nhb_count;
		mp_hbarray = hb_array;
		r = true;
	}

	return r;
}

bool response::alloc_buffer(void) {
	bool r = false;

	if(m_buffers == m_hbcount) {
		if(!resize_array())
			return r;
	}

	if((mp_hbarray[m_buffers] = mpi_bmap->alloc())) {
		m_buffers++;
		r = true;
	}

	return r;
}

void response::clear(void) {
	for(_u32 i = 0; i < m_buffers; i++) {
		if(mp_hbarray[i]) {
			mpi_bmap->free(mp_hbarray[i]);
			mp_hbarray[i] = NULL;
		}
	}

	m_content_len = 0;
	m_buffers = 0;
}

_u32 response::capacity(void) {
	return mpi_bmap->size() * m_hbcount;
}

_u32 response::remainder(void) {
	_u32 size = m_buffers * mpi_bmap->size();
	return size - m_content_len;
}

void response::var(_cstr_t name, _cstr_t value) {
	if(mpi_httpc)
		mpi_httpc->res_var(name, value);
}

void response::_var(_cstr_t name, _cstr_t fmt, ...) {
	_char_t lb[1024]="";
	va_list args;

	va_start(args, fmt);
	vsnprintf(lb, sizeof(lb), fmt, args);
	var(name, lb);
	va_end(args);
}

_u32 response::write(void *data, _u32 size) {
	_u32 r = 0;

	while(remainder() < size) {
		if(!alloc_buffer())
			break;
	}

	if(remainder() >= size) {
		_u32 sz = 0;
		_u32 bs = mpi_bmap->size();

		while(r < size) {
			_u32 nbuffer = m_content_len / bs;
			_u32 offset = m_content_len % bs;
			_u32 blen = bs - offset;
			_u32 brem = size - r;
			HBUFFER hb = mp_hbarray[nbuffer];

			if(hb) {
				_u8 *ptr = (_u8 *)mpi_bmap->ptr(hb);
				sz = (brem < blen) ? brem : blen;
				memcpy(ptr + offset, ((_u8 *)data) + r, sz);
				r += sz;
				m_content_len += sz;
			} else
				break;
		}
	}

	return r;
}

_u32 response::write(_cstr_t str) {
	return write((void *)str, strlen(str));
}

_u32 response::_write(_cstr_t fmt, ...) {
	_u32 r = 0;
	_char_t lb[4096]="";
	va_list args;

	va_start(args, fmt);
	r = vsnprintf(lb, sizeof(lb), fmt, args);
	r = write(lb, r);
	va_end(args);

	return r;
}

_u32 response::end(_u16 response_code, void *data, _u32 size) {
	if(data && size)
		write(data, size);
	mpi_httpc->res_code(response_code);
	mpi_httpc->res_content_len(m_content_len);
	return m_content_len;
}

_u32 response::end(_u16 response_code, _cstr_t str) {
	return end(response_code, (void *)str, strlen(str));
}

_u32 response::_end(_u16 response_code, _cstr_t fmt, ...) {
	_u32 r = 0;
	_char_t lb[4096]="";
	va_list args;

	va_start(args, fmt);
	r = vsnprintf(lb, sizeof(lb), fmt, args);
	r = end(response_code, lb, r);
	va_end(args);

	return r;
}

void response::destroy(void) {
	if(mp_hbarray && m_hbcount) {
		for(_u32 i = 0; i < m_hbcount; i++) {
			if(mp_hbarray[i])
				mpi_bmap->free(mp_hbarray[i]);
		}
		mpi_heap->free(mp_hbarray, m_hbcount * sizeof(HBUFFER));
	}
}

void response::process_content(void) {
	_u32 content_len = mpi_httpc->res_content_len();
	_u32 content_sent= mpi_httpc->res_content_sent();
	_u32 bs = mpi_bmap->size();
	_u32 content_buffers = (content_len / bs) + (content_len % bs) ? 1 : 0;
	_u32 nbuffer = content_sent / bs;
	_u32 offset = content_sent % bs;
	_u32 size = bs - offset;

	if(nbuffer == content_buffers - 1)
		// last buffer (adjust size)
		size -= (content_buffers * bs) - content_len;

	if(content_sent < content_len) {
		HBUFFER hb = mp_hbarray[nbuffer];

		if(hb) {
			_u8 *ptr = (_u8 *)mpi_bmap->ptr(hb);
			mpi_httpc->res_write(ptr + offset, size);
		}
	}

	if(nbuffer) {
		// release old buffers
		for(_u32 i = 0; i < nbuffer; i++) {
			if(mp_hbarray[i]) {
				mpi_bmap->free(mp_hbarray[i]);
				mp_hbarray[i] = NULL;
			}
		}
	}
}

void response::redirect(_cstr_t uri) {
	var("Content-Type", "text/html");
	_end(HTTPRC_OK, "<meta http-equiv=\"refresh\" content=\"0; url=%s\"/>", uri);
}

bool response::render(_cstr_t fname) {
	bool r = false;
	_char_t doc[MAX_DOC_ROOT_PATH * 2]="";

	snprintf(doc, sizeof(doc), "%s/%s", m_doc_root, fname);

	HFCACHE fc = mpi_fcache->open(doc);

	if(fc) {
		_ulong doc_sz = 0;
		_u8 *ptr = (_u8 *)mpi_fcache->ptr(fc, &doc_sz);

		if(ptr) { // found in cache
			end(HTTPRC_OK, ptr, (_u32)doc_sz);
			r = true;
		}

		mpi_fcache->close(fc);
	}

	return r;
}
