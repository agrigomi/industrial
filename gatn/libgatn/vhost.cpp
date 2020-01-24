#include <string.h>
#include "err.h"
#include "private.h"
#include "iRepository.h"
#include "iLog.h"

#define GATN_SERVER_PREFIX	"industrial-gatn"
#define MAX_GATN_SERVER_NAME	256

typedef struct {
	iGatnExtension	*pi_ext;
	_str_t		options;
	_u32		sz_options;
	bool		active;
}_class_t;

bool vhost::init(_server_t *server, _cstr_t name, _cstr_t doc_root,
		_cstr_t cache_path, _cstr_t cache_key, _cstr_t cache_exclude,
		_cstr_t root_exclude, iHeap *_heap) {
	pi_server = server;
	strncpy(host, name, sizeof(host)-1);
	pi_route_map = pi_class_map = NULL;
	pi_mutex = NULL;
	m_hlock = 0;
	pi_heap = _heap;
	m_running = false;
	memset(event, 0, sizeof(event));
	pi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL));

	return root.init(doc_root, cache_path, cache_key, cache_exclude, root_exclude, pi_heap);
}

void vhost::destroy(void) {
	stop_extensions();
	remove_extensions();
	root.destroy();

	if(pi_mutex) {
		_gpi_repo_->object_release(pi_mutex, false);
		pi_mutex = NULL;
	}
	if(pi_route_map) {
		_gpi_repo_->object_release(pi_route_map, false);
		pi_route_map = NULL;
	}

	if(pi_class_map) {
		_gpi_repo_->object_release(pi_class_map, false);
		pi_class_map = NULL;
	}

	if(pi_log) {
		_gpi_repo_->object_release(pi_log);
		pi_log = NULL;
	}
}

vhost::vhost() {
	host[0] = 0;
	pi_route_map = pi_class_map = NULL;
	pi_mutex = 0;
	pi_log = NULL;
	m_hlock = 0;
	m_running = false;
	memset(event, 0, sizeof(event));
}

iMutex *vhost::get_mutex(void) {
	if(!pi_mutex)
		pi_mutex = dynamic_cast<iMutex *>(_gpi_repo_->object_by_iname(I_MUTEX, RF_CLONE|RF_NONOTIFY));
	return pi_mutex;
}

iMap *vhost::get_route_map(void) {
	if(!pi_route_map) {
		if((pi_route_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			pi_route_map->init(31, pi_heap);
	}
	return pi_route_map;
}

iMap *vhost::get_class_map(void) {
	if(!pi_class_map) {
		if((pi_class_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			pi_class_map->init(31, pi_heap);
	}

	return pi_class_map;
}

HMUTEX vhost::lock(HMUTEX hlock) {
	HMUTEX r = 0;

	iMutex *pi_mutex = get_mutex();

	if(pi_mutex)
		r = pi_mutex->lock(hlock);

	return r;
}

void vhost::_lock(void) {
	m_hlock = lock(m_hlock);
}

void vhost::unlock(HMUTEX hlock) {
	iMutex *pi_mutex = get_mutex();

	if(pi_mutex)
		pi_mutex->unlock(hlock);
}

void vhost::_unlock(void) {
	unlock(m_hlock);
}

void vhost::clear_events(void) {
	memset(event, 0, sizeof(event));
}

void vhost::start_extensions(HMUTEX hlock) {
	iMap *pi_map = get_class_map();
	HMUTEX hm = lock(hlock);
	typedef struct {
		_cstr_t host;
		_server_t *pi_srv;
	}_attach_info_t;

	if(pi_map) {
		_attach_info_t tmp = {host, pi_server};

		pi_class_map->enumerate([](void *p, _u32 sz, void *udata)->_s32 {
			_attach_info_t *pa = (_attach_info_t *)udata;
			_class_t *pclass = (_class_t *)p;

			if(!pclass->active && pclass->pi_ext)
				pclass->active = pclass->pi_ext->attach(pa->pi_srv, pa->host);

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}

void vhost::stop_extensions(HMUTEX hlock) {
	iMap *pi_map = get_class_map();
	HMUTEX hm = lock(hlock);
	typedef struct {
		_cstr_t host;
		_server_t *pi_srv;
	}_attach_info_t;

	if(pi_map) {
		_attach_info_t tmp = {host, pi_server};

		pi_class_map->enumerate([](void *p, _u32 sz, void *udata)->_s32 {
			_attach_info_t *pa = (_attach_info_t *)udata;
			_class_t *pclass = (_class_t *)p;

			if(pclass->active && pclass->pi_ext) {
				pclass->pi_ext->detach(pa->pi_srv, pa->host);
				pclass->active = false;
			}

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}

void vhost::remove_extensions(void) {
	iMap *pi_map = get_class_map();
	HMUTEX hm = lock();
	typedef struct {
		_cstr_t host;
		_server_t *pi_srv;
		vhost  *pvhost;
	}_attach_info_t;

	if(pi_map) {
		_attach_info_t tmp = {host, pi_server, this};

		pi_class_map->enumerate([](void *p, _u32 sz, void *udata)->_s32 {
			_attach_info_t *pa = (_attach_info_t *)udata;
			_class_t *pclass = (_class_t *)p;
			vhost *pvhost = pa->pvhost;

			if(pclass->active && pclass->pi_ext) {
				pclass->pi_ext->detach(pa->pi_srv, pa->host);
				pclass->active = false;
			}

			_gpi_repo_->object_release(pclass->pi_ext, false);
			pclass->pi_ext = NULL;
			if(pclass->options) {
				pvhost->pi_heap->free(pclass->options, pclass->sz_options);
				pclass->options = NULL;
				pclass->sz_options = 0;
			}

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}

bool vhost::start(void) {
	bool r = false;
	HMUTEX hm = lock();

	if(!(r = root.is_enabled())) {
		start_extensions(hm);
		root.start();
		if(!(r = m_running = root.is_enabled()))
			pi_log->fwrite(LMT_ERROR, "Gatn: Unable to start '%s'", host);
	}

	unlock(hm);

	return r;
}

void vhost::stop(void) {
	HMUTEX hm = lock();

	if(root.is_enabled()) {
		stop_extensions(hm);
		clear_events();
		root.stop();
		m_running = root.is_enabled();
	}

	unlock(hm);
}

void vhost::add_route_handler(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};
	iMap *pi_map = get_route_map();

	if(pi_map) {
		strncpy(data.path, path, sizeof(data.path)-1);
		memset(&key, 0, sizeof(_route_key_t));
		key.method = method;
		strncpy(key.path, path, MAX_ROUTE_PATH-1);
		pi_map->add(&key, key.size(), &data, data.size());
	}
}

void vhost::remove_route_handler(_u8 method, _cstr_t path) {
	_route_key_t key;
	iMap *pi_map = get_route_map();

	if(pi_map) {
		memset(&key, 0, sizeof(_route_key_t));
		key.method = method;
		strncpy(key.path, path, MAX_ROUTE_PATH-1);
		pi_map->del(&key, key.size());
	}
}

void vhost::set_event_handler(_u8 evt, _gatn_http_event_t *pcb, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		event[evt].pcb = pcb;
		event[evt].udata = udata;
	}
}

_gatn_http_event_t *vhost::get_event_handler(_u8 evt, void **pp_udata) {
	_gatn_http_event_t *r = NULL;

	if(evt < HTTP_MAX_EVENTS) {
		r = event[evt].pcb;
		*pp_udata = event[evt].udata;
	}

	return r;
}

bool vhost::attach_class(_cstr_t cname, _cstr_t options) {
	bool r = false;
	iMap *pi_map = get_class_map();

	if(pi_map) {
		_u32 sz = 0;
		HMUTEX hm = lock();
		_class_t *pclass = (_class_t *)pi_map->get(cname, strlen(cname), &sz);

		if(!pclass) {
			_object_info_t oi;
			iBase *pi_base = _gpi_repo_->object_by_cname(cname, RF_CLONE|RF_NONOTIFY);

			if(pi_base) {
				pi_base->object_info(&oi);
				if(strcmp(I_GATN_EXTENSION, oi.iname) == 0) {
					_class_t tmp = {NULL, NULL, 0, false};

					tmp.pi_ext = dynamic_cast<iGatnExtension *>(pi_base);

					if(tmp.pi_ext) {
						if((pclass = (_class_t *)pi_map->add(cname, strlen(cname), &tmp, sizeof(tmp)))) {
							pi_log->fwrite(LMT_INFO, "Gatn: Attach class '%s' to '%s/%s'",
									cname, pi_server->name(), host);
							if(options) { // backup options
								pclass->sz_options = strlen(options) + 1;
								if((pclass->options = (_str_t)pi_heap->alloc(pclass->sz_options)))
									strcpy(pclass->options, options);
								pclass->pi_ext->options(options);
							}

							pclass->active = r = pclass->pi_ext->attach(pi_server, host);
						} else
							_gpi_repo_->object_release(tmp.pi_ext, false);
					}
				} else {
					pi_log->fwrite(LMT_ERROR, "Gatn: Unable to attach class '%s'", cname);
					_gpi_repo_->object_release(pi_base, false);
				}
			} else
				pi_log->fwrite(LMT_ERROR, "Gatn: Unable to clone class '%s'", cname);
		} else {
			// try to reattach class
			if(!pclass->pi_ext) {
				if(options) {
					_u32 sz_opt = strlen(options) + 1;

					if(pclass->options)
						pi_heap->free(pclass->options, pclass->sz_options);

					pclass->options = (_str_t)pi_heap->alloc(sz_opt);
					strcpy(pclass->options, options);
					pclass->sz_options = sz_opt;
				}

				if((pclass->pi_ext = dynamic_cast<iGatnExtension *>(_gpi_repo_->object_by_cname(cname, RF_CLONE)))) {
					pclass->pi_ext->options(pclass->options);
					pclass->active = pclass->pi_ext->attach(pi_server, host);
				}
			} else
				pi_log->fwrite(LMT_ERROR, "Gatn: class '%s' already attached to '%s/%s'",
						cname, pi_server->name(), host);
		}

		unlock(hm);
	}

	return r;
}

bool vhost::detach_class(_cstr_t cname, bool remove) {
	bool r = false;
	iMap *pi_map = get_class_map();

	if(pi_map) {
		_u32 sz = 0;
		HMUTEX hm = lock();
		_class_t *pclass = (_class_t *)pi_map->get(cname, strlen(cname), &sz);

		if(pclass) {
			if(pclass->pi_ext) {
				_event_data_t 	_event[HTTP_MAX_EVENTS];	// backup of HTTP event handlers
				bool reset = false;

				if(pclass->pi_ext) {
					// backup event handlers
					memcpy(_event, event, sizeof(_event));

					pi_log->fwrite(LMT_INFO, "Gatn: Detach class '%s' from '%s/%s'",
							cname, pi_server->name(), host);
					pclass->pi_ext->detach(pi_server, host);
					pclass->active = false;

					if(memcmp(_event, event, sizeof(event)) != 0)
						reset = true;

					_gpi_repo_->object_release(pclass->pi_ext, false);
					pclass->pi_ext = NULL;
				}

				if(remove) {
					if(pclass->options)
						pi_heap->free(pclass->options, pclass->sz_options);
					pi_map->del(cname, strlen(cname));
				}

				if(reset) {
					stop_extensions(hm);
					clear_events();
					start_extensions(hm);
				}
			}
		}

		unlock(hm);
	}

	return r;
}

void vhost::restore_class(_cstr_t cname) {
	iMap *pi_map = get_class_map();

	if(pi_map) {
		_u32 sz = 0;
		HMUTEX hm = lock();
		_class_t *pclass = (_class_t *)pi_map->get(cname, strlen(cname), &sz);

		if(pclass) {
			if(!pclass->pi_ext) {
				if((pclass->pi_ext = dynamic_cast<iGatnExtension *>(_gpi_repo_->object_by_cname(cname, RF_CLONE)))) {
					pi_log->fwrite(LMT_INFO, "Gatn: Attach class '%s' to '%s/%s'",
							cname, pi_server->name(), host);
					pclass->pi_ext->options(pclass->options);
					pclass->active = pclass->pi_ext->attach(pi_server, host);
				}
			}
		}

		unlock(hm);
	}
}

_s32 vhost::call_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_s32 r = EHR_CONTINUE;
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc && pc->p_vhost == this) {
		_lock();

		if(evt < HTTP_MAX_EVENTS) {
			if(event[evt].pcb)
				r = event[evt].pcb(&(pc->req), &(pc->res), event[evt].udata);
		}

		_unlock();
	}

	return r;
}

void vhost::call_route_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_lock();

	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
	iMap *pi_map = get_route_map();

	if(pc && pi_map) {
		_cstr_t url = pc->url;

		if(url) {
			_route_data_t *prd = NULL;
			_u8 method = p_httpc->req_method();
			_route_key_t key;
			_u32 sz = 0;
			_char_t server_name[MAX_GATN_SERVER_NAME]="";

			snprintf(server_name, sizeof(server_name), "%s [%s]", GATN_SERVER_PREFIX, pi_server->name());
			p_httpc->res_var("Server", server_name);

			memset(&key, 0, sizeof(_route_key_t));
			key.method = method;
			strncpy(key.path, url, sizeof(key.path)-1);

			prd = (_route_data_t *)pi_map->get(&key, key.size(), &sz);

			if(prd)
				// route found
				prd->pcb(evt, &(pc->req), &(pc->res), prd->udata);
			else if(root.is_enabled() && evt == HTTP_ON_REQUEST) {
				// route not found
				// try to resolve file name

				p_httpc->res_protocol("HTTP/1.1");

				if(method == HTTP_METHOD_GET || method == HTTP_METHOD_HEAD || method == HTTP_METHOD_POST) {
					HDOCUMENT hdoc = root.open(url);

					if(hdoc) {
						p_httpc->res_var("Content-Type", root.mime(hdoc));
						p_httpc->res_mtime(root.mtime(hdoc));

						if(method == HTTP_METHOD_GET || method == HTTP_METHOD_POST) {
							_ulong doc_sz = 0;

							_u8 *ptr = (_u8 *)root.ptr(hdoc, &doc_sz);
							if(ptr) {
								// response header
								p_httpc->res_content_len(doc_sz);
								p_httpc->res_code(HTTPRC_OK);
								// response content
								p_httpc->res_write(ptr, doc_sz);
								pc->hdoc = hdoc;
							} else {
								p_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
								call_handler(ON_ERROR, p_httpc);
								root.close(hdoc);
							}
						} else if(method == HTTP_METHOD_HEAD) {
							p_httpc->res_code(HTTPRC_OK);
							root.close(hdoc);
						}
					} else {
						p_httpc->res_code(HTTPRC_NOT_FOUND);
						call_handler(ON_ERROR, p_httpc);
					}
				} else {
					p_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
					call_handler(ON_ERROR, p_httpc);
				}
			}
		}
	}

	_unlock();
}

