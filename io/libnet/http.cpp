#include <unistd.h>
#include "iRepository.h"
#include "iNet.h"
#include "iTaskMaker.h"
#include "private.h"

bool cHttpServer::_init(_u32 port) {
	bool r = false;

	if(p_tcps && !m_is_init)
		m_is_init = r = p_tcps->_init(port);

	return r;
}

void cHttpServer::_close(void) {
	if(m_is_init && p_tcps) {
		m_is_init = false;
		p_tcps->_close();
	}
}

void http_server_thread(cHttpServer *pobj) {
	pobj->m_is_running = true;
	pobj->m_is_stopped = false;

	while(pobj->m_is_running) {
		if(pobj->m_is_init) {
			//...
		} else
			usleep(10000);
	}

	pobj->m_is_stopped = true;
}

bool cHttpServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			m_is_init = m_is_running = m_use_ssl = false;
			mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
			if((p_tcps = (cTCPServer *)pi_repo->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE)))
				r = true;
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			_close();
			pi_repo->object_release(p_tcps);
			pi_repo->object_release(mpi_log);
			p_tcps = 0;
			r = true;
		} break;
		case OCTL_START:
			http_server_thread(this);
			break;
		case OCTL_STOP:
			m_is_running = false;
			while(!m_is_stopped)
				usleep(10000);
			break;
	}

	return r;
}

void cHttpServer::on_event(_u8 evt, _on_http_event_t *handler) {
	//...
}

bool cHttpServer::enable_ssl(bool enable, _ulong options) {
	bool r = false;

	if(m_is_init && p_tcps) {
		if((r = p_tcps->enable_ssl(enable, options)))
			m_use_ssl = enable;
	}

	return r;
}

bool cHttpServer::ssl_use(_cstr_t str, _u32 type) {
	bool r = false;

	if(m_is_init && m_use_ssl && p_tcps)
		r = p_tcps->ssl_use(str, type);

	return r;
}

static cHttpServer _g_http_server_;
