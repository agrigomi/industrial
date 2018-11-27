#include "private.h"

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
			mpi_bmap = 0;
			m_state = 0;
			m_req_buffer = 0;
			m_req_len = 0;
			m_res_len = 0;
			m_content_len = 0;
			m_content_sent = 0;
			if((mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL))) {
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			close();
			pi_repo->object_release(mpi_str);
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
		if(m_req_buffer) {
			mpi_bmap->free(m_req_buffer);
			m_req_buffer = 0;
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

_str_t cHttpConnection::req_header(_u32 *psz) {
	_str_t r = 0;

	if(mp_sio && m_req_buffer) {
		r = (_str_t)mpi_bmap->ptr(m_req_buffer);
		*psz = m_req_hdr_len;
	}

	return r;
}

_str_t cHttpConnection::req_body(_u32 *psz) {
	_str_t r = 0;

	if(mp_sio && m_req_buffer) {
		r = (_str_t)mpi_bmap->ptr(m_req_buffer);
		r += m_req_hdr_len;
		*psz = m_req_len - m_req_hdr_len;
	}

	return r;
}

bool cHttpConnection::complete_request(void) {
	bool r = false;

	//...
	if(m_req_len)
		r = true;

	return r;
}

_u32 cHttpConnection::read_request(void) {
	_u32 r = 0;

	if(alive()) {
		if(!m_req_buffer)
			m_req_buffer = mpi_bmap->alloc();

		if(m_req_buffer) {
			void *buffer = mpi_bmap->ptr(m_req_buffer);
			_u32 sz = mpi_bmap->get_size() - m_req_len;
			_u32 r = mp_sio->read(((_str_t)buffer) + m_req_len, sz);

			if(r) {
				m_req_len += r;
				if(!(m_state & (HTTPC_RES_HEADER | HTTPC_RES_BODY | HTTPC_RES_END)))
					m_state = HTTPC_REQ_PENDING;
			}
		}
	}

	return r;
}

void cHttpConnection::process(void) {
	if(!m_state)
		m_state |= HTTPC_REQ_PENDING;
	else if(m_state & HTTPC_REQ_PENDING) {
		if(!read_request() && alive()) {
			if(!(m_state & (HTTPC_RES_HEADER | HTTPC_RES_BODY | HTTPC_RES_END))) {
				if(complete_request())
					m_state |= HTTPC_RES_PENDING;
			} else if(m_state & HTTPC_RES_END) {
				if(m_content_sent >= m_content_len)
					m_state |= HTTPC_RES_SENT;
				else
					m_state &= ~HTTPC_RES_END;
			}
		}
	}
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

_u32 cHttpConnection::response(_u16 rc, // response code
				_str_t hdr, // response header
				_str_t body, // response body
				// Size of response body.
				// If zero, string length should be taken.
				// If it's greater than body lenght, ON_HTTP_RES_CONTINUE
				//  should be happen
				_u32 sz_body
			) {
	_u32 r = 0;
	_char_t lb[128]="";

	if(alive()) {
		_u32 body_len = (body) ? mpi_str->str_len(body) : 0;
		_u32 sz = snprintf(lb, sizeof(lb), "HTTP/1.1 %d %s\r\n", rc, get_rc_text(rc));

		m_content_len = (sz_body) ? sz_body : body_len;
		r = mp_sio->write(lb, sz);
		sz = snprintf(lb, sizeof(lb), "Content-Length: %u\r\n", m_content_len);
		r += mp_sio->write(lb, sz);
		r += mp_sio->write(hdr, mpi_str->str_len(hdr));
		r += mp_sio->write("\r\n", 2);
		m_state |= HTTPC_RES_HEADER;
		r += response(body, body_len);
	}

	return r;
}

_u32 cHttpConnection::response(_str_t body, // remainder part of response body
				_u32 sz_body // size of response body
				) {
	_u32 r = 0;

	if(alive() && body && sz_body && (m_content_sent < m_content_len)) {
		m_state |= HTTPC_RES_SENDING;
		if((m_content_sent + sz_body) >= m_content_len)
			m_state |= HTTPC_RES_END;
		if((r = mp_sio->write(body, sz_body)) > 0) {
			m_state |= HTTPC_RES_BODY;
			m_content_sent += r;
		}
	}

	return r;
}

_u32 cHttpConnection::remainder(void) {
	return m_content_len - (m_content_sent < m_content_len) ? m_content_sent : 0;
}

static cHttpConnection _g_httpc_;
