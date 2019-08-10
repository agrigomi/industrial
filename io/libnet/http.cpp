#include <string.h>
#include <unistd.h>
#include "iRepository.h"
#include "iNet.h"
#include "private.h"

#define CFREE		0 // unused connections
#define CPENDING	1 // column for pending connections
#define CBUSY		2 // column for busy connections

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

void *_http_server_thread(_u8 sig, void *arg) {
	cHttpServer *srv = (cHttpServer *)arg;

	if(sig == TM_SIG_START)
		srv->http_server_thread();
	else if(sig == TM_SIG_STOP)
		;
	return NULL;
}

bool cHttpServer::_init(_u32 port,
			_u32 buffer_size,
			_u32 max_workers,
			_u32 max_connections,
			_u32 connection_timeout,
			SSL_CTX *ssl_context) {
	bool r = false;

	if(p_tcps && !m_is_init) {
		if((m_is_init = r = p_tcps->_init(port, ssl_context))) {
			_char_t sname[17]="";

			mpi_bmap->init(buffer_size, buffer_io);
			m_max_workers = max_workers;
			m_max_connections = max_connections;
			m_connection_timeout = connection_timeout;
			m_port = port;
			p_tcps->blocking(false);

			HTASK ht = mpi_tmaker->start(_http_server_thread, this);

			snprintf(sname, sizeof(sname) - 1, "http-s:%u", m_port);
			mpi_tmaker->set_name(ht, sname);
		}
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
	_u32 to_idle = 100;

	while(m_is_running) {
		if(m_is_init && m_num_connections < m_max_connections) {
			add_connection();
			to_idle = 100;
		}

		if(to_idle) {
			usleep(10000);
			to_idle--;
		} else
			usleep(100000);
	}

	m_is_stopped = true;
}

void *_http_worker_thread(_u8 sig, void *udata) {
	void *r = 0;
	cHttpServer *p_https = static_cast<cHttpServer *>(udata);

	if(sig == TM_SIG_START) {
		volatile _u32 num = p_https->m_active_workers++;
		_u32 to_idle = 100;

		p_https->m_num_workers++;

		while(num < p_https->m_active_workers) {
			_http_connection_t *rec = p_https->get_connection();

			if(rec) {
				to_idle = 100;
				if(rec->p_httpc) {
					if(rec->p_httpc->alive()) {
						_u8 evt = rec->p_httpc->process();

						p_https->call_event_handler(evt, rec->p_httpc);
						p_https->pending_connection(rec);
					} else {
						p_https->call_event_handler(HTTP_ON_CLOSE, rec->p_httpc);
						p_https->release_connection(rec);
					}
				} else
					p_https->release_connection(rec);
			} else {
				if(to_idle) {
					usleep(10000);
					to_idle--;
				} else
					usleep(100000);
			}
		}

		p_https->m_num_workers--;
	}

	return r;
}

bool cHttpServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			m_is_init = m_is_running = m_use_ssl = false;
			m_is_stopped = true;
			m_num_connections = m_num_workers = m_active_workers = 0;
			memset(m_event, 0, sizeof(m_event));
			mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
			p_tcps = (cTCPServer *)pi_repo->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE|RF_NONOTIFY);
			mpi_bmap = (iBufferMap *)pi_repo->object_by_iname(I_BUFFER_MAP, RF_CLONE|RF_NONOTIFY);
			mpi_tmaker = (iTaskMaker *)pi_repo->object_by_iname(I_TASK_MAKER, RF_ORIGINAL);
			mpi_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE|RF_NONOTIFY);
			m_hconnection = pi_repo->handle_by_cname(CLASS_NAME_HTTP_SERVER_CONNECTION);
			if(p_tcps && mpi_bmap && mpi_tmaker && mpi_list && m_hconnection) {
				mpi_list->init(LL_VECTOR, 3);
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			// stop server listen thread
			if(m_is_running) {
				m_is_running = false;
				while(!m_is_stopped)
					usleep(10000);
			}
			// stop all workers
			m_active_workers = 0;
			_u32 t = 1000;
			while(m_num_workers && t) {
				usleep(10000);
				t--;
			}

			remove_all_connections();
			_close();
			pi_repo->object_release(p_tcps, false);
			pi_repo->object_release(mpi_log);
			pi_repo->object_release(mpi_bmap, false);
			pi_repo->object_release(mpi_tmaker);
			pi_repo->object_release(mpi_list, false);
			p_tcps = 0;
			r = true;
		} break;
	}

	return r;
}

void cHttpServer::on_event(_u8 evt, _on_http_event_t *handler, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		m_event[evt].pf_handler = handler;
		m_event[evt].udata = udata;
	}
}

bool cHttpServer::call_event_handler(_u8 evt, iHttpServerConnection *pi_httpc) {
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

	if((ht = mpi_tmaker->start(_http_worker_thread, this))) {
		_char_t wname[17]="";

		snprintf(wname, sizeof(wname) - 1, "http-w:%u", m_port);
		mpi_tmaker->set_name(ht, wname);
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

_http_connection_t *cHttpServer::alloc_connection(HMUTEX hlock) {
	_http_connection_t rec;
	_http_connection_t *r = NULL;
	_u32 sz = 0;

	mpi_list->col(CFREE, hlock);
	if((r = (_http_connection_t *)mpi_list->first(&sz, hlock))) {
		r->state = CPENDING;
		mpi_list->mov(r, CPENDING, hlock);
	} else if((rec.p_httpc = (cHttpServerConnection *)_gpi_repo_->object_by_handle(m_hconnection, RF_CLONE|RF_NONOTIFY))) {
		mpi_list->col(CPENDING, hlock);
		rec.state = CPENDING;
		r = (_http_connection_t *)mpi_list->add(&rec, sizeof(_http_connection_t), hlock);
	}

	return r;
}

_http_connection_t *cHttpServer::add_connection(void) {
	_http_connection_t *r = 0;
	cSocketIO *p_sio = dynamic_cast<cSocketIO *>(p_tcps->listen());

	if(p_sio) {
		HMUTEX hm = mpi_list->lock();

		if((r = alloc_connection(hm))) {
			if(r->p_httpc->_init(p_sio, mpi_bmap, m_connection_timeout)) {
				_u32 nphttpc = 0;
				_u32 nbhttpc = 0;

				mpi_list->col(CPENDING, hm);
				m_num_connections = nphttpc = mpi_list->cnt(hm);
				mpi_list->col(CBUSY, hm);
				nbhttpc = mpi_list->cnt(hm);

				if((m_num_workers - nbhttpc) < nphttpc && m_num_workers < m_max_workers)
					// create worker
					start_worker();
			} else
				release_connection(r);
		} else
			p_tcps->close(p_sio);
		mpi_list->unlock(hm);
	}

	return r;
}

_http_connection_t *cHttpServer::get_connection(void) {
	HMUTEX hm = mpi_list->lock();
	_u32 sz = 0;

	mpi_list->col(CPENDING, hm);

	_http_connection_t *rec = (_http_connection_t *)mpi_list->first(&sz, hm);

	if(rec) {
		if(mpi_list->mov(rec, CBUSY, hm))
			rec->state = CBUSY;
	}

	mpi_list->unlock(hm);

	return rec;
}

void cHttpServer::pending_connection(_http_connection_t *rec) {
	HMUTEX hm = mpi_list->lock();

	if(rec->state == CBUSY) {
		if(mpi_list->mov(rec, CPENDING, hm))
			rec->state = CPENDING;
	}

	mpi_list->unlock(hm);
}

void cHttpServer::release_connection(_http_connection_t *rec) {
	HMUTEX hm = mpi_list->lock();

	mpi_list->col(rec->state, hm);
	if(mpi_list->sel(rec, hm)) {
		if(rec->p_httpc)
			rec->p_httpc->close();
		mpi_list->mov(rec, CFREE, hm);
	}

	mpi_list->unlock(hm);
}

void cHttpServer::clear_column(_u8 col, HMUTEX hlock) {
	_u32 sz = 0;
	_http_connection_t *rec = 0;

	mpi_list->col(col, hlock);
	while((rec = (_http_connection_t *)mpi_list->first(&sz, hlock))) {
		if(rec->p_httpc) {
			call_event_handler(HTTP_ON_CLOSE, rec->p_httpc);
			_gpi_repo_->object_release(rec->p_httpc, false);
		}
		mpi_list->del(hlock);
	}
}

void cHttpServer::remove_all_connections(void) {
	HMUTEX hm = mpi_list->lock();

	clear_column(CFREE, hm);
	clear_column(CBUSY, hm);
	clear_column(CPENDING, hm);
	mpi_list->unlock(hm);
}

static cHttpServer _g_http_server_;
