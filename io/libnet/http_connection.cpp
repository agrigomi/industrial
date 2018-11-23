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
			m_res_buffer = 0;
			m_res_len = 0;
			m_res_offset = 0;
			if((mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL))) {
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			_close();
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

void cHttpConnection::_close(void) {
	if(mp_sio) {
		mp_sio->_close();
		mp_sio = 0;
		if(m_req_buffer) {
			mpi_bmap->free(m_req_buffer);
			m_req_buffer = 0;
		}
		if(m_res_buffer) {
			mpi_bmap->free(m_res_buffer);
			m_res_buffer = 0;
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
		*psz = m_req_len;
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

void cHttpConnection::read_request(void) {
	if(!m_req_buffer)
		m_req_buffer = mpi_bmap->alloc();

	if(m_req_buffer) {
		void *buffer = mpi_bmap->ptr(m_req_buffer);
		_u32 sz = mpi_bmap->get_size() - m_req_len;
		_u32 nb = mp_sio->read(buffer, sz);

		if(nb)
			m_req_len += nb;
		else {
			if(complete_request()) {
				m_state &= ~HTTPC_REQ_PENDING;
				m_state |= HTTPC_REQ_END;
			}
		}
	}
}

void cHttpConnection::process(void) {
	if(!m_state)
		m_state |= HTTPC_REQ_PENDING;
	else if(m_state & HTTPC_REQ_PENDING)
		read_request();
	else if((m_state & HTTPC_REQ_END) && !(m_state & HTTPC_RES_PENDING))
		m_state |= HTTPC_RES_PENDING;
}

static cHttpConnection _g_httpc_;
