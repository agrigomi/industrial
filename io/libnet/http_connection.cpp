#include "private.h"
#include "time.h"

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

bool cHttpConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			mp_sio = NULL;
			mpi_bmap = NULL;
			m_state = 0;
			m_ibuffer = m_oheader = m_obuffer = 0;
			m_ibuffer_offset = m_oheader_offset = m_obuffer_offset = m_obuffer_sent = 0;
			m_response_code = 0;
			m_content_len = 0;
			m_content_sent = 0;
			m_udata = 0;
			mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL);
			mpi_map = (iMap *)pi_repo->object_by_iname(I_MAP, RF_CLONE);
			if(mpi_str && mpi_map)
				r = true;
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			close();
			pi_repo->object_release(mpi_str);
			pi_repo->object_release(mpi_map);
			r = true;
		} break;
	}

	return r;
}

bool cHttpConnection::_init(cSocketIO *p_sio, iBufferMap *pi_bmap) {
	bool r = false;

	if(!mp_sio && p_sio && (r = p_sio->alive())) {
		mp_sio = p_sio;
		mpi_bmap = pi_bmap;
		// use non blocking mode
		mp_sio->blocking(false);
	}

	return r;
}

void cHttpConnection::close(void) {
	if(mp_sio) {
		mp_sio->_close();
		mp_sio = 0;
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
}

bool cHttpConnection::alive(void) {
	bool r = false;

	if(mp_sio)
		r = mp_sio->alive();

	return r;
}

_u32 cHttpConnection::peer_ip(void) {
	_u32 r = 0;

	if(mp_sio)
		r = mp_sio->peer_ip();

	return r;
}

bool cHttpConnection::peer_ip(_str_t strip, _u32 len) {
	bool r = false;

	if(mp_sio)
		r = mp_sio->peer_ip(strip, len);

	return r;
}


_cstr_t cHttpConnection::get_rc_text(_u16 rc) {
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

_u32 cHttpConnection::receive(void) {
	_u32 r = 0;

	if(alive()) {
		if(!m_ibuffer)
			m_ibuffer = mpi_bmap->alloc();

		if(m_ibuffer) {
			_u8 *ptr = (_u8 *)mpi_bmap->ptr(m_ibuffer);
			_u32 sz_buffer = mpi_bmap->size();

			r = mp_sio->read(ptr + m_ibuffer_offset, sz_buffer - m_ibuffer_offset);
			m_ibuffer_offset += r;
		}
	}

	return r;
}

_u8 cHttpConnection::complete_req_header(void) {
	_u8 r = HTTPC_RECEIVE_HEADER;
	_u32 sz = mpi_bmap->size();

	if(m_ibuffer && m_ibuffer_offset) {
		if(m_ibuffer_offset < sz) {
			_str_t ptr = (_str_t)mpi_bmap->ptr(m_ibuffer);

			if(mpi_str->find_string(ptr, (_str_t)"\r\n\r\n") != -1)
				r = HTTPC_PARSE_HEADER;
		} else {
			m_response_code = HTTPRC_REQ_ENTITY_TOO_LARGE;
			r = HTTPC_SEND_HEADER;
		}
	}

	return r;
}

bool cHttpConnection::add_req_variable(_str_t name, _str_t value) {
	bool r = false;

	if(mpi_map->add(name, mpi_str->str_len(name), value, mpi_str->str_len(value)))
		r = true;

	return r;
}

_u8 cHttpConnection::parse_req_header(void) {
	_u8 r = HTTPC_RECEIVE_CONTENT;
	_u16 state = 0;
	_char_t var[128]="";
	_char_t val[128]="";
	_char_t method[16]="";
	_char_t uri[256]="";
	_char_t protocol[16]="";
	_u32 method_idx=0, uri_idx=0, protocol_idx=0,var_idx=0, val_idx=0;

#define _METHOD		(1<<0)
#define _URI		(1<<1)
#define _PROTOCOL	(1<<2)
#define _VAR		(1<<3)
#define _VAL		(1<<4)
#define _CR		(1<<5)
#define _LF		(1<<6)
#define _SPC		(1<<7)
#define _SYMBOL		(1<<8)

	if(m_ibuffer && m_ibuffer_offset) {
		_str_t ptr = (_str_t)mpi_bmap->ptr(m_ibuffer);
		_char_t _c = 0;

		state = _METHOD;
		for(_u32 i = 0; i < m_ibuffer_offset; i++) {
			_char_t c = ptr[i];

			if(c == ' ') {
				if(!(state & _SPC)) {
					if(state & _METHOD) {
						state &= ~_METHOD;
						state |= _URI;
					} else if(state & _URI) {
						state &= ~_URI;
						state |= _PROTOCOL;
					} else if(state & _PROTOCOL) {
						state &= ~_PROTOCOL;
						state |= _VAR;
					}
					state |= _SPC;
					state &= ~(_SYMBOL|_CR|_LF);
				}
			} else if(c == ':') {
			} else if(c == '\r') {
			} else if(c == '\n') {
			} else {
				if((state & _METHOD) && method_idx < sizeof(method))
					method[method_idx++] = c;
				else if((state & _URI) && uri_idx < sizeof(uri))
					uri[uri_idx++] = c;
				else if((state & _PROTOCOL) && protocol_idx < sizeof(protocol))
					protocol[protocol_idx++] = c;
				else if((state & _VAR) && var_idx < sizeof(var))
					var[var_idx++] = c;
				else if((state & _VAL) && val_idx < sizeof(val))
					val[val_idx++] = c;

				state |= _SYMBOL;
			}

			_c = c;
		}
	}

	return r;
}

_u32 cHttpConnection::res_remainder(void) {
	return m_content_len - (m_content_sent < m_content_len) ? m_content_sent : 0;
}

static cHttpConnection _g_httpc_;
