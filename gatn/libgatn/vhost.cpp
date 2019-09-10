#include <string.h>
#include "err.h"
#include "private.h"
#include "iRepository.h"

typedef struct {
	iGatnExtension	*pi_ext;
	bool		active;
}_class_t;

bool vhost::init(_cstr_t name, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_key, _cstr_t cache_exclude,
		_cstr_t root_exclude, iHeap *pi_heap) {
	bool r = false;

	//...

	return r;
}

void vhost::destroy(void) {
	lock();

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
}

vhost::vhost() {
	host[0] = 0;
	pi_route_map = pi_class_map = NULL;
	pi_mutex = NULL;
	m_hlock = 0;
	memset(event, 0, sizeof(event));
}

iMutex *vhost::get_mutex(void) {
	if(!pi_mutex)
		pi_mutex = dynamic_cast<iMutex *>(_gpi_repo_->object_by_iname(I_MUTEX, RF_CLONE|RF_NONOTIFY));
	return pi_mutex;
}

iMap *get_route_map(void) {
	if(!pi_route_map) {
		if((pi_route_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			pi_route_map->init(31, pi_heap);
	}
	return pi_route_map;
}

iMap *get_class_map(void) {
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

void vhost::start_extensions(void) {
	//...
}

void vhost::stop_extensions(void) {
	//...
}

void vhost::start(void) {
	//...
}

void vhost::stop(void) {
	//...
}

void vhost::add_route_handler(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata) {
	//...
}

void vhost::remove_route_handler(_u8 method, _cstr_t path) {
	//...
}

_err_t vhost::add_class(_cstr_t cname, _cstr_t options) {
	_err_t r = ERR_UNKNOWN;

	//...

	return r;
}

bool vhost::remove_class(_cstr_t cname) {
	bool r = false;

	//...

	return r;
}

void vhost::start_class(_cstr_t cname) {
	//...
}

void vhost::stop_class(_cstr_t cname) {
	//...
}

