#include <string.h>
#include <unistd.h>
#include "iRepository.h"
#include "iNet.h"
#include "private.h"

#define CFREE	0 // column for pending connections
#define CBUSY	1 // column for busy connections

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

void *http_worker_thread(void *udata) {
	void *r = 0;
	cHttpServer *p_https = (cHttpServer *)udata;

	volatile _u32 num = p_https->m_active_workers++;
	p_https->m_num_workers = p_https->m_active_workers;

	while(num < p_https->m_active_workers) {
		//...
	}

	p_https->m_num_workers = p_https->m_active_workers;

	return r;
}

_u32 buffer_io(_u8 op, void *ptr, _u32 size, void *udata) {
	_u32 r = 0;

	switch(op) {
		case BIO_INIT:
			memset(ptr, 0, size);
			break;
	}

	return r;
}

bool cHttpServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			m_is_init = m_is_running = m_use_ssl = false;
			m_num_workers = m_active_workers = 0;
			mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
			p_tcps = (cTCPServer *)pi_repo->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE);
			mpi_bmap = (iBufferMap *)pi_repo->object_by_iname(I_BUFFER_MAP, RF_CLONE);
			mpi_tmaker = (iTaskMaker *)pi_repo->object_by_iname(I_TASK_MAKER, RF_ORIGINAL);
			mpi_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
			if(p_tcps && mpi_bmap && mpi_tmaker && mpi_list) {
				mpi_bmap->init(8192, buffer_io);
				mpi_list->init(LL_RING, 2);
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			// stop all workers
			m_active_workers = 0;
			while(m_num_workers)
				usleep(10000);

			_close();
			pi_repo->object_release(p_tcps);
			pi_repo->object_release(mpi_log);
			pi_repo->object_release(mpi_bmap);
			pi_repo->object_release(mpi_list);
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

bool cHttpServer::start_worker(void) {
	bool r = false;
	HTASK ht = 0;

	if((ht = mpi_tmaker->start(http_worker_thread, this))) {
		mpi_tmaker->set_name(ht, "http worker");
		r = true;
	}

	return r;
}

bool cHttpServer::stop_worker(void) {
	bool r = false;

	if(m_num_workers) {
		m_active_workers--;
		while(m_active_workers != m_num_workers)
			usleep(10000);
		r = true;
	}

	return r;
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
