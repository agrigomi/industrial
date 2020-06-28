#include <string.h>
#include <unistd.h>
#include "private.h"
#include "time.h"
#include "url-codec.h"

// http connection status
enum httpc_state {
	HTTPC_RECEIVE_HEADER =	1,
	HTTPC_COMPLETE_HEADER,
	HTTPC_PARSE_HEADER,
	HTTPC_RECEIVE_CONTENT,
	HTTPC_SEND_HEADER,
	HTTPC_SEND_CONTENT,
	HTTPC_CLOSE
};

typedef struct {
	_u16 	rc;
	_cstr_t	text;
}_http_resp_text_t;

static _http_resp_text_t _g_http_resp_text[] = {
	{HTTPRC_CONTINUE,		"Continue"},
	{HTTPRC_SWITCHING_PROTOCOL,	"Switching Protocols"},
	{HTTPRC_OK,			"OK"},
	{HTTPRC_CREATED,		"Created"},
	{HTTPRC_ACCEPTED,		"Accepted"},
	{HTTPRC_NON_AUTH,		"Non-Authoritative Information"},
	{HTTPRC_NO_CONTENT,		"No Content"},
	{HTTPRC_RESET_CONTENT,		"Reset Content"},
	{HTTPRC_PART_CONTENT,		"Partial Content"},
	{HTTPRC_MULTICHOICES,		"Multiple Choices"},
	{HTTPRC_MOVED_PERMANENTLY,	"Moved Permanently"},
	{HTTPRC_FOUND,			"Found"},
	{HTTPRC_SEE_OTHER,		"See Other"},
	{HTTPRC_NOT_MODIFIED,		"Not Modified"},
	{HTTPRC_USE_PROXY,		"Use proxy"},
	{HTTPRC_TEMP_REDIRECT,		"Temporary redirect"},
	{HTTPRC_BAD_REQUEST,		"Bad Request"},
	{HTTPRC_UNAUTHORIZED,		"Unauthorized"},
	{HTTPRC_PAYMENT_REQUIRED,	"Payment Required"},
	{HTTPRC_FORBIDDEN,		"Forbidden"},
	{HTTPRC_NOT_FOUND,		"Not Found"},
	{HTTPRC_METHOD_NOT_ALLOWED,	"Method Not Allowed"},
	{HTTPRC_NOT_ACCEPTABLE,		"Not Acceptable"},
	{HTTPRC_PROXY_AUTH_REQUIRED,	"Proxy Authentication Required"},
	{HTTPRC_REQUEST_TIMEOUT,	"Request Time-out"},
	{HTTPRC_CONFLICT,		"Conflict"},
	{HTTPRC_GONE,			"Gone"},
	{HTTPRC_LENGTH_REQUIRED,	"Length Required"},
	{HTTPRC_PRECONDITION_FAILED,	"Precondition Failed"},
	{HTTPRC_REQ_ENTITY_TOO_LARGE,	"Request Entity Too Large"},
	{HTTPRC_REQ_URI_TOO_LARGE,	"Request-URI Too Large"},
	{HTTPRC_UNSUPPORTED_MEDIA_TYPE,	"Unsupported Media Type"},
	{HTTPRC_EXPECTATION_FAILED,	"Expectation Failed"},
	{HTTPRC_INTERNAL_SERVER_ERROR,	"Internal Server Error"},
	{HTTPRC_NOT_IMPLEMENTED,	"Not Implemented"},
	{HTTPRC_BAD_GATEWAY,		"Bad Gateway"},
	{HTTPRC_SERVICE_UNAVAILABLE,	"Service Unavailable"},
	{HTTPRC_GATEWAY_TIMEOUT,	"Gateway Time-out"},
	{HTTPRC_VERSION_NOT_SUPPORTED,	"HTTP Version not supported"},
	{0,				NULL}
};

typedef struct {
	_u8 	im;
	_cstr_t	sm;
}_http_method_map;

static _http_method_map _g_method_map[] = {
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

#define USE_CONNECTION_TIMEOUT

static iStr *gpi_str = 0;
static HOBJECT g_hmap = 0;
static HOBJECT g_hlist = 0;

bool cHttpServerConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			mp_sio = NULL;
			mpi_bmap = NULL;
			memset(m_udata, 0, sizeof(m_udata));
			m_ibuffer = m_oheader = m_obuffer = 0;
			if(!gpi_str)
				gpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL);
			if(g_hmap)
				mpi_req_map = (iMap *)pi_repo->object_by_handle(g_hmap, RF_CLONE | RF_NONOTIFY);
			else {
				if((g_hmap = pi_repo->handle_by_iname(I_MAP))) {
					mpi_req_map = (iMap *)pi_repo->object_by_handle(g_hmap, RF_CLONE | RF_NONOTIFY);
				}
			}
			if(!g_hlist)
				g_hlist = pi_repo->handle_by_iname(I_LLIST);
			if(g_hlist)
				mpi_cookie_list = (iLlist *)pi_repo->object_by_handle(g_hlist, RF_CLONE | RF_NONOTIFY);

			if(gpi_str && mpi_req_map && mpi_cookie_list) {
				r = mpi_req_map->init(31);
				r &= mpi_cookie_list->init(LL_VECTOR, 1);
				clean_members();
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			close();
			pi_repo->object_release(gpi_str);
			pi_repo->object_release(mpi_req_map);
			pi_repo->object_release(mpi_cookie_list);
			r = true;
		} break;
	}

	return r;
}

void cHttpServerConnection::clean_members(void) {
	m_state = 0;
	release_buffers();
	m_ibuffer_offset = m_oheader_offset = m_obuffer_offset = 0;
	m_response_code = 0;
	m_error_code = 0;
	m_res_content_len = 0;
	m_req_content_len = 0;
	m_req_content_rcv = 0;
	m_oheader_sent = 0;
	m_obuffer_sent = 0;
	m_content_sent = 0;
	m_header_len = 0;
	m_content_type = 0;
	mp_doc = 0;
	m_req_data = false;
	m_stime = time(NULL);
	strncpy(m_res_protocol, "HTTP/1.1", sizeof(m_res_protocol)-1);
	mpi_req_map->clr();
	mpi_cookie_list->clr();
}

bool cHttpServerConnection::_init(cSocketIO *p_sio, iBufferMap *pi_bmap, _u32 timeout) {
	bool r = false;

	if(!mp_sio && p_sio && (r = p_sio->alive())) {
		clean_members();
		mp_sio = p_sio;
		mpi_bmap = pi_bmap;
		m_timeout = timeout;
		// use non blocking mode
		mp_sio->blocking(false);
	}

	return r;
}

void cHttpServerConnection::close(void) {
	if(mp_sio) {
		_gpi_repo_->object_release(mp_sio);
		mp_sio = NULL;
	}

	release_buffers();
}

void cHttpServerConnection::release_buffers(void) {
	if(m_ibuffer) {
		mpi_bmap->free(m_ibuffer);
		m_ibuffer = 0;
	}
	if(m_oheader) {
		mpi_bmap->free(m_oheader);
		m_oheader = 0;
	}
	if(m_obuffer) {
		mpi_bmap->free(m_obuffer);
		m_obuffer = 0;
	}
}

bool cHttpServerConnection::alive(void) {
	bool r = false;

	if(mp_sio)
		r = mp_sio->alive();

	return r;
}

_u32 cHttpServerConnection::peer_ip(void) {
	_u32 r = 0;

	if(mp_sio)
		r = mp_sio->peer_ip();

	return r;
}

bool cHttpServerConnection::peer_ip(_str_t strip, _u32 len) {
	bool r = false;

	if(mp_sio)
		r = mp_sio->peer_ip(strip, len);

	return r;
}


_cstr_t cHttpServerConnection::get_rc_text(_u16 rc) {
	_cstr_t r = "...";
	_u32 n = 0;

	while(_g_http_resp_text[n].rc) {
		if(rc == _g_http_resp_text[n].rc) {
			r = _g_http_resp_text[n].text;
			break;
		}
		n++;
	}

	return r;
}

_u32 cHttpServerConnection::receive(void) {
	_u32 r = 0;

	if(alive()) {
		if(!m_ibuffer)
			m_ibuffer = mpi_bmap->alloc();

		if(m_ibuffer) {
			_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_ibuffer);
			_u32 sz_buffer = mpi_bmap->size();

			if(ptr) {
				r = mp_sio->read(ptr + m_ibuffer_offset, sz_buffer - m_ibuffer_offset);
				m_ibuffer_offset += r;
			}
		}
	}

	return r;
}

bool cHttpServerConnection::complete_req_header(void) {
	bool r = false;

	if(m_ibuffer_offset) {
		_u32 sz = mpi_bmap->size();

		_str_t ptr = (_str_t)mpi_bmap->ptr(m_ibuffer);

		if(ptr) {
			_s32 hl = 0;

			// !!! dangerous !!!
			if((hl = gpi_str->nfind_string(ptr, sz, "\r\n\r\n")) != -1) {
				r = true;
				add_req_variable(VAR_REQ_HEADER, ptr, hl);
				m_header_len = hl + 4;
			} else
				m_header_len = 0;
		}
	}

	return r;
}

bool cHttpServerConnection::add_req_variable(_cstr_t name, _cstr_t value, _u32 sz_value) {
	bool r = false;

	if(mpi_req_map->add(name, gpi_str->str_len(name),
			value,
			(sz_value) ? sz_value : gpi_str->str_len(value)))
		r = true;

	return r;
}

_u32 cHttpServerConnection::parse_url(_str_t url, _u32 sz_max) {
	_u32 r = 0;
	HBUFFER hburl = mpi_bmap->alloc();

	if(hburl) {
		_str_t decoded = (_str_t)mpi_bmap->ptr(hburl);
		_u32 sz_decoded = UrlDecode((_cstr_t)url, sz_max, decoded, mpi_bmap->size());
		_u32 i = 0; // buffer index
		_u32 sz = 0; // common size of URL or variable (name=value)
		_str_t name = NULL, value = NULL;
		_u32 sz_name = 0, sz_value = 0;

		add_req_variable(VAR_REQ_URI, decoded, sz_decoded);

		// determine URL part
		for(; i < sz_decoded; i++) {
			if(*(decoded + i) == '?')
				break;
			sz++;
		}

		add_req_variable(VAR_REQ_URL, decoded, sz);

		if(i < sz_decoded) {
			// parse variables
			sz = 0;
			i++; // skip '?'

			add_req_variable(VAR_REQ_URN, decoded + i, sz_decoded - i);

			for(; i < sz_decoded; i++) {
				switch(*(decoded + i)) {
					case '=': // end of name
						value = decoded + i + 1;
						break;
					case '&': // end of variable
						if(name && sz_name && value && sz_value)
							mpi_req_map->add(name, sz_name, value, sz_value);
						name = value = NULL;
						sz_name = sz_value = 0;
						break;
					default:
						if(value)
							sz_value++;
						else if(name)
							sz_name++;
						else {
							name = decoded + i;
							sz_name = 1;
						}
						break;
				}
			}

			if(name && sz_name && value && sz_value)
				mpi_req_map->add(name, sz_name, value, sz_value);
		}

		mpi_bmap->free(hburl);
	}

	return r;
}

_u32 cHttpServerConnection::parse_request_line(_str_t req, _u32 sz_max) {
	_u32 r = 0;
	_char_t c = 0;
	_char_t _c = 0;
	_str_t fld[4] = {req, 0, 0, 0};
	_u32 fld_sz[4] = {0, 0, 0, 0};

	for(_u32 i=0, n=0; r < sz_max && i < 3; r++) {
		c = req[r];

		switch(c) {
			case ' ':
				if(c != _c) {
					fld_sz[i] = n;
					n = 0;
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
					i++;
					fld[i] = req + r;
					n = 1;
				} else
					n++;
				break;
		}

		_c = c;
	}

	if(fld[0] && fld_sz[0] && fld_sz[0] < sz_max)
		add_req_variable(VAR_REQ_METHOD, fld[0], fld_sz[0]);
	if(fld[1] && fld_sz[1] && fld_sz[1] < sz_max)
		parse_url(fld[1], fld_sz[1]);
	if(fld[2] && fld_sz[2] && fld_sz[2] < sz_max)
		add_req_variable(VAR_REQ_PROTOCOL, fld[2], fld_sz[2]);

	return r;
}

_u32 cHttpServerConnection::parse_var_line(_str_t var, _u32 sz_max) {
	_u32 r = 0;
	_str_t fld[3] = {var, 0, 0};
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
					fld[i] = var + r;
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
		mpi_req_map->add(fld[0], fld_sz[0], fld[1], fld_sz[1]);

	return r;
}

bool cHttpServerConnection::parse_req_header(void) {
	bool r = false;

	if(m_ibuffer && m_ibuffer_offset) {
		_str_t hdr = (_str_t)mpi_bmap->ptr(m_ibuffer);

		if(hdr) {
			_u32 offset = parse_request_line(hdr, m_header_len);

			while(offset && offset < m_header_len) {
				_u32 n = parse_var_line(hdr + offset, m_header_len - offset);
				offset = (n) ? (offset + n) : 0;
			}

			if(offset == m_header_len) {
				r = true;
				if(m_ibuffer_offset > m_header_len) {
					// have request data
					gpi_str->mem_cpy(hdr, hdr + m_header_len, m_ibuffer_offset - m_header_len);
					m_ibuffer_offset -= m_header_len;
					hdr[m_ibuffer_offset] = 0; // terminate content
					m_req_data = true;
				} else {
					m_ibuffer_offset = 0;
					m_req_data = false;
				}

				m_header_len = 0;

				_cstr_t cl = req_var("Content-Length");
				if(cl)
					m_req_content_len = atoi(cl);
				m_req_content_rcv = m_ibuffer_offset;
			}
		}
	}

	return r;
}

bool cHttpServerConnection::req_parse_content(void) {
	bool r = false;
	_u32 sz = 0;
	_cstr_t data = (_cstr_t)req_data(&sz);

	if(data && sz) {
		HBUFFER hbcontent = mpi_bmap->alloc();

		if(hbcontent) {
			_str_t decoded = (_str_t)mpi_bmap->ptr(hbcontent);
			_u32 sz_decoded = UrlDecode(data, sz, decoded, mpi_bmap->size());
			_u32 i = 0;
			_str_t name = NULL, value = NULL;
			_u32 sz_name = 0, sz_value = 0;

			for(; i < sz_decoded; i++) {
				switch(*(decoded + i)) {
					case '=':
						value = decoded + i + 1;
						break;
					case '&':
						if(name && sz_name && value && sz_value)
							mpi_req_map->add(name, sz_name, value, sz_value);
						name = value = NULL;
						sz_name = sz_value = 0;
						break;
					default:
						if(value)
							sz_value++;
						else if(name)
							sz_name++;
						else {
							name = decoded + i;
							sz_name = 1;
						}
						break;
				}
			}

			if(name && sz_name && value && sz_value)
				mpi_req_map->add(name, sz_name, value, sz_value);

			mpi_bmap->free(hbcontent);
			r = true;
		}
	}

	return r;
}

_u32 cHttpServerConnection::res_remainder(void) {
	_u32 r = 0;

	if(m_content_sent < m_res_content_len)
		r = m_res_content_len - m_content_sent;

	return r;
}

void cHttpServerConnection::clear_ibuffer(void) {
	if(m_ibuffer) {
		_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_ibuffer);

		if(ptr) {
			_u32 sz = mpi_bmap->size();

			memset(ptr, 0, sz);
			m_ibuffer_offset = 0;
		}
	}
}

_u32 cHttpServerConnection::send_header(void) {
	_u32 r = 0;
	_char_t rs[128]="";

	if(m_response_code) {
		// Add content type to response header
		if(m_content_type)
			res_var("Content-Type", m_content_type);

		// Add content length to response header
		if(m_res_content_len) {
			_char_t cl[32]="";

			sprintf(cl, "%u", m_res_content_len);
			res_var("Content-Length", cl);
		}

		// Add cookies to response header
		if(mpi_cookie_list->cnt()) {
			_u32 sz = 0;
			_cstr_t cookie = (_cstr_t)mpi_cookie_list->first(&sz);

			while(cookie) {
				res_var("Set-Cookie", cookie);
				cookie = (_cstr_t)mpi_cookie_list->next(&sz);
			}

			mpi_cookie_list->clr();
		}

		mp_sio->blocking(true);
		if(!m_oheader_sent) {
			// send first line
			_u32 n = snprintf(rs, sizeof(rs), "%s %u %s\r\n",
					m_res_protocol,
					m_response_code,
					get_rc_text(m_response_code));

			mp_sio->write(rs, n);
		}

		if(m_oheader_offset) {
			// send header
			_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_oheader);

			if(ptr) {
				r = mp_sio->write(ptr + m_oheader_sent,
						m_oheader_offset - m_oheader_sent);
				m_oheader_sent += r;
			}
		}

		if(m_oheader_sent == m_oheader_offset) {
			// send header end
			mp_sio->write("\r\n", 2);
			m_oheader_sent = r;
		}
		mp_sio->blocking(false);
	}

	return r;
}

_u32 cHttpServerConnection::receive_content(void) {
	_u32 r = receive();

	m_req_content_rcv += r;

	return r;
}

_u32 cHttpServerConnection::send_content(void) {
	_u32 r = 0;

	if(mp_doc) {
		if(m_res_content_len && m_content_sent < m_res_content_len) {
			_u8 *ptr = (_u8 *)mp_doc + m_content_sent;
			_u32 len = m_res_content_len - m_content_sent;

			mp_sio->blocking(true);
			r = mp_sio->write(ptr, len);
			m_content_sent += r;
			mp_sio->blocking(false);
		}
	} else {
		if(m_res_content_len && m_content_sent < m_res_content_len) {
			_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_obuffer);
			if(ptr && m_obuffer_offset) {
				mp_sio->blocking(true);
				r = mp_sio->write(ptr + m_obuffer_sent, m_obuffer_offset - m_obuffer_sent);
				m_obuffer_sent += r;
				m_content_sent += r;
				mp_sio->blocking(false);
			}
		}
	}

	return r;
}

_u8 cHttpServerConnection::process(void) {
	_u8 r = 0;

	switch(m_state) {
		case 0:
			r = HTTP_ON_OPEN;
			m_state = HTTPC_RECEIVE_HEADER;
			break;
		case HTTPC_RECEIVE_HEADER:
			if(!receive()) {
				if(alive())
					m_state = HTTPC_COMPLETE_HEADER;
				else
					m_state = HTTPC_CLOSE;
			}
			break;
		case HTTPC_COMPLETE_HEADER:
			if(complete_req_header())
				m_state = HTTPC_PARSE_HEADER;
			else {
#ifdef USE_CONNECTION_TIMEOUT
				if((time(NULL) - m_stime) > m_timeout) {
					m_state = HTTPC_SEND_HEADER;
					m_error_code = HTTPRC_REQUEST_TIMEOUT;
					r = HTTP_ON_ERROR;
				} else
#endif
					m_state = HTTPC_RECEIVE_HEADER;
			}
			break;
		case HTTPC_PARSE_HEADER:
			if(parse_req_header()) {
				if(req_url() && req_method()) {
					r = HTTP_ON_REQUEST;
					m_state = HTTPC_RECEIVE_CONTENT;
				} else {
					r = HTTP_ON_ERROR;
					m_error_code = HTTPRC_BAD_REQUEST;
					m_state = HTTPC_SEND_HEADER;
				}
			} else {
				r = HTTP_ON_ERROR;
				m_error_code = HTTPRC_BAD_REQUEST;
				m_state = HTTPC_SEND_HEADER;
			}
			break;
		case HTTPC_RECEIVE_CONTENT:
			clear_ibuffer();
			if(receive_content()) {
				r = HTTP_ON_REQUEST_DATA;
				m_req_data = true;
			} else {
				if(alive()) {
					if(m_req_content_rcv >= m_req_content_len)
						m_state = HTTPC_SEND_HEADER;
				} else
					m_state = HTTPC_CLOSE;
				m_req_data = false;
			}
			break;
		case HTTPC_SEND_HEADER:
			clear_ibuffer();
			if(!receive_content()) {
				if(alive() && m_response_code) {
					send_header();
					if(m_oheader_sent == m_oheader_offset)
						m_state = HTTPC_SEND_CONTENT;
				} else
					m_state = HTTPC_CLOSE;
				m_req_data = false;
			} else {
				m_req_data = true;
				r = HTTP_ON_REQUEST_DATA;
			}
			break;
		case HTTPC_SEND_CONTENT:
			clear_ibuffer();
			if(receive_content()) {
				r = HTTP_ON_REQUEST_DATA;
				m_req_data = true;
			} else {
				if(alive()) {
					send_content();
					if(m_content_sent < m_res_content_len) {
						if(!mp_doc) {
							if(m_obuffer_sent >= m_obuffer_offset) {
								r = HTTP_ON_RESPONSE_DATA;
								m_obuffer_sent = m_obuffer_offset = 0;
							}
						}
					} else {
						_cstr_t ctype = req_var("Connection");

						if(ctype && strcasecmp(ctype, "keep-alive") == 0) { // reuse connection
							clean_members();
							m_state = HTTPC_RECEIVE_HEADER;
							r = HTTP_ON_CLOSE_DOCUMENT;
						} else
							m_state = HTTPC_CLOSE;
					}
				} else
					m_state = HTTPC_CLOSE;
				m_req_data = false;
			}
			break;
		case HTTPC_CLOSE:
			close();
			break;
	}

	return r;
}

_u32 cHttpServerConnection::res_write(_u8 *data, _u32 size) {
	_u32 r = 0;

	if(!m_obuffer)
		m_obuffer = mpi_bmap->alloc();

	if(m_obuffer) {
		_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_obuffer);

		if(ptr) {
			_u32 bsz = mpi_bmap->size();
			_u32 brem = bsz - m_obuffer_offset;

			if((r = (size < brem ) ? size : brem)) {
				gpi_str->mem_cpy(ptr + m_obuffer_offset, data, r);
				m_obuffer_offset += r;
			}
		}
	}

	return r;
}

_u32 cHttpServerConnection::res_write(_cstr_t str) {
	return res_write((_u8 *)str, strlen(str));
}

_u8 cHttpServerConnection::req_method(void) {
	_u8 r = 0;
	_cstr_t sm = req_var(VAR_REQ_METHOD);

	if(sm) {
		_u32 n = 0;

		while(_g_method_map[n].sm) {
			if(strcmp(sm, _g_method_map[n].sm) == 0) {
				r = _g_method_map[n].im;
				break;
			}

			n++;
		}
	}

	return r;
}

_cstr_t cHttpServerConnection::req_header(void) {
	return req_var(VAR_REQ_HEADER);
}

_cstr_t cHttpServerConnection::req_uri(void) {
	return req_var(VAR_REQ_URI);
}

_cstr_t cHttpServerConnection::req_url(void) {
	return req_var(VAR_REQ_URL);
}

_cstr_t cHttpServerConnection::req_urn(void) {
	return req_var(VAR_REQ_URN);
}

_cstr_t cHttpServerConnection::req_var(_cstr_t name) {
	_u32 sz = 0;
	_str_t vn = (_str_t)name;
	return (_str_t)mpi_req_map->get(vn, strlen(vn), &sz);
}

_u8 *cHttpServerConnection::req_data(_u32 *size) {
	_u8 *r = 0;

	if(m_ibuffer && m_req_data) {
		if((r = (_u8 *)mpi_bmap->ptr(m_ibuffer)))
			*size = m_ibuffer_offset;
	}

	return r;
}

_cstr_t cHttpServerConnection::req_protocol(void) {
	return req_var(VAR_REQ_PROTOCOL);
}

bool cHttpServerConnection::res_var(_cstr_t name, _cstr_t value) {
	bool r = false;

	if(!m_oheader)
		m_oheader = mpi_bmap->alloc();
	if(m_oheader) {
		_char_t *ptr = (_char_t *)mpi_bmap->ptr(m_oheader);

		if(ptr) {
			_u32 sz = mpi_bmap->size();
			_u32 sz_data = strlen(name) + strlen(value) + 4;
			_u32 rem = (sz > m_oheader_offset) ? (sz - m_oheader_offset) : 0;

			if(rem > sz_data) {
				m_oheader_offset += snprintf(ptr + m_oheader_offset, rem, "%s: %s\r\n", name, value);
				r = true;
			}
		}
	}

	return r;
}

 void cHttpServerConnection::res_protocol(_cstr_t protocol) {
	strncpy(m_res_protocol, protocol, sizeof(m_res_protocol)-1);
 }

bool cHttpServerConnection::res_content_len(_u32 content_len) {
	m_res_content_len = content_len;
	return true;
}

void cHttpServerConnection::res_cookie(_cstr_t name,
		_cstr_t value,
		_u8 flags,
		_cstr_t expires,
		_cstr_t max_age,
		_cstr_t path,
		_cstr_t domain) {
	_char_t val[4096]="";
	_u32 n = 0;

	n += snprintf(val + n, sizeof(val) - n, "%s=%s", name, value);
	if(flags & CF_SECURE)
		n += snprintf(val + n, sizeof(val) - n, "; %s", "Secure");
	if(flags & CF_HTTP_ONLY)
		n += snprintf(val + n, sizeof(val) - n, "; %s", "HttpOnly");
	if(flags & CF_SAMESITE_STRICT)
		n += snprintf(val + n, sizeof(val) - n, "; %s", "SameSite=Strict");
	if(flags & CF_SAMESITE_LAX)
		n += snprintf(val + n, sizeof(val) - n, "; %s", "SameSite=Lax");
	if(expires)
		n += snprintf(val + n, sizeof(val) - n, "; Expires=%s", expires);
	if(max_age)
		n += snprintf(val + n, sizeof(val) - n, "; Max-Age=%s", max_age);
	if(path)
		n += snprintf(val + n, sizeof(val) - n, "; Path=%s", path);
	if(domain)
		n += snprintf(val + n, sizeof(val) - n, "; Domain=%s", domain);

	mpi_cookie_list->add(val, n + 1);
}

void cHttpServerConnection::res_mtime(time_t mtime) {
	_char_t value[128]="";
	tm *_tm = gmtime(&mtime);
	strftime(value, sizeof(value), "%a, %d %b %Y %H:%M:%S GMT", _tm);

	res_var("Last-Modified", value);
}

static cHttpServerConnection _g_httpc_;
