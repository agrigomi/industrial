#include "iRepository.h"
#include "private.h"

bool server::init(_cstr_t name, _u32 port, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_exclude,
		_cstr_t path_disable, _u32 buffer_size,
		_u32 max_workers, _u32 max_connections,
		_u32 connection_timeout, SSL_CTX *ssl_context) {
	bool r = false;

	mpi_server = NULL; // HTTP server
	mpi_vhost_map = NULL;
	strncpy(m_name, name, sizeof(m_name)-1);
	m_port = port;
	mpi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL));
	mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
	if((mpi_pool = dynamic_cast<iPool *>(_gpi_repo_->object_by_iname(I_POOL, RF_CLONE|RF_NONOTIFY)))) {
		mpi_pool->init(sizeof(_connection_t), [](_u8 op, void *data, void *udata) {
			_connection_t *pcnt = (_connection_t *)data;
			server *p_srv = (server *)udata;

			switch(op) {
				case POOL_OP_NEW:
				case POOL_OP_BUSY: {
						_connection_t tmp;

						tmp.req.mpi_server = tmp.res.mpi_server = p_srv;
						tmp.req.mpi_cookie_map = NULL;
						tmp.res.mpi_heap = p_srv->mpi_heap;
						tmp.res.mpi_bmap = p_srv->mpi_bmap;
						tmp.res.mp_hbarray = NULL;
						tmp.res.m_hbcount = 0;
						tmp.res.m_buffers = 0;
						tmp.res.m_content_len = 0;
						tmp.res.mpi_fs = p_srv->mpi_fs;
						tmp.url = NULL;
						tmp.hdoc = NULL;
						tmp.p_vhost = NULL;
						memcpy((void *)pcnt, (void *)&tmp, sizeof(_connection_t));
					} break;
				case POOL_OP_FREE:
				case POOL_OP_DELETE:
					pcnt->destroy();
					break;
			};
		}, this, mpi_heap);
	}
	mpi_net = NULL;
	mpi_fs = NULL;
	attach_fs();
	attach_network();
	m_autorestore = false;
	m_buffer_size = buffer_size;
	if((mpi_bmap = dynamic_cast<iBufferMap *>(_gpi_repo_->object_by_iname(I_BUFFER_MAP, RF_CLONE|RF_NONOTIFY)))) {
		mpi_bmap->init(m_buffer_size, [](_u8 op, void *bptr, _u32 sz, void *udata)->_u32 {
			_u32 r = 0;

			if(op == BIO_INIT)
				memset(bptr, 0, sz);

			return r;
		});
	}
	m_max_workers = max_workers;
	m_max_connections = max_connections;
	m_connection_timeout = connection_timeout;
	m_ssl_context = ssl_context;

	r = host.init(this, "defaulthost", root, cache_path, name, cache_exclude, path_disable, mpi_heap);

	return r;
}

void server::attach_network(void) {
	if(!mpi_net) {
		if((mpi_net = dynamic_cast<iNet *>(_gpi_repo_->object_by_iname(I_NET, RF_ORIGINAL))))
			mpi_log->fwrite(LMT_INFO, "'%s' attach networking", m_name);
		else
			mpi_log->fwrite(LMT_ERROR, "'%s' unable to attach networking", m_name);
	}
}
void server::attach_fs(void) {
	if(!mpi_fs) {
		if((mpi_fs = dynamic_cast<iFS *>(_gpi_repo_->object_by_iname(I_FS, RF_ORIGINAL))))
			mpi_log->fwrite(LMT_INFO, "'%s' attach FS", m_name);
		else
			mpi_log->fwrite(LMT_ERROR, "'%s' unable to attach FS", m_name);
	}
}

void server::release_network(void) {
	stop();
	if(mpi_net) {
		mpi_log->fwrite(LMT_INFO, "'%s' detach networking", m_name);
		_gpi_repo_->object_release(mpi_net);
		mpi_net = NULL;
	}
}

void server::release_fs(void) {
	stop();
	if(mpi_fs) {
		mpi_log->fwrite(LMT_INFO, "'%s' detach FS", m_name);
		_gpi_repo_->object_release(mpi_fs);
		mpi_fs = NULL;
	}
}

void server::destroy(void) {
	release_network();
	release_fs();

	// destroy virtual hosts
	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		server *p = (server *)udata;
		p->destroy(pvhost);
	}, this);

	destroy(&host); // destroy default host

	_gpi_repo_->object_release(mpi_vhost_map, false);
	_gpi_repo_->object_release(mpi_heap);
	_gpi_repo_->object_release(mpi_log);
	_gpi_repo_->object_release(mpi_bmap, false);
	_gpi_repo_->object_release(mpi_pool, false);
}

void server::destroy(_vhost_t *pvhost) {
	stop(pvhost);
	pvhost->destroy();
}

bool server::create_connection(iHttpServerConnection *p_httpc) {
	bool r = false;

	_connection_t *p_cnt = (_connection_t *)mpi_pool->alloc();

	if(p_cnt) {
		p_cnt->req.mpi_httpc = p_cnt->res.mpi_httpc = p_httpc;
		p_httpc->set_udata((_ulong)p_cnt, IDX_CONNECTION);
		r = true;
	}

	return r;
}

void server::destroy_connection(iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		// detach connection record
		p_httpc->set_udata((_ulong)NULL, IDX_CONNECTION);

		// release connection
		mpi_pool->free(pc);
	}
}

void server::set_handlers(void) {
	mpi_server->on_event(HTTP_ON_OPEN, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->create_connection(p_httpc);
		p_srv->call_handler(HTTP_ON_OPEN, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;
		_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

		pc->clear();
		pc->p_vhost = p_srv->get_host(p_httpc->req_var("Host"));
		pc->url = p_httpc->req_url();
		pc->res.mpi_root = pc->p_vhost->get_root();
		pc->res.mpi_fcache = pc->p_vhost->get_root()->get_file_cache();

		p_srv->call_handler(HTTP_ON_REQUEST, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST_DATA, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_REQUEST_DATA, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST_DATA, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->update_response(p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_ERROR, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_ERROR, p_httpc);
		p_srv->call_route_handler(HTTP_ON_ERROR, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_CLOSE, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_CLOSE, p_httpc);
		p_srv->destroy_connection(p_httpc);
	}, this);
}

bool server::start(void) {
	bool r = false;

	attach_fs();
	attach_network();

	if(mpi_net && mpi_fs) {
		// start virtual hosts
		enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
			server *p = (server *)udata;
			p->start(pvhost);
		}, this);

		// start default host
		start(&host);

		if(!mpi_server) // create HTTP engine
			mpi_server = mpi_net->create_http_server(m_port, m_buffer_size, m_max_workers,
								m_max_connections, m_connection_timeout,
								m_ssl_context);

		if(host.get_root()->is_enabled() && mpi_server) {
			mpi_log->fwrite(LMT_INFO, "Gatn: Start server '%s'", m_name);
			set_handlers();
			r = true;
		}
	}

	return r;
}

void server::stop(_vhost_t *pvhost) {
	if(pvhost->is_running()) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Stop host '%s' on server '%s'", pvhost->name(), m_name);
		pvhost->stop();
	}
}

_vhost_t *server::get_host(_cstr_t _host) {
	_vhost_t *r = (_host) ? get_virtual_host(_host) : &host;
	return (r) ? r : &host;
}

bool server::start(_vhost_t *pvhost) {
	bool r = false;

	if(!(r = pvhost->is_running())) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Start host '%s' on server '%s'", pvhost->name(), m_name);
		pvhost->start();
		r = pvhost->is_running();
	}

	return r;
}

void server::stop(void) {
	if(mpi_server) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Stop server '%s'", m_name);
		_gpi_repo_->object_release(mpi_server, false);
		mpi_server = NULL;
	}

	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		server *p = (server *)udata;
		p->stop(pvhost);
	}, this);

	stop(&host); // stop default host
}

void server::call_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		_vhost_t *pvhost = (pc->p_vhost) ? pc->p_vhost : &host;

		pvhost->call_handler(evt, p_httpc);
	}
}

void server::call_route_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		_vhost_t *pvhost = (pc->p_vhost) ? pc->p_vhost : &host;

		pvhost->call_route_handler(evt, p_httpc);
	}
}

void server::update_response(iHttpServerConnection *p_httpc) {
	// write response remainder
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		if(pc->hdoc) {
			// update content from file cache
			_ulong sz = 0;
			_vhost_t *pvhost = pc->p_vhost;

			if(pvhost) {
				_root_t *proot = pvhost->get_root();

				if(proot->is_enabled()) {
					_u8 *ptr = (_u8 *)proot->ptr(pc->hdoc, &sz);

					if(ptr) {
						_u32 res_sent = p_httpc->res_content_sent();
						_u32 res_remainder = p_httpc->res_remainder();

						p_httpc->res_write(ptr + res_sent, res_remainder);
					}
				}
			}
		} else
			// update content from response buffer
			pc->res.process_content();
	}
}

void server::on_route(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata, _cstr_t _host) {
	_vhost_t *pvhost = get_host(_host);

	pvhost->add_route_handler(method, path, pcb, udata);
}

void server::on_event(_u8 evt, _gatn_http_event_t *pcb, void *udata, _cstr_t _host) {
	_vhost_t *pvhost = get_host(_host);

	pvhost->set_event_handler(evt, pcb, udata);
}

_gatn_http_event_t *server::get_event_handler(_u8 evt, void **pp_udata, _cstr_t host) {
	_vhost_t *pvhost = get_host(host);

	return pvhost->get_event_handler(evt, pp_udata);
}

void server::remove_route(_u8 method, _cstr_t path, _cstr_t host) {
	_vhost_t *pvhost = get_host(host);

	pvhost->remove_route_handler(method, path);
}

bool server::add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key,
				_cstr_t cache_exclude, _cstr_t path_disable) {
	bool r = false;
	_vhost_t vhost;

	if(!mpi_vhost_map) {
		if((mpi_vhost_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			mpi_vhost_map->init(15, mpi_heap);
	}

	if(mpi_vhost_map) {
		_u32 sz = 0;
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

		if(!pvhost) {
			 pvhost = (_vhost_t *)mpi_vhost_map->add(host, strlen(host), &vhost, sizeof(_vhost_t));

			if(pvhost)
				r = pvhost->init(this, host, root, cache_path, cache_key,
						cache_exclude, path_disable, mpi_heap);
		} else
			r = true;
	}

	return r;
}

_vhost_t *server::get_virtual_host(_cstr_t host) {
	_u32 sz = 0;
	_vhost_t *r = NULL;

	if(mpi_vhost_map)
		r = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

	return r;
}

bool server::remove_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

		if(pvhost) {
			destroy(pvhost);
			mpi_vhost_map->del(host, strlen(host));
			r = true;
		}
	}

	return r;
}

bool server::start_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

		if(pvhost)
			r = start(pvhost);
	}

	return r;
}

bool server::stop_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

		if(pvhost) {
			stop(pvhost);
			r = true;
		}
	}

	return r;
}

void server::enum_virtual_hosts(void (*enum_cb)(_vhost_t *, void *udata), void *udata) {
	if(mpi_vhost_map) {
		_map_enum_t vhe = mpi_vhost_map->enum_open();
		_u32 sz = 0;
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->enum_first(vhe, &sz);

		while(pvhost) {
			enum_cb(pvhost, udata);
			pvhost = (_vhost_t *)mpi_vhost_map->enum_next(vhe, &sz);
		}

		mpi_vhost_map->enum_close(vhe);
	}
}

bool server::attach_class(_cstr_t cname, _cstr_t options, _cstr_t _host) {
	_vhost_t *pvhost = get_host(_host);

	return pvhost->attach_class(cname, options);
}

bool server::detach_class(_cstr_t cname, _cstr_t _host) {
	_vhost_t *pvhost = get_host(_host);

	return pvhost->detach_class(cname);
}

void server::release_class(_cstr_t cname) {
	struct _xhost {
		_cstr_t _cname;
		server	*psrv;
	};

	_xhost tmp = { cname, this};

	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		_xhost *p = (_xhost *)udata;

		p->psrv->detach_class(p->_cname, pvhost->name());
	}, &tmp);

	detach_class(cname);
}
