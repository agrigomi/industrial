#include "startup.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "private.h"

IMPLEMENT_BASE_ARRAY("net", 10)

class cNet: public iNet {
private:
	iHeap	*mpi_heap;
	_s32	m_optval;
public:
	BASE(cNet, "cNet", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;
				mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
				r = true;
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;
				pi_repo->object_release(mpi_heap);
				r = true;
			} break;
		}

		return r;
	}

	iSocketIO *create_udp_server(_u32 port) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);
			if(pcsio) {
				m_optval = 1;
				struct sockaddr_in *p_saddr = 0;
				struct sockaddr_in *p_caddr = 0;

				if(mpi_heap) {
					if((p_saddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in))))
						memset(p_saddr, 0, sizeof(struct sockaddr_in));
					if((p_caddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in))))
						memset(p_caddr, 0, sizeof(struct sockaddr_in));
				}

				setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&m_optval, sizeof(m_optval));
				p_saddr->sin_family = AF_INET;
				p_saddr->sin_addr.s_addr = htonl(INADDR_ANY);
				p_saddr->sin_port = htons((unsigned short)port);
				if(bind(sfd, (struct sockaddr *)p_saddr, sizeof(struct sockaddr_in)) >=0) {
					pcsio->_init(p_saddr, p_caddr, sfd, SOCKET_IO_UDP);
					r = pcsio;
				} else {
					::close(sfd);
					mpi_heap->free(p_saddr, sizeof(struct sockaddr_in));
					mpi_heap->free(p_caddr, sizeof(struct sockaddr_in));
					_gpi_repo_->object_release(pcsio);
				}
			} else
				::close(sfd);
		}

		return r;
	}

	iSocketIO *create_udp_client(_str_t dst_ip, _u32 port) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);
			if(pcsio) {
				struct sockaddr_in *p_caddr = 0;

				if(mpi_heap)
					p_caddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in));
				if(p_caddr) {
					memset(p_caddr, 0, sizeof(struct sockaddr_in));
					p_caddr->sin_family = AF_INET;
					p_caddr->sin_addr.s_addr = inet_addr(dst_ip);
					p_caddr->sin_port = htons((unsigned short)port);
					pcsio->_init(0, p_caddr, sfd, SOCKET_IO_UDP);
					r = pcsio;
				} else {
					::close(sfd);
					_gpi_repo_->object_release(pcsio);
				}
			} else
				::close(sfd);
		}

		return r;
	}

	iSocketIO *create_multicast_sender(_str_t group, _u32 port) {
		return create_udp_client(group, port);
	}

	iSocketIO *create_multicast_listener(_str_t group, _u32 port) {
		iSocketIO *r = 0;
		struct ip_mreq mreq;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			struct sockaddr_in *p_caddr = 0;
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);

			if(pcsio) {
				_u32 opt = 1;
				if(setsockopt(sfd, SOL_SOCKET,SO_REUSEADDR, (char*)&opt, sizeof(opt)) >= 0) {
					if(mpi_heap)
						p_caddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in));
					if(p_caddr) {
						memset(p_caddr, 0, sizeof(struct sockaddr_in));
						p_caddr->sin_family = AF_INET;
						p_caddr->sin_addr.s_addr = htonl(INADDR_ANY);
						p_caddr->sin_port = htons((unsigned short)port);
						if(bind(sfd, (struct sockaddr *)p_caddr, sizeof(struct sockaddr_in)) >= 0) {
							memset(&mreq, 0, sizeof(struct ip_mreq));
							mreq.imr_multiaddr.s_addr = inet_addr(group);
							mreq.imr_interface.s_addr = htonl(INADDR_ANY);
							if (setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *)&mreq, sizeof(mreq)) >= 0) {
								pcsio->_init(0, p_caddr, sfd, SOCKET_IO_UDP);
								r = pcsio;
							}
						}
					}
				}
			}

			if(!r) {
				::close(sfd);
				if(pcsio)
					_gpi_repo_->object_release(pcsio);

				if(p_caddr && mpi_heap)
					mpi_heap->free(p_caddr, sizeof(struct sockaddr_in));
			}
		}

		return r;
	}

	void close_socket(iSocketIO *p_sio) {
		cSocketIO *pcsio = dynamic_cast<cSocketIO *>(p_sio);

		if(pcsio) {
			struct sockaddr_in *p_saddr = pcsio->serveraddr();
			struct sockaddr_in *p_caddr = pcsio->clientaddr();

			pcsio->_close();

			if(p_saddr)
				mpi_heap->free(p_saddr, sizeof(struct sockaddr_in));
			if(p_caddr)
				mpi_heap->free(p_caddr, sizeof(struct sockaddr_in));

			_gpi_repo_->object_release(p_sio);
		}
	}

	iTCPServer *create_tcp_server(_u32 port) {
		iTCPServer *r = 0;

		cTCPServer *pctcps = (cTCPServer *)_gpi_repo_->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE);
		if(pctcps) {
			if(pctcps->_init(port))
				r = pctcps;
			else
				_gpi_repo_->object_release(pctcps);
		}

		return r;
	}

	iSocketIO *create_tcp_client(_str_t host, _u32 port, SSL_CTX *ssl_context=NULL) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sfd > 0) {
			struct hostent *server;
			struct sockaddr_in *p_saddr = 0;

			server = gethostbyname(host);
			if(server) {
				if(mpi_heap)
					p_saddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in));
				if(p_saddr) {
					memset(p_saddr, 0, sizeof(struct sockaddr_in));
					p_saddr->sin_family = AF_INET;
					p_saddr->sin_addr = *((struct in_addr *)server->h_addr);
					p_saddr->sin_port = htons(port);
					if(connect(sfd, (struct sockaddr *)p_saddr, sizeof(struct sockaddr_in)) >= 0) {
						cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);
						if(pcsio) {
							if(ssl_context)
								pcsio->_init(p_saddr, 0, sfd, SOCKET_IO_SSL_CLIENT, ssl_context);
							else
								pcsio->_init(p_saddr, 0, sfd, SOCKET_IO_TCP);
							r = pcsio;
						}
					}
				}
			}

			if(!r) {
				::close(sfd);
				if(p_saddr && mpi_heap)
					mpi_heap->free(p_saddr, sizeof(struct sockaddr_in));
			}
		}

		return r;
	}

	iHttpServer *create_http_server(_u32 port) {
		iHttpServer *r = 0;
		cHttpServer *chttps = (cHttpServer *)_gpi_repo_->object_by_cname(CLASS_NAME_HTTP_SERVER, RF_CLONE);

		if(chttps) {
			if(chttps->_init(port))
				r = chttps;
			else
				_gpi_repo_->object_release(chttps);
		}

		return r;
	}
};

static cNet _g_net_;
