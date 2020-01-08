#include <string.h>
#include <time.h>
#include "iGatn.h"
#include "iArgs.h"
#include "iLog.h"
#include "iRepository.h"

#define MAX_EVENTS	16

typedef struct {
	_gatn_http_event_t	*p_cb;
	void			*udata;
}_http_evt_t;

class cHttpLog: public iGatnExtension {
private:
	iArgs		*mpi_args;
	iLog		*mpi_log;
	_http_evt_t	m_original_evt[MAX_EVENTS]; // backup events
	_server_t	*mpi_gatn_server;
	_char_t		m_host_name[256];
	bool		e, i, t;
	_cstr_t		m_time_fmt;

	void timestamp(_str_t buffer, _u32 len) {
		memset(buffer, 0, len);

		if(m_time_fmt) {
			time_t _time = time(NULL);
			tm *_tm = localtime(&_time);

			strftime(buffer, len, m_time_fmt, _tm);
		}
	}

	void backup_handlers(_server_t *psrv, _cstr_t host) {
		if(i)
			m_original_evt[ON_REQUEST].p_cb = psrv->get_event_handler(ON_REQUEST, &(m_original_evt[ON_REQUEST].udata), host);
		if(t)
			m_original_evt[ON_DATA].p_cb = psrv->get_event_handler(ON_DATA, &(m_original_evt[ON_DATA].udata), host);
		if(e)
			m_original_evt[ON_ERROR].p_cb = psrv->get_event_handler(ON_ERROR, &(m_original_evt[ON_ERROR].udata), host);
	}

	void set_handlers(_server_t *psrv, _cstr_t host) {
		if(i) {
			psrv->on_event(ON_REQUEST, [](_request_t *req, _response_t *res, void *udata)->_s32 {
				_s32 r = EHR_CONTINUE;
				cHttpLog *pobj = (cHttpLog *)udata;
				iLog *pi_log = pobj->mpi_log;
				_cstr_t method = req->var(VAR_REQ_METHOD);
				_cstr_t uri = req->var(VAR_REQ_URI);
				_cstr_t protocol = req->var(VAR_REQ_PROTOCOL);
				_char_t ip[32];
				_char_t tm[64];

				req->connection()->peer_ip(ip, sizeof(ip));
				pobj->timestamp(tm, sizeof(tm));

				pi_log->fwrite(LMT_INFO, "%s %s/%s: (%s) %s %s %s",
							tm,
							pobj->mpi_gatn_server->name(),
							(strlen(pobj->m_host_name)) ? pobj->m_host_name: "defaulthost",
							ip,
							method, protocol, uri);

				_u32 size = 0;
				_cstr_t data = (_cstr_t)req->data(&size);

				if(data) {
					pi_log->write(LMT_TEXT, data);
					req->parse_content();
				}

				r = pobj->call_original_handler(ON_REQUEST, req, res);

				return r;
			}, this, host);
		}

		if(t) {
			psrv->on_event(ON_DATA, [](_request_t *req, _response_t *res, void *udata)->_s32 {
				_s32 r = EHR_CONTINUE;
				cHttpLog *pobj = (cHttpLog *)udata;
				iLog *pi_log = pobj->mpi_log;
				_u32 size = 0;
				_cstr_t data = (_cstr_t)req->data(&size);

				if(data)
					pi_log->write(LMT_TEXT, data);

				r = pobj->call_original_handler(ON_DATA, req, res);
				return r;
			}, this, host);
		}

		if(e) {
			psrv->on_event(ON_ERROR, [](_request_t *req, _response_t *res, void *udata)->_s32 {
				_s32 r = EHR_CONTINUE;
				cHttpLog *pobj = (cHttpLog *)udata;
				_u16 rc = res->error();

				if(rc != HTTPRC_REQUEST_TIMEOUT) {
					iLog *pi_log = pobj->mpi_log;
					_cstr_t uri = req->var(VAR_REQ_URI);
					_char_t ip[32];
					_char_t tm[64];
					_cstr_t rc_text = res->text(rc);

					req->connection()->peer_ip(ip, sizeof(ip));
					pobj->timestamp(tm, sizeof(tm));

					pi_log->fwrite(LMT_ERROR, "%s %s/%s: (%s) ERROR(%d) %s %s",
								tm,
								pobj->mpi_gatn_server->name(),
								(strlen(pobj->m_host_name)) ? pobj->m_host_name: "defaulthost",
								ip, rc, rc_text,
								 (uri) ? uri : "");
				}

				r = pobj->call_original_handler(ON_ERROR, req, res);
				return r;
			}, this, host);
		}
	}

	void restore_handlers(_server_t *psrv, _cstr_t host) {
		if(i)
			psrv->on_event(ON_REQUEST, m_original_evt[ON_REQUEST].p_cb, m_original_evt[ON_REQUEST].udata, host);
		if(t)
			psrv->on_event(ON_DATA, m_original_evt[ON_DATA].p_cb, m_original_evt[ON_DATA].udata, host);
		if(e)
			psrv->on_event(ON_ERROR, m_original_evt[ON_ERROR].p_cb, m_original_evt[ON_ERROR].udata, host);
	}

	_s32 call_original_handler(_u8 evt, _request_t *req, _response_t *res) {
		_s32 r = EHR_CONTINUE;

		if(evt < MAX_EVENTS) {
			if(m_original_evt[evt].p_cb)
				r = m_original_evt[evt].p_cb(req, res, m_original_evt[evt].udata);
		}

		return r;
	}
public:
	BASE(cHttpLog, "cHttpLog", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				memset(m_original_evt, 0, sizeof(m_original_evt));
				memset(m_host_name, 0, sizeof(m_host_name));

				mpi_args = dynamic_cast<iArgs *>(_gpi_repo_->object_by_iname(I_ARGS, RF_CLONE|RF_NONOTIFY));
				mpi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL));

				if(mpi_args && mpi_log)
					r = true;
				break;
			case OCTL_UNINIT:
				_gpi_repo_->object_release(mpi_args, false);
				_gpi_repo_->object_release(mpi_log, false);
				r = true;
				break;
		}

		return r;
	}

	bool options(_cstr_t opt) {
		bool r = false;

		e = i = t = false;

		if(opt) {
			if((r = mpi_args->init(opt, strlen(opt)))) {
				e = mpi_args->check('E');
				i = mpi_args->check('I');
				t = mpi_args->check('T');
				m_time_fmt = mpi_args->value("time-format");
			}
		}

		return r;
	}

	bool attach(_server_t *p_srv, _cstr_t host=NULL) {
		bool r = true;

		memset(m_original_evt, 0, sizeof(m_original_evt));
		memset(m_host_name, 0, sizeof(m_host_name));

		mpi_gatn_server = p_srv;
		if(host)
			strncpy(m_host_name, host, sizeof(m_host_name)-1);
		backup_handlers(p_srv, host);
		set_handlers(p_srv, host);

		return r;
	}

	void detach(_server_t *p_srv, _cstr_t host=NULL) {
		restore_handlers(p_srv, host);
	}
};

static cHttpLog _g_http_log_;
