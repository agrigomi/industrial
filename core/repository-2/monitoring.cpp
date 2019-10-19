#include <string.h>
#include "private.h"

typedef struct {
	_char_t	iname[MAX_INAME];
	_char_t	cname[MAX_CNAME];
	iBase	*handler;
}_mon_rec_t;

static _map_t _gm_mon_;

void add_monitoring(_cstr_t iname, _cstr_t cname, iBase *pi_handler) {
	_mon_rec_t rec;
	_u32 sz_iname = (iname) ? strlen(iname) : 0;
	_u32 sz_cname = (cname) ? strlen(cname) : 0;

	memset(&rec, 0, sizeof(_mon_rec_t));

	if(iname && sz_iname)
		strncpy((_str_t)rec.iname, iname, sizeof(rec.iname)-1);
	if(cname && sz_cname)
		strncpy((_str_t)rec.cname, cname, sizeof(rec.cname)-1);
	rec.handler = pi_handler;

	_gm_mon_.add(&rec, sizeof(rec), &rec, sizeof(rec));;
}

void remove_monitoring(iBase *pi_handler) {
	_gm_mon_.enm([](void *data, _u32 size, void *udata)->_s32 {
		_s32 r = ENUM_CONTINUE;
		iBase *pi_handler = (iBase *)udata;
		_mon_rec_t *p_rec = (_mon_rec_t *)data;

		if(p_rec->handler == pi_handler)
			r = ENUM_DELETE;

		return r;
	}, pi_handler);
}

void enum_monitoring(iBase *pi_handler, _monitoring_enum_cb_t *pcb, void *udata) {
	typedef struct {
		iBase *pi_handler;
		_monitoring_enum_cb_t *pcb;
		void *udata;
	}_enum_t;

	_enum_t e = {pi_handler, pcb, udata};

	_gm_mon_.enm([](void *data, _u32 size, void *udata)->_s32 {
		_enum_t *pe = (_enum_t *)udata;
		_mon_rec_t *p = (_mon_rec_t *)data;

		if(pe->pi_handler == p->handler)
			pe->pcb(p->iname, p->cname, pe->udata);

		return ENUM_CONTINUE;
	}, &e);
}

void destroy_monitoring_storage(void) {
	_gm_mon_.clr();
}

