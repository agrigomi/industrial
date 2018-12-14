#include <string.h>
#include <unistd.h>
#include "iRepository.h"
#include "iNet.h"
#include "private.h"

#define CFREE	0 // column for pending connections
#define CBUSY	1 // column for busy connections

#define MAX_WORKERS	32

// bufferMap callback
_u32 buffer_io(_u8 op, void *ptr, _u32 size, void *udata) {
	_u32 r = 0;

	switch(op) {
		case BIO_INIT:
			memset(ptr, 0, size);
			break;
	}

	return r;
}

bool cHttpServer::_init(_u32 port, _u32 buffer_size) {
	bool r = false;

	if(p_tcps && !m_is_init) {
		mpi_bmap->init(buffer_size, buffer_io);
		m_is_init = r = p_tcps->_init(port);
		p_tcps->blocking(false);
	}

	return r;
}

void cHttpServer::_close(void) {
	if(m_is_init && p_tcps) {
		m_is_init = false;
		p_tcps->_close();
	}
}

void cHttpServer::http_server_thread(void) {
	m_is_running = true;
	m_is_stopped = false;

	HTASK ht = mpi_tmaker->handle(this);
	if(ht)
		mpi_tmaker->set_name(ht, "http server");

	while(m_is_running) {
		if(m_is_init)
			add_connection();
		usleep(10000);
	}

	m_is_stopped = true;
}

void *http_worker_thread(void *udata) {
	void *r = 0;
	cHttpServer *p_https = static_cast<cHttpServer *>(udata);
	volatile _u32 num = p_https->m_active_workers++;

	p_https->m_num_workers = p_https->m_active_workers;

	while(num < p_https->m_active_workers) {
		_http_connection_t *rec = p_https->get_connection();

		if(rec) {
			if(rec->p_httpc->alive()) {
				_u8 evt = rec->p_httpc->process();

				p_https->call_event_handler(evt, rec->p_httpc);
				p_https->free_connection(rec);
			} else {
				p_https->call_event_handler(HTTP_ON_CLOSE, rec->p_httpc);
				p_https->remove_connection(rec);
			}
		} else
			usleep(10000);
	}

	p_https->m_num_workers--;

	return r;
}

bool cHttpServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			m_is_init = m_is_running = m_use_ssl = false;
			m_num_workers = m_active_workers = 0;
			memset(m_event, 0, sizeof(m_event));
			mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
			p_tcps = (cTCPServer *)pi_repo->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE);
			mpi_bmap = (iBufferMap *)pi_repo->object_by_iname(I_BUFFER_MAP, RF_CLONE);
			mpi_tmaker = (iTaskMaker *)pi_repo->object_by_iname(I_TASK_MAKER, RF_ORIGINAL);
			mpi_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
			if(p_tcps && mpi_bmap && mpi_tmaker && mpi_list) {
				mpi_list->init(LL_VECTOR, 2);
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			// stop all workers
			m_active_workers = 0;
			while(m_num_workers)
				usleep(10000);

			remove_all_connections();
			_close();
			pi_repo->object_release(p_tcps);
			pi_repo->object_release(mpi_log);
			pi_repo->object_release(mpi_bmap);
			pi_repo->object_release(mpi_tmaker);
			pi_repo->object_release(mpi_list);
			p_tcps = 0;
			r = true;
		} break;
		case OCTL_START:
			http_server_thread();
			break;
		case OCTL_STOP:
			m_is_running = false;
			while(!m_is_stopped)
				usleep(10000);
			break;
	}

	return r;
}

void cHttpServer::on_event(_u8 evt, _on_http_event_t *handler, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		m_event[evt].pf_handler = handler;
		m_event[evt].udata = udata;
	}
}

bool cHttpServer::call_event_handler(_u8 evt, iHttpConnection *pi_httpc) {
	bool r = false;

	if(evt && evt < HTTP_MAX_EVENTS && pi_httpc) {
		if(m_event[evt].pf_handler) {
			m_event[evt].pf_handler(pi_httpc, m_event[evt].udata);
			r = true;
		}
	}

	return r;
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

_http_connection_t *cHttpServer::add_connection(void) {
	_http_connection_t *r = 0;
	cSocketIO *p_sio = dynamic_cast<cSocketIO *>(p_tcps->listen());

	if(p_sio) {
		_http_connection_t rec;

		rec.p_httpc = (cHttpConnection *)_gpi_repo_->object_by_cname(CLASS_NAME_HTTP_CONNECTION, RF_CLONE);
		if(rec.p_httpc) {
			_u32 nfhttpc = 0;
			_u32 nbhttpc = 0;

			rec.state = CFREE;
			if(rec.p_httpc->_init(p_sio, mpi_bmap)) {
				HMUTEX hm = mpi_list->lock();
				mpi_list->col(CFREE, hm);
				if((r = (_http_connection_t *)mpi_list->add(&rec, sizeof(_http_connection_t), hm))) {
					nfhttpc = mpi_list->cnt(hm);
					mpi_list->col(CBUSY, hm);
					nbhttpc = mpi_list->cnt(hm);
				}
				mpi_list->unlock(hm);
				if((m_num_workers - nbhttpc) < nfhttpc && m_num_workers < MAX_WORKERS)
					// create worker
					start_worker();
			}
		}

		if(!r) {
			p_tcps->close(p_sio);
			if(rec.p_httpc)
				_gpi_repo_->object_release(rec.p_httpc);
		}
	}

	return r;
}

_http_connection_t *cHttpServer::get_connection(void) {
	HMUTEX hm = mpi_list->lock();
	_u32 sz = 0;

	mpi_list->col(CFREE, hm);

	_http_connection_t *rec = (_http_connection_t *)mpi_list->first(&sz, hm);

	if(rec) {
		if(mpi_list->mov(rec, CBUSY, hm))
			rec->state = CBUSY;
	}

	mpi_list->unlock(hm);

	return rec;
}

void cHttpServer::free_connection(_http_connection_t *rec) {
	HMUTEX hm = mpi_list->lock();

	if(rec->state == CBUSY) {
		if(mpi_list->mov(rec, CFREE, hm))
			rec->state = CFREE;
	}

	mpi_list->unlock(hm);
}

void cHttpServer::remove_connection(_http_connection_t *rec) {
	HMUTEX hm = mpi_list->lock();

	mpi_list->col(rec->state, hm);
	if(mpi_list->sel(rec, hm)) {
		p_tcps->close(dynamic_cast<iSocketIO *>(rec->p_httpc->get_socket_io()));
		_gpi_repo_->object_release(rec->p_httpc);
		mpi_list->del(hm);
	}

	mpi_list->unlock(hm);
}

void cHttpServer::clear_column(_u8 col, HMUTEX hlock) {
	_u32 sz = 0;
	_http_connection_t *rec = 0;

	mpi_list->col(col, hlock);
	while((rec = (_http_connection_t *)mpi_list->first(&sz, hlock))) {
		p_tcps->close(dynamic_cast<iSocketIO *>(rec->p_httpc->get_socket_io()));
		_gpi_repo_->object_release(rec->p_httpc);
		mpi_list->del(hlock);
	}
}

void cHttpServer::remove_all_connections(void) {
	HMUTEX hm = mpi_list->lock();

	clear_column(CFREE, hm);
	clear_column(CBUSY, hm);

	mpi_list->unlock(hm);
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
