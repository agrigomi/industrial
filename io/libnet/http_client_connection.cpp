#include <string.h>
#include <unistd.h>
#include "private.h"

#define INITIAL_BUFFER_ARRAY	16
#define MAX_VAR_LEN		4096
#define MAX_KEY_LEN		256

#define KEY_URL		"KEY-URL"
#define KEY_PROTOCOL	"KEY-PROTOCOL"
#define KEY_RES_TEXT	"KEY-RES-TEXT"
#define KEY_RES_CODE	"KEY-RES-CODE"
#define KEY_CONTENT_LEN "Content-Length"
#define KEY_RES_HEADER	"KEY-RES-HEADER"

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
			mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));
			if(mpi_map && mpi_heap && mpi_str) {
				if(mpi_map->init(31))
					r = true;
			}
			break;
		case OCTL_UNINIT: {
			_gpi_repo_->object_release(mpi_sio, false);
			_gpi_repo_->object_release(mpi_map, false);
			_gpi_repo_->object_release(mpi_str);

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
		if(mpp_buffer_array[i] == NULL) {
			if((r = mpp_buffer_array[i] = mpi_heap->alloc(m_buffer_size)))
				break;
			else
				return r;
		}
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
	req_var(KEY_URL, url);
}

void cHttpClientConnection::req_protocol(_cstr_t protocol) {
	req_var(KEY_PROTOCOL, protocol);
}

bool cHttpClientConnection::add_var(_cstr_t vname, _u32 sz_vname, _cstr_t vvalue, _u32 sz_vvalue) {
	bool r = false;
	_hdr_pair_t hp;
	_char_t key[MAX_KEY_LEN]="";

	strncpy(key, vname, (sz_vname < MAX_KEY_LEN -1) ? sz_vname : MAX_KEY_LEN -1);
	mpi_str->toupper(key);

	memset(&hp, 0, sizeof(_hdr_pair_t));
	memcpy(hp.pair, vname, (sz_vname < MAX_KEY_LEN-1) ? sz_vname : MAX_KEY_LEN-1);
	memcpy(hp.pair + sz_vname + 1, vvalue, (sz_vvalue < (sizeof(hp.pair)-(sz_vname+1))) ? sz_vvalue : (sizeof(hp.pair)-(sz_vname+1)));

	if(mpi_map->set(key, strlen(key), &hp, hp.size()))
		r = true;

	return r;
}

bool cHttpClientConnection::req_var(_cstr_t name, _cstr_t value) {
	bool r = false;
	_hdr_pair_t hp;
	_char_t key[MAX_KEY_LEN]="";
	_u8 key_len = snprintf(hp.pair, sizeof(hp.pair), "%s", name) + 1;

	strncpy(key, name, sizeof(key)-1);
	mpi_str->toupper(key);

	snprintf(hp.pair + key_len,
		sizeof(hp.pair) - key_len - 1,
		"%s", value);

	if(mpi_map->set(key, strlen(key), &hp, hp.size()))
		r = true;

	return r;
}

void *cHttpClientConnection::calc_buffer(_u32 *sz) {
	void *r = NULL;
	_u32 bi = m_content_len / m_buffer_size; // buffer index
	_u32 bo = m_content_len % m_buffer_size; // buffer offset

	if(bi < m_sz_barray) {
		if((r = mpp_buffer_array[bi])) {
			r = (_u8 *)r + bo;
			*sz = m_buffer_size - bo;
		}
	}

	return r;
}

_u32 cHttpClientConnection::write_buffer(void *data, _u32 size) {
	_u32 r = 0;

	while(r < size) {
		_u32 sz = 0;
		void *buffer = calc_buffer(&sz);

		if(buffer) {
			_u32 _sz = ((size - r) < sz) ? (size - r) : sz;

			memcpy(buffer, (_u8 *)data + r, _sz);
			r += _sz;
		} else {
			if(!alloc_buffer())
				break;
		}
	}

	m_content_len += r;

	return r;
}

_u32 cHttpClientConnection::req_write(_u8 *data, _u32 size) {
	return write_buffer(data, size);
}

_u32 cHttpClientConnection::req_write(_cstr_t str) {
	return write_buffer((_str_t)str, strlen(str));
}

_u32 cHttpClientConnection::_req_write(_cstr_t fmt, ...) {
	_u32 r = 0;
	_char_t lb[4096]="";
	va_list args;

	va_start(args, fmt);
	r = vsnprintf(lb, sizeof(lb), fmt, args);
	r = write_buffer(lb, r);
	va_end(args);

	return r;
}

bool cHttpClientConnection::prepare_req_header(void) {
	bool r = false;
	_cstr_t rurl = res_var(KEY_URL);
	_cstr_t rprotocol = res_var(KEY_PROTOCOL);
	static _cstr_t http_1_1 = "HTTP/1.1";
	typedef struct {
		_u8 	im;
		_cstr_t	sm;
	}_http_method_map;

	static _http_method_map _method_map[] = {
		{HTTP_METHOD_GET,	"GET"},
		{HTTP_METHOD_HEAD,	"HEAD"},
		{HTTP_METHOD_POST,	"POST"},
		{HTTP_METHOD_PUT,	"PUT"},
		{HTTP_METHOD_DELETE,	"DELETE"},
		{HTTP_METHOD_CONNECT,	"CONNECT"},
		{HTTP_METHOD_OPTIONS,	"OPTIONS"},
		{HTTP_METHOD_TRACE,	"TRACE"},
		{0,			NULL}
	};

	_cstr_t str_method = NULL;

	_u32 n = 0;
	while(_method_map[n].im) {
		if(m_req_method == _method_map[n].im) {
			str_method = _method_map[n].sm;
			break;
		}
		n++;
	}

	if(!rprotocol)
		rprotocol = http_1_1;

	if(str_method && rurl) {
		m_header_len = snprintf(mp_bheader, m_buffer_size, "%s %s %s\r\n", str_method, rurl, rprotocol);
		mpi_map->del(KEY_URL, strlen(KEY_URL));
		mpi_map->del(KEY_PROTOCOL, strlen(KEY_PROTOCOL));

		_map_enum_t me = mpi_map->enum_open();

		if(me) {
			_u32 sz = 0;

			HMUTEX hm = mpi_map->lock();
			_hdr_pair_t *p_hp = (_hdr_pair_t *)mpi_map->enum_first(me, &sz, hm);

			while(p_hp) {
				m_header_len += snprintf(mp_bheader + m_header_len,
							m_buffer_size - m_header_len,
							"%s: %s\r\n",
							p_hp->pair, p_hp->pair + (strlen(p_hp->pair) + 1));
				p_hp = (_hdr_pair_t *)mpi_map->enum_next(me, &sz, hm);
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(me);
		}

		if(m_content_len)
			m_header_len += snprintf(mp_bheader + m_header_len,
						m_buffer_size - m_header_len,
						"Content-Length: %u\r\n", m_content_len);

		_char_t _peer_ip[32]="";

		mpi_sio->peer_ip(_peer_ip, sizeof(_peer_ip));
		m_header_len += snprintf(mp_bheader + m_header_len, m_buffer_size - m_header_len, "Host: %s\r\n\r\n", _peer_ip);
		r = true;
	}

	return r;
}

_u32 cHttpClientConnection::receive_buffer(void *buffer, time_t now, _u32 timeout_s) {
	_u32 r = 0;
	_u32 n = 0;
	_u32 t = 10;

	while(t && r < m_buffer_size && time(NULL) < (now + timeout_s)) {
		n = mpi_sio->read((_u8 *)buffer + r, m_buffer_size - r);

		if(n) {
			r += n;
			t = 10;
		} else {
			if(alive()) {
				if(r)
					t--;
				usleep(10000);
			} else
				break;
		}
	}

	return r;
}

bool cHttpClientConnection::send(_u32 timeout_s, _on_http_response_t *p_cb_resp, void *udata) {
	bool r = false;

	if(prepare_req_header()) {
		time_t now = time(NULL);

		mpi_sio->blocking(true);

		// send header
		mpi_sio->write(mp_bheader, m_header_len);

		// send content
		_u32 bytes = 0;

		while(bytes < m_content_len) {
			_u32 bi = bytes / m_buffer_size;
			_u32 bo = bytes % m_buffer_size;
			_u32 rem = m_content_len - bytes;

			if(bi >= m_sz_barray)
				break;

			void *buffer = mpp_buffer_array[bi];

			if(buffer) {
				_u32 sz = ((m_buffer_size - bo) < rem) ? (m_buffer_size - bo) : rem;

				bytes += mpi_sio->write((_u8 *)buffer + bo,  sz);
			} else
				break;
		}

		bool complete_header=false, complete_content=false;

		reset();
		bytes = 0;
		mpi_sio->blocking(false);

		// receive header ...
		if((bytes = receive_buffer(mp_bheader, now, timeout_s))) {
			_s32 hdr_end = mpi_str->nfind_string((_str_t)mp_bheader, bytes, "\r\n\r\n");

			if(hdr_end != -1) {
				m_header_len = hdr_end + 4;
				complete_header = true;
			}
		}

		// receive the rest of content
		if(bytes == m_buffer_size) {
			_u32 buffer_idx = 0;

			while(alive() && time(NULL) < (now + timeout_s)) {
				void *buffer = NULL;

				if(mpp_buffer_array)
					buffer = mpp_buffer_array[buffer_idx];
				_u32 n = 0;

				if(!buffer)
					buffer = alloc_buffer();

				if(buffer) {
					if((n = receive_buffer(buffer, now, timeout_s))) {
						bytes += n;
						if(n < m_buffer_size)
							break;
						else
							buffer_idx++;
					} else
						break;
				} else
					break;
			}
		}

		if(parse_response_header()) {
			if(m_content_len) {
				if(m_content_len <= (bytes - m_header_len))
					complete_content = true;
			} else {
				m_content_len = bytes - m_header_len;
				complete_content = true;
			}
		}

		if((r = complete_header && complete_content)) {
			if(p_cb_resp)
				res_content(p_cb_resp, udata);
		}
	}

	return r;
}

_u32 cHttpClientConnection::parse_response_line(void) {
	_u32 r = 0;
	_char_t c = 0;
	_char_t _c = 0;
	_char_t *hdr = (_char_t *)mp_bheader;
	_str_t fld[4] = {hdr, 0, 0, 0};
	_u32 fld_sz[4] = {0, 0, 0, 0};

	for(_u32 i=0, n=0; r < m_header_len && i < 3; r++) {
		c = hdr[r];

		switch(c) {
			case ' ':
				if(c != _c) {
					if(i < 2) {
						fld_sz[i] = n;
						n = 0;
					} else
						n++;
				}
				break;
			case '\r':
				fld_sz[i] = n;
				n = 0;
				break;
			case '\n':
				if(_c == '\r')
					i = 3;
				break;
			default:
				if(_c == ' ') {
					if(i < 2) {
						i++;
						fld[i] =  hdr + r;
						n = 1;
					} else
						n++;
				} else
					n++;
				break;
		}

		_c = c;
	}

	if(fld[0] && fld_sz[0] && fld_sz[0] < m_header_len)
		add_var(KEY_PROTOCOL, strlen(KEY_PROTOCOL), fld[0], fld_sz[0]);
	if(fld[1] && fld_sz[1] && fld_sz[1] < m_header_len)
		add_var(KEY_RES_CODE, strlen(KEY_RES_CODE), fld[1], fld_sz[1]);
	if(fld[2] && fld_sz[2] && fld_sz[2] < m_header_len)
		add_var(KEY_RES_TEXT, strlen(KEY_RES_TEXT), fld[2], fld_sz[2]);

	return r;
}

_u32 cHttpClientConnection::parse_variable_line(_cstr_t var, _u32 sz_max) {
	_u32 r = 0;

	_str_t fld[3] = {(_str_t)var, 0, 0};
	_u32 fld_sz[3] = {0, 0, 0};
	_char_t c = 0;
	_char_t _c = 0;
	bool val = false;

	for(_u32 i=0, n=0; r < sz_max && i < 2; r++) {
		c = var[r];

		switch(c) {
			case ' ':
				if(_c != ':' && i)
					n++;
				break;
			case ':':
				if(!i) {
					fld_sz[i] = n;
					n = 0;
					i++;
				} else
					n++;
				break;
			case '\r':
				if(i)
					fld_sz[i] = n;
				break;
			case '\n':
				if(_c == '\r')
					i = 2;
				break;
			default:
				if(!val && i && _c == ' ') {
					fld[i] = (_str_t)var + r;
					val = true;
					n = 1;
				} else
					n++;
				break;
		}

		_c = c;
	}

	if(fld[0] && fld_sz[0] && fld[1] && fld_sz[1] &&
			fld_sz[0] < sz_max && fld_sz[1] < sz_max)
		add_var(fld[0], fld_sz[0], fld[1], fld_sz[1]);

	return r;
}

bool cHttpClientConnection::parse_response_header(void) {
	bool r = true;
	_u32 offset = parse_response_line();
	_cstr_t hdr = (_cstr_t)mp_bheader;
	_cstr_t res_code = res_var(KEY_RES_CODE);

	if(res_code)
		m_res_code = atoi(res_code);

	while(offset && offset < m_header_len) {
		_u32 n = parse_variable_line(hdr + offset, m_header_len - offset);
		offset = (n) ? (offset + n) : 0;
	}

	if(offset == m_header_len) {
		_cstr_t content_len = res_var(KEY_CONTENT_LEN);

		if(content_len)
			m_content_len = atoi(content_len);

		add_var(KEY_RES_HEADER, strlen(KEY_RES_HEADER), hdr, m_header_len);
	}

	return r;
}

_cstr_t cHttpClientConnection::res_var(_cstr_t name) {
	_cstr_t r = NULL;
	_u32 _sz = 0;
	_char_t key[MAX_KEY_LEN]="";

	strncpy(key, name, sizeof(key) - 1);
	mpi_str->toupper(key);

	_hdr_pair_t *p_hp = (_hdr_pair_t *)mpi_map->get(key, strlen(key), &_sz);

	if(p_hp) {
		_u32 key_len = strlen(p_hp->pair) + 1;

		r = p_hp->pair + key_len;
	}

	return r;
}

void cHttpClientConnection::res_content(_on_http_response_t *p_cb_resp, void *udata) {
	_u32 content = (m_content_len <= (m_buffer_size - m_header_len)) ? m_content_len : m_buffer_size - m_header_len;

	if(content)
		p_cb_resp(mp_bheader + m_header_len, content, udata);

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

static cHttpClientConnection _g_http_client_;
