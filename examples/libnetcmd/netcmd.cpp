#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "startup.h"
#include "iNet.h"
#include "iCmd.h"
#include "iMemory.h"
#include "iSync.h"
#include "iLog.h"
#include "iArgs.h"

IMPLEMENT_BASE_ARRAY("libnetcmd", 1);

#define DEFAULT_PORT	3000
#define NC_LOG_PREFIX	"netcmd: "

typedef struct {
	bool prompt;
	iSocketIO *pi_io;
}_connection_t;

class cNetCmd: public iBase {
private:
	iNet *mpi_net;
	iCmdHost *mpi_cmd_host;
	iMutex *mpi_mutex;
	iLlist *mpi_list;
	iLog *mpi_log;
	HNOTIFY mhn_net, mhn_cmd_host;
	volatile bool m_running, m_stopped;
	_u32 m_port;
	iTCPServer *mpi_server;

	void release(iRepository *pi_repo, iBase **pp_obj) {
		if(*pp_obj) {
			HMUTEX hm = 0;

			if(*pp_obj != mpi_mutex && mpi_mutex)
				hm = mpi_mutex->lock();

			pi_repo->object_release(*pp_obj);
			*pp_obj = 0;

			if(hm)
				mpi_mutex->unlock(hm);
		}
	}

	void close_connections(void) {
		_u32 sz = 0;
		HMUTEX hm = mpi_list->lock();
		_connection_t *p_cnt = (_connection_t *)mpi_list->first(&sz, hm);

		while(p_cnt) {
			if(mpi_server) {
				mpi_log->fwrite(LMT_INFO, "%sClose connection 0x%x",
						NC_LOG_PREFIX, p_cnt->pi_io);
				mpi_server->close(p_cnt->pi_io);
			}
			mpi_list->del(hm); // remove current connection
			// next
			p_cnt = (_connection_t *)mpi_list->current(&sz, hm);
		}

		mpi_list->unlock(hm);
	}
public:
	BASE(cNetCmd, "cNetCmd", RF_ORIGINAL | RF_TASK, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				m_running = false;
				m_stopped = true;
				mpi_cmd_host = 0;
				mpi_net = 0;
				mpi_server = 0;
				mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
				mpi_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
				mpi_mutex = (iMutex *)pi_repo->object_by_iname(I_MUTEX, RF_CLONE);
				if(mpi_list && mpi_mutex && mpi_log) {
					iArgs *pi_arg = (iArgs *)pi_repo->object_by_iname(I_ARGS,
											RF_ORIGINAL);
					if(pi_arg) {
						_str_t port = pi_arg->value("netcmd-port");
						if(port)
							m_port = atoi(port);
						else {
							m_port = DEFAULT_PORT;
							mpi_log->fwrite(LMT_WARNING,
								"%srequire '--netcmd-port' argument in command line."
								" Defaulting listen port to %d",
								NC_LOG_PREFIX, m_port);
						}

						pi_repo->object_release(pi_arg);
					}

					mpi_list->init(LL_VECTOR, 1);

					mhn_net = pi_repo->monitoring_add(0, I_NET,
									0, this, SCAN_ORIGINAL);
					mhn_cmd_host = pi_repo->monitoring_add(0, I_CMD_HOST,
									0, this, SCAN_ORIGINAL);

					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_log->fwrite(LMT_INFO, "%s"
						"Uninit", NC_LOG_PREFIX);
				pi_repo->monitoring_remove(mhn_net);
				pi_repo->monitoring_remove(mhn_cmd_host);

				close_connections();
				release(pi_repo, (iBase **)&mpi_list);
				release(pi_repo, (iBase **)&mpi_server);
				release(pi_repo, (iBase **)&mpi_net);
				release(pi_repo, (iBase **)&mpi_cmd_host);
				release(pi_repo, (iBase **)&mpi_log);
				release(pi_repo, (iBase **)&mpi_mutex);
				r = true;
			} break;
			case OCTL_START: {
				m_stopped = false;
				m_running = true;

				while(m_running) {
					HMUTEX hm = mpi_mutex->lock();

					if(mpi_server) {
						iSocketIO *pi_io = mpi_server->listen();

						if(pi_io) {
							_connection_t cnt = {true, pi_io};

							pi_io->blocking(false); // non blocking I/O
							mpi_list->add(&cnt, sizeof(_connection_t));
							mpi_log->fwrite(LMT_INFO, "%sIncoming connection 0x%x",
									NC_LOG_PREFIX, pi_io);
						}

						_u32 sz = 0;
						HMUTEX hml = mpi_list->lock();
						_connection_t *p_cnt = (_connection_t *)mpi_list->first(&sz, hml);

						while(p_cnt) {
							if(p_cnt->pi_io->alive() == false) {
								mpi_log->fwrite(LMT_INFO, "%sClose connection 0x%x",
										NC_LOG_PREFIX, p_cnt->pi_io);
								mpi_server->close(p_cnt->pi_io);
								mpi_list->del(hml);
								p_cnt = (_connection_t *)mpi_list->current(&sz, hml);
								continue;
							} else {
								_char_t buffer[1024]="";

								if(p_cnt->prompt) {
									p_cnt->pi_io->write((_str_t)"netcmd: ", 8);
									p_cnt->prompt = false;
								}
								_u32 len = p_cnt->pi_io->read(buffer, sizeof(buffer));

								if(len) {
									if(mpi_cmd_host) {
										// needed to unlock
										mpi_list->unlock(hml);
										mpi_mutex->unlock(hm);
										mpi_cmd_host->exec(buffer, p_cnt->pi_io);
										// lock again
										hml = mpi_list->lock();
										hm = mpi_mutex->lock();
									}

									p_cnt->prompt = true;
								}

								p_cnt = (_connection_t *)mpi_list->next(&sz, hml);
							}
						}

						mpi_list->unlock(hml);
					}

					mpi_mutex->unlock(hm);
					usleep(10000);
				}

				r = m_stopped = true;
			} break;
			case OCTL_STOP: {
				if(m_running) {
					_u32 t = 10;

					m_running = false;
					while(!m_stopped && t) {
						t--;
						usleep(10000);
					}
				}
				r = m_stopped;
			} break;
			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				_object_info_t oi;

				memset(&oi, 0, sizeof(_object_info_t));
				if(pn->object)
					pn->object->object_info(&oi);

				if(pn->flags & NF_INIT) { // catch
					if(strcmp(oi.iname, I_CMD_HOST) == 0) {
						mpi_cmd_host = (iCmdHost *)pn->object;
						mpi_log->fwrite(LMT_INFO, "%s"
								"Catch command host", NC_LOG_PREFIX);
					}
					if(strcmp(oi.iname, I_NET) == 0) {
						mpi_net = (iNet *)pn->object;
						mpi_log->fwrite(LMT_INFO, "%s"
								"Catch networking", NC_LOG_PREFIX);
						// Create server
						if(mpi_net) {
							if((mpi_server = mpi_net->create_tcp_server(m_port))) {
								mpi_log->fwrite(LMT_INFO, "%s"
										"Create TCP server on port %d",
										NC_LOG_PREFIX, m_port);
								// waiting for connections in non blocking mode
								mpi_server->blocking(false);
							} else
								mpi_log->fwrite(LMT_ERROR, "%s"
										"Failed to create TCP server on port %d",
										NC_LOG_PREFIX, m_port);
						}
					}
				} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
					if(strcmp(oi.iname, I_CMD_HOST) == 0) {
						release(_gpi_repo_, (iBase **)&mpi_cmd_host);
						mpi_log->fwrite(LMT_INFO, "%s"
								"Release command host", NC_LOG_PREFIX);
					}
					if(strcmp(oi.iname, I_NET) == 0) {
						close_connections();

						// Close server
						mpi_log->fwrite(LMT_INFO, "%sClose server", NC_LOG_PREFIX);
						release(_gpi_repo_, (iBase **)&mpi_server);

						release(_gpi_repo_, (iBase **)&mpi_net);
						mpi_log->fwrite(LMT_INFO, "%s"
								"Release networking", NC_LOG_PREFIX);
					}
				}
			} break;
		}

		return r;
	}
};

static cNetCmd _g_net_cmd_;
