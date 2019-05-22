#include <string.h>
#include "startup.h"
#include "iGatn.h"
#include "iLog.h"
#include "private.h"

IMPLEMENT_BASE_ARRAY("libgatn", 10);

class cGatn: public iGatn {
private:
	iMap		*mpi_map;// server map
	iNet		*mpi_net;
	iFS		*mpi_fs;
	iLog		*mpi_log;
	iBufferMap	*mpi_bmap;
	iHeap		*mpi_heap;

	void stop(bool autorestore=false) { // stop servers
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				server *p = dynamic_cast<server *>(p_srv);

				if(p) {
					p->stop();
					p->mpi_net = NULL;
					p->mpi_fs = NULL;
					p->m_autorestore = autorestore;
					p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
				}
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}

	void restore(void) {
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				server *p = dynamic_cast<server *>(p_srv);

				if(p) {
					p->mpi_net = mpi_net;
					p->mpi_fs = mpi_fs;
					if(p->m_autorestore)
						p->start();
					p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
				}
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}

	void destroy(void) {
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				server *p = dynamic_cast<server *>(p_srv);

				if(p) {
					mpi_log->fwrite(LMT_INFO, "Gatn: stop server '%s'", p->m_name);
					_gpi_repo_->object_release(p->mpi_server);
					p->mpi_server = NULL;
					_gpi_repo_->object_release(p->mpi_map);
					_gpi_repo_->object_release(p->mpi_fcache);
					p->mpi_fcache = NULL;
				}
				p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}
public:
	BASE(cGatn, "cGatn", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				if((mpi_map = dynamic_cast<iMap *>(pi_repo->object_by_iname(I_MAP, RF_CLONE))))
					mpi_map->init(31);
				mpi_log = dynamic_cast<iLog *>(pi_repo->object_by_iname(I_LOG, RF_ORIGINAL));
				mpi_bmap = dynamic_cast<iBufferMap *>(pi_repo->object_by_iname(I_BUFFER_MAP, RF_CLONE));
				mpi_heap = dynamic_cast<iHeap *>(pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL));
				mpi_net = NULL;
				mpi_fs = NULL;
				init_mime_type_resolver();
				if(mpi_map && mpi_log && mpi_bmap && mpi_heap) {
					pi_repo->monitoring_add(NULL, I_NET, NULL, this, SCAN_ORIGINAL);
					pi_repo->monitoring_add(NULL, I_FS, NULL, this, SCAN_ORIGINAL);

					mpi_bmap->init(GATN_BUFFER_SIZE, [](_u8 op, void *bptr, _u32 sz, void *udata)->_u32 {
						_u32 r = 0;

						if(op == BIO_INIT)
							memset(bptr, 0, sz);

						return r;
					});
					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				destroy();
				pi_repo->object_release(mpi_map);
				pi_repo->object_release(mpi_log);
				pi_repo->object_release(mpi_bmap);
				pi_repo->object_release(mpi_heap);
				pi_repo->object_release(mpi_fs);
				pi_repo->object_release(mpi_net);
				uninit_mime_type_resolver();
				r = true;
			} break;
			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				_object_info_t oi;

				memset(&oi, 0, sizeof(_object_info_t));
				if(pn->object) {
					pn->object->object_info(&oi);

					if(pn->flags & NF_INIT) { // catch
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: catch networking");
							mpi_net = (iNet *)_gpi_repo_->object_by_handle(pn->hobj, RF_ORIGINAL|RF_NONOTIFY);
							restore();
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: catch FS support");
							mpi_fs = (iFS *)_gpi_repo_->object_by_handle(pn->hobj, RF_ORIGINAL|RF_NONOTIFY);
							restore();
						}
					} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: release networking");
							stop(true);
							_gpi_repo_->object_release(mpi_net);
							mpi_net = NULL;
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: release FS support");
							stop(true);
							_gpi_repo_->object_release(mpi_fs);
							mpi_fs = NULL;
						}
					}
				}
			} break;
		}

		return r;
	}

	_server_t *create_server(_cstr_t name, _u32 port,
				_cstr_t doc_root,
				_cstr_t cache_path,
				_u32 buffer_size=8192,
				_u32 max_workers=32,
				_u32 max_connections=500,
				_u32 connection_timeout=10,
				SSL_CTX *ssl_context=NULL
				) {
		_server_t *r = NULL;
		_u32 sz = 0;

		if(!(r = (_server_t *)mpi_map->get(name, strlen(name), &sz))) {
			if(mpi_map && mpi_net && mpi_fs) {
				server srv;

				srv.mpi_server = NULL;
				memset(srv.m_event, 0, sizeof(srv.m_event));
				memset(srv.m_doc_root, 0, sizeof(srv.m_doc_root));
				memset(srv.m_cache_path, 0, sizeof(srv.m_cache_path));
				strncpy(srv.m_name, name, MAX_SERVER_NAME-1);
				strncpy(srv.m_doc_root, doc_root, MAX_DOC_ROOT_PATH-1);
				strncpy(srv.m_cache_path, cache_path, MAX_CACHE_PATH-1);
				srv.m_port = port;
				srv.mpi_net = mpi_net;
				srv.mpi_fs = mpi_fs;
				srv.mpi_log = mpi_log;
				srv.mpi_heap = mpi_heap;
				srv.m_autorestore = false;
				srv.mpi_bmap = mpi_bmap;
				srv.m_buffer_size = buffer_size;
				srv.mpi_fcache = NULL;
				srv.m_max_workers = max_workers;
				srv.m_max_connections = max_connections;
				srv.m_connection_timeout = connection_timeout;
				srv.m_ssl_context = ssl_context;
				if((srv.mpi_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE))))
					if(srv.mpi_map->init(63))
						r = (_server_t *)mpi_map->add(name, strlen(name), &srv, sizeof(server));

				if(r)
					r->start();
				else {
					if(srv.mpi_map)
						_gpi_repo_->object_release(srv.mpi_map);
				}
			}
		}

		return r;
	}

	_server_t *server_by_name(_cstr_t name) {
		_u32 sz = 0;
		_server_t *r = (_server_t *)mpi_map->get(name, strlen(name), &sz);
		return r;
	}

	void remove_server(_server_t *p_srv) {
		server *p = dynamic_cast<server *>(p_srv);

		if(p) {
			_gpi_repo_->object_release(p->mpi_server);
			_gpi_repo_->object_release(p->mpi_map);
			_gpi_repo_->object_release(p->mpi_fcache);
			mpi_map->del(p->m_name, strlen(p->m_name));
		}
	}

	bool stop_server(_server_t *p_srv) {
		bool r = false;
		server *ps = dynamic_cast<server *>(p_srv);

		if(ps) {
			ps->stop();
			r = !ps->is_running();
		}

		return r;
	}

	bool start_server(_server_t *p_srv) {
		bool r = false;
		server *ps = dynamic_cast<server *>(p_srv);

		if(ps)
			r = ps->start();

		return r;
	}
};

static cGatn _g_gatn_;
