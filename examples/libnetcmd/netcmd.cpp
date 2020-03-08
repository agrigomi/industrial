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

IMPLEMENT_BASE_ARRAY("netcmd", 1);

#define DEFAULT_PORT	3001
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

					r = mpi_list->init(LL_VECTOR, 1);
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_log->fwrite(LMT_INFO, "%s"
						"Uninit", NC_LOG_PREFIX);

				close_connections();
				release(pi_repo, (iBase **)&mpi_server);
				r = true;
			} break;
			case OCTL_START: {
				m_stopped = false;
				m_running = true;

				mpi_log->fwrite(LMT_INFO, "%sStart thread", NC_LOG_PREFIX);
				while(m_running) {
					HMUTEX hm = mpi_mutex->lock();

					if(mpi_server) {
						iSocketIO *pi_io = mpi_server->listen();

						if(pi_io) {
							_connection_t cnt = {true, pi_io};

							pi_io->blocking(false); // non blocking I/O
							_connection_t *pcnt = (_connection_t *)mpi_list->add(&cnt, sizeof(_connection_t));
							if(pcnt) {
								_str_t msg = (_str_t)"!!! Welcome !!!\n";
								pcnt->pi_io->write(msg, strlen(msg));
							}
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
									if(mpi_cmd_host && len > 1) {
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
					usleep(100000);
				}

				r = m_stopped = true;
			} break;
			case OCTL_STOP: {
				if(m_running) {
					_u32 t = 10;

					mpi_log->fwrite(LMT_INFO, "%sStop thread", NC_LOG_PREFIX);
					m_running = false;
					while(!m_stopped && t) {
						t--;
						usleep(10000);
					}
				}
				r = m_stopped;
			} break;
		}

		return r;
	}

BEGIN_LINK_MAP
	LINK(mpi_list, I_LLIST, NULL, RF_CLONE, NULL, NULL),
	LINK(mpi_log, I_LOG, NULL, RF_ORIGINAL, NULL, NULL),
	LINK(mpi_mutex, I_MUTEX, NULL, RF_CLONE, NULL, NULL),
	LINK(mpi_cmd_host, I_CMD_HOST, NULL, RF_ORIGINAL|RF_PLUGIN, NULL, NULL),
	LINK(mpi_net, I_NET, NULL, RF_ORIGINAL|RF_PLUGIN, [](_u32 n, void *udata) {
		cNetCmd *p = (cNetCmd *)udata;

		if(n == RCTL_REF) {
			p->mpi_log->fwrite(LMT_INFO, "%s"
					"Attach networking", NC_LOG_PREFIX);
			// Create server
			if(p->mpi_net) {
				if((p->mpi_server = p->mpi_net->create_tcp_server(p->m_port))) {
					p->mpi_log->fwrite(LMT_INFO, "%s"
							"Create TCP server on port %d",
							NC_LOG_PREFIX, p->m_port);
					// waiting for connections in non blocking mode
					p->mpi_server->blocking(false);
				} else
					p->mpi_log->fwrite(LMT_ERROR, "%s"
							"Failed to create TCP server on port %d",
							NC_LOG_PREFIX, p->m_port);
			}
		} else if(n == RCTL_UNREF)  {
				p->close_connections();

				// Close server
				p->mpi_log->fwrite(LMT_INFO, "%sClose server", NC_LOG_PREFIX);
				p->release(_gpi_repo_, (iBase **)&p->mpi_server);

				p->mpi_log->fwrite(LMT_INFO, "%s"
						"Detach networking", NC_LOG_PREFIX);
		}
	}, this)
END_LINK_MAP
};

static cNetCmd _g_net_cmd_;
