#include <string.h>
#include "private.h"

typedef struct {
	iBase	*monitored;
	_cstr_t	iname;
	_cstr_t	cname;
	iBase	*handler;
}_mon_rec_t;

typedef std::vector<_mon_rec_t, zAllocator<_mon_rec_t>> _v_mon_t;
typedef _v_mon_t::iterator	_v_it_mon_t; // monitoring iterator

static  _v_mon_t _gv_mon_; // monitoring vector


HNOTIFY add_monitoring(iBase *mon_object, _cstr_t iname, _cstr_t cname, iBase *pi_handler) {
	_mon_rec_t rec;
	_u32 sz_iname = (iname) ? strlen(iname) : 0;
	_u32 sz_cname = (cname) ? strlen(cname) : 0;

	memset(&rec, 0, sizeof(_mon_rec_t));

	rec.monitored = mon_object;
	if(iname && sz_iname) {
		if((rec.iname = (_cstr_t)zalloc(sz_iname + 1)))
			strcpy((_str_t)rec.iname, iname);
	}
	if(cname && sz_cname) {
		if((rec.cname = (_cstr_t)zalloc(sz_cname + 1)))
			strcpy((_str_t)rec.cname, cname);
	}
	rec.handler = pi_handler;

	_gv_mon_.push_back(rec);

	return &_gv_mon_.back();
}

static void clean_record(_mon_rec_t *p_rec) {
	_u32 sz_iname = (p_rec->iname) ? strlen(p_rec->iname) + 1 : 0;
	_u32 sz_cname = (p_rec->cname) ? strlen(p_rec->cname) + 1 : 0;

	if(sz_iname)
		zfree((void *)p_rec->iname, sz_iname);
	if(sz_cname)
		zfree((void *)p_rec->cname, sz_cname);
}

void remove_monitoring(HNOTIFY hn) {
	_v_it_mon_t it = _gv_mon_.begin();

	while(it != _gv_mon_.end()) {
		_mon_rec_t *p = &*it;

		if(p == hn) {
			clean_record(p);
			_gv_mon_.erase(it);
			break;
		}

		it++;
	}
}

void remove_monitoring(iBase *pi_handler) {
	_v_it_mon_t it = _gv_mon_.begin();

	while(it != _gv_mon_.end()) {
		_mon_rec_t *p = &*it;

		if(p->handler == pi_handler) {
			clean_record(p);
			_gv_mon_.erase(it);
			break;
		}

		it++;
	}
}

void enum_monitoring(iBase *pi_obj, _monitoring_enum_cb_t *pcb, void *udata) {
	_v_it_mon_t it = _gv_mon_.begin();
	_object_info_t oi;

	pi_obj->object_info(&oi);

	while(it != _gv_mon_.end()) {
		_mon_rec_t *p = &*it;
		bool notify = false;

		if(p->iname) {
			if(strcmp(p->iname, oi.iname) == 0)
				notify = true;
		}
		if(p->cname) {
			if(strcmp(p->cname, oi.cname) == 0)
				notify = true;
		}
		if(p->monitored == pi_obj)
			notify = true;

		if(notify)
			pcb(pi_obj, p->handler, udata);

		it++;
	}
}

void destroy_monitoring_storage(void) {
	_v_it_mon_t it = _gv_mon_.begin();

	while(it != _gv_mon_.end()) {
		_mon_rec_t *p = &*it;

		clean_record(p);
	}

	_gv_mon_.clear();
}

