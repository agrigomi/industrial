#include <string.h>
#include "err.h"
#include "private.h"
#include "iRepository.h"
#include "iLog.h"

typedef struct {
	iGatnExtension	*pi_ext;
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
	memset(event, 0, sizeof(event));
	pi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL);

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
	iMap pi_map = get_class_map();
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

			if(!pclass->active)
				pclass->active = pclass->pi_ext->attach(pa->pi_srv, pa->host);

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}

void vhost::stop_extensions(HMUTEX hlock) {
	iMap pi_map = get_class_map();
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

			if(pclass->active) {
				pclass->pi_ext->detach(pa->pi_srv, pa->host);
				pclass->active = false;
			}

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}

void vhost::remove_extensions(void) {
	iMap pi_map = get_class_map();
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

			if(pclass->active) {
				pclass->pi_ext->detach(pa->pi_srv, pa->host);
				pclass->active = false;
			}

			_gpi_repo_->object_release(pclass->pi_ext, false);

			return ENUM_NEXT;
		}, &tmp);
	}

	unlock(hm);
}
}

bool vhost::start(void) {
	bool r = false;
	HMUTEX hm = lock();

	if(!(r = root.is_enabled())) {
		start_extensions();
		root.start();
		m_running = root.is_enabled();
	}

	unlock(hm);
}

void vhost::stop(void) {
	HMUTEX hm = lock();

	if(root.is_enabled()) {
		stop_extensions(hm);
		clear_events();
		root.stop();
	}

	unlock(hm);
}

void vhost::add_route_handler(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};
	iMap *pi_map = get_route_map();

	if(pi_map) {
		lock();
		strncpy(data.path, path, sizeof(data.path)-1);
		memset(&key, 0, sizeof(_route_key_t));
		key.method = method;
		strncpy(key.path, path, MAX_ROUTE_PATH-1);
		pi_map->add(&key, key.size(), &data, data.size());
		_unlock();
	}
}

void vhost::remove_route_handler(_u8 method, _cstr_t path) {
	_route_key_t key;
	iMap *pi_map = get_route_map();

	if(pi_map) {
		_lock();
		memset(&key, 0, sizeof(_route_key_t));
		key.method = method;
		strncpy(key.path, path, MAX_ROUTE_PATH-1);
		pi_map->del(&key, key.size());
		_unlock();
	}
}

void vhost::set_event_handler(_u8 evt, _gatn_http_event_t *pcb, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		pvhost->event[evt].pcb = pcb;
		pvhost->event[evt].udata = udata;
	}
}

_gatn_http_event_t *vhost::get_event_handler(_u8 evt, void **pp_udata) {
	_gatn_http_event_t *r = NULL;

	if(evt < HTTP_MAX_EVENTS) {
		r = pvhost->event[evt].pcb;
		*pp_udata = pvhost->event[evt].udata;
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
					_class_t tmp;

					tmp.pi_ext = dynamic_cast<iGatnExtension *>(pi_base);

					if(tmp.pi_ext) {
						if((pclass = pi_map->add(cname, strlen(cname), &tmp, sizeof(tmp))) {
							pi_log->fwrite(LMT_INFO, "Gatn: Attach class '%s' to '%s/%s'",
									cname, pi_server->name(), host);
							if(options)
								pclass->pi_ext->options(options);

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
		} else
			pi_log->fwrite(LMT_ERROR, "Gatn: class '%s' already attached to '%s/%s'",
					cname, pi_server->name(), host);

		unlock(hm);
	}

	return r;
}

bool vhost::detach_class(_cstr_t cname) {
	bool r = false;
	iMap *pi_map = get_class_map();

	if(pi_map) {
		_u32 sz = 0;
		HMUTEX hm = lock();
		_class_t *pclass = (_class_t *)pi_map->get(cname, strlen(cname), &sz);

		if(pclass) {
			if(pclass->active) {
				pclass->pi_ext->detach(pi_server, host);
				pclass->active = false;
			}

			_gpi_repo_->object_release(pclass->pi_ext, false);
			pi_map->del(cname, strlen(cname));

			stop_extensions(hm);
			start_extensions(hm);
		}

		unlock(hm);
	}

	return r;
}

