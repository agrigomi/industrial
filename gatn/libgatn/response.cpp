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
			// copy old array to noew one
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

_u32 response::write(void *data, _u32 size) {
	_u32 r = 0;

	//...

	return r;
}

_u32 response::end(_u16 response_code, void *data, _u32 size) {
	write(data, size);
	mpi_httpc->res_code(response_code);
	mpi_httpc->res_content_len(m_content_len);
	return m_content_len;
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
