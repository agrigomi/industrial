#include "private.h"

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
		_u32 sz = snprintf(lb, sizeof(lb), "HTTP/1.1 %d %s\r\n", rc, "...");

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
