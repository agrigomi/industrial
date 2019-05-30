#include <string.h>
#include "private.h"

#define INITIAL_BUFFER_ARRAY	16

#define KEY_REQ_URL	"key-req-url"

#define MAX_VAR_LEN	1024

typedef struct {
	_char_t pair[MAX_VAR_LEN];

	_u32 size(void) {
		_u8 key_len = (_u8)strlen(pair) + 1;
		return (key_len + strlen(pair + key_len) + 1);
	}
}_hdr_pair_t;

bool cHttpClientConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			mpi_sio = NULL;
			mpi_map = dynamic_cast <iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));
			mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
			if(mpi_map && mpi_heap) {
				if(mpi_map->init(31))
					r = true;
			}
			break;
		case OCTL_UNINIT: {
			_gpi_repo_->object_release(mpi_sio);
			_gpi_repo_->object_release(mpi_map);

			for(_u32 i = 0; i < m_sz_barray; i++) {
				if(mpp_buffer_array[i])
					mpi_heap->free(mpp_buffer_array[i], m_buffer_size);
			}

			if(mpp_buffer_array && m_sz_barray)
				mpi_heap->free(mpp_buffer_array, m_sz_barray * sizeof(void *));

			if(mp_bheader)
				mpi_heap->free(mp_bheader, m_buffer_size);
			_gpi_repo_->object_release(mpi_heap);
			r = true;
		} break;
	}

	return r;
}

bool cHttpClientConnection::_init(iSocketIO *pi_sio, _u32 buffer_size) {
	bool r = false;

	mpi_sio = pi_sio;
	m_buffer_size = buffer_size;
	mpp_buffer_array = NULL;
	if((mp_bheader = (_char_t *)mpi_heap->alloc(m_buffer_size))) {
		memset(mp_bheader, 0, m_buffer_size);
		r = true;
	}
	m_sz_barray = 0;
	m_req_method = 0;
	m_content_len = 0;
	m_header_len = 0;
	m_res_code = 0;
	return r;
}

void *cHttpClientConnection::alloc_buffer(void) {
	void *r = NULL;
	_u32 i = 0;

_alloc_buffer_:
	for(; i < m_sz_barray; i++) {
		if(mpp_buffer_array[i] == NULL)
			r = mpp_buffer_array[i] = mpi_heap->alloc(m_buffer_size);
	}

	if(!r && i == m_sz_barray) {
		_u32 sz_narray = m_sz_barray + INITIAL_BUFFER_ARRAY;
		void **pp_narray = (void **)mpi_heap->alloc(sz_narray * sizeof(void *));

		if(pp_narray) {
			memset(pp_narray, 0, sz_narray * sizeof(void *));
			if(mpp_buffer_array) {
				memcpy(pp_narray, mpp_buffer_array, m_sz_barray * sizeof(void *));
				mpi_heap->free(mpp_buffer_array, m_sz_barray * sizeof(void *));
			}

			mpp_buffer_array = pp_narray;
			m_sz_barray = sz_narray;
			goto _alloc_buffer_;
		}
	}

	return r;
}

void cHttpClientConnection::reset(void) {
	m_content_len = 0;
	m_header_len = 0;
	m_req_method = 0;
	m_res_code = 0;
	mpi_map->clr();
	if(mp_bheader)
		memset(mp_bheader, 0, m_buffer_size);
}

bool cHttpClientConnection::alive(void) {
	bool r = false;

	if(mpi_sio)
		r = mpi_sio->alive();

	return r;
}

void cHttpClientConnection::req_url(_cstr_t url) {
	req_var(KEY_REQ_URL, url);
}

bool cHttpClientConnection::req_var(_cstr_t name, _cstr_t value) {
	bool r = false;
	_hdr_pair_t hp;
	_u8 key_len = snprintf(hp.pair, sizeof(hp.pair), "%s", name) + 1;

	snprintf(hp.pair + key_len,
		sizeof(hp.pair) - key_len - 1,
		"%s", value);

	if(mpi_map->set(name, strlen(name), &hp, hp.size()))
		r = true;

	return r;
}

void *cHttpClientConnection::calc_buffer(_u32 *sz) {
	void *r = NULL;

	//...

	return r;
}

_u32 cHttpClientConnection::write_buffer(void *data, _u32 size) {
	_u32 r = 0;

	//...

	return r;
}

_u32 cHttpClientConnection::req_write(_u8 *data, _u32 size) {
	_u32 r = write_buffer(data, size);
	m_content_len += r;

	return r;
}

_u32 cHttpClientConnection::req_write(_cstr_t str) {
	return req_write((_u8 *)str, strlen(str));
}

_u32 cHttpClientConnection::_req_write(_cstr_t fmt, ...) {
	_u32 r = 0;
	_char_t lb[4096]="";
	va_list args;

	va_start(args, fmt);
	r = vsnprintf(lb, sizeof(lb), fmt, args);
	r = req_write((_u8 *)lb, r);
	va_end(args);

	return r;
}

bool cHttpClientConnection::send(_u32 timeout_s, _on_http_response_t *p_cb_resp, void *udata) {
	bool r = false;

	//...

	return r;
}

_cstr_t cHttpClientConnection::res_var(_cstr_t name, _u32 *sz) {
	_cstr_t r = NULL;
	_u32 _sz = 0;
	_hdr_pair_t *p_hp = (_hdr_pair_t *)mpi_map->get(name, strlen(name), &_sz);

	if(p_hp) {
		_u32 key_len = strlen(p_hp->pair) + 1;

		r = p_hp->pair + key_len;
	}

	return r;
}

void cHttpClientConnection::res_content(_on_http_response_t *p_cb_resp, void *udata) {
	_u32 content = 0;

	for(_u32 i = 0; i < m_sz_barray; i++) {
		_u32 rem_sz = m_content_len - content;

		if(rem_sz) {
			void *buffer = mpp_buffer_array[i];

			if(buffer) {
				_u32 sz = (m_buffer_size < rem_sz) ? m_buffer_size : rem_sz;

				p_cb_resp(buffer, sz, udata);
				content += sz;
			} else
				break;
		} else
			break;
	}
}
