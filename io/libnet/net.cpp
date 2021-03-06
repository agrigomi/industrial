#include "startup.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "private.h"

IMPLEMENT_BASE_ARRAY("net", 10)

class cNet: public iNet {
private:
	_s32	m_optval;
	HOBJECT mh_sio;
public:
	BASE(cNet, "cNet", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				if((mh_sio = _gpi_repo_->handle_by_cname(CLASS_NAME_SOCKET_IO)))
					r = true;
			} break;
			case OCTL_UNINIT: {
				r = true;
			} break;
		}

		return r;
	}

	iSocketIO *create_udp_server(_u32 port) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_handle(mh_sio, RF_CLONE | RF_NONOTIFY);
			if(pcsio) {
				m_optval = 1;
				struct sockaddr_in saddr;
				struct sockaddr_in caddr;

				memset(&saddr, 0, sizeof(struct sockaddr_in));
				memset(&caddr, 0, sizeof(struct sockaddr_in));

				setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&m_optval, sizeof(m_optval));
				saddr.sin_family = AF_INET;
				saddr.sin_addr.s_addr = htonl(INADDR_ANY);
				saddr.sin_port = htons((unsigned short)port);
				if(bind(sfd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in)) >=0) {
					pcsio->_init(&saddr, &caddr, sfd, SOCKET_IO_UDP);
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

	iSocketIO *create_udp_client(_cstr_t dst_ip, _u32 port) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_handle(mh_sio, RF_CLONE | RF_NONOTIFY);
			if(pcsio) {
				struct sockaddr_in caddr;

				memset(&caddr, 0, sizeof(struct sockaddr_in));
				caddr.sin_family = AF_INET;
				caddr.sin_addr.s_addr = inet_addr(dst_ip);
				caddr.sin_port = htons((unsigned short)port);
				pcsio->_init(0, &caddr, sfd, SOCKET_IO_UDP);
				r = pcsio;
			} else
				::close(sfd);
		}

		return r;
	}

	iSocketIO *create_multicast_sender(_cstr_t group, _u32 port) {
		return create_udp_client(group, port);
	}

	iSocketIO *create_multicast_listener(_cstr_t group, _u32 port) {
		iSocketIO *r = 0;
		struct ip_mreq mreq;
		_s32 sfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sfd > 0) {
			struct sockaddr_in caddr;
			cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_handle(mh_sio, RF_CLONE | RF_NONOTIFY);

			if(pcsio) {
				_u32 opt = 1;
				if(setsockopt(sfd, SOL_SOCKET,SO_REUSEADDR, (char*)&opt, sizeof(opt)) >= 0) {
					memset(&caddr, 0, sizeof(struct sockaddr_in));
					caddr.sin_family = AF_INET;
					caddr.sin_addr.s_addr = htonl(INADDR_ANY);
					caddr.sin_port = htons((unsigned short)port);
					if(bind(sfd,  (struct sockaddr*)&caddr, sizeof(struct sockaddr_in)) >= 0) {
						memset(&mreq, 0, sizeof(struct ip_mreq));
						mreq.imr_multiaddr.s_addr = inet_addr(group);
						mreq.imr_interface.s_addr = htonl(INADDR_ANY);
						if (setsockopt(sfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *)&mreq, sizeof(mreq)) >= 0) {
							pcsio->_init(0, &caddr, sfd, SOCKET_IO_UDP);
							r = pcsio;
						}
					}
				}
			}

			if(!r) {
				::close(sfd);
				if(pcsio)
					_gpi_repo_->object_release(pcsio);
			}
		}

		return r;
	}

	void close_socket(iSocketIO *p_sio) {
		cSocketIO *pcsio = dynamic_cast<cSocketIO *>(p_sio);

		if(pcsio) {
			pcsio->_close();
			_gpi_repo_->object_release(p_sio);
		}
	}

	iTCPServer *create_tcp_server(_u32 port, SSL_CTX *ssl_context=NULL) {
		iTCPServer *r = 0;

		cTCPServer *pctcps = (cTCPServer *)_gpi_repo_->object_by_cname(CLASS_NAME_TCP_SERVER, RF_CLONE | RF_NONOTIFY);
		if(pctcps) {
			if(pctcps->_init(port, ssl_context))
				r = pctcps;
			else
				_gpi_repo_->object_release(pctcps);
		}

		return r;
	}

	iSocketIO *create_tcp_client(_cstr_t host, _u32 port, SSL_CTX *ssl_context=NULL) {
		iSocketIO *r = 0;
		_s32 sfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sfd > 0) {
			struct hostent *server;
			struct sockaddr_in saddr;

			server = gethostbyname(host);
			if(server) {
				memset(&saddr, 0, sizeof(struct sockaddr_in));
				saddr.sin_family = AF_INET;
				saddr.sin_addr = *((struct in_addr *)server->h_addr);
				saddr.sin_port = htons(port);
				if(connect(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) >= 0) {
					cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_handle(mh_sio, RF_CLONE | RF_NONOTIFY);
					if(pcsio) {
						if(ssl_context)
							pcsio->_init(&saddr, 0, sfd, SOCKET_IO_SSL_CLIENT, ssl_context);
						else
							pcsio->_init(&saddr, 0, sfd, SOCKET_IO_TCP);
						r = pcsio;
					}
				}
			}

			if(!r)
				::close(sfd);
		}

		return r;
	}

	iHttpServer *create_http_server(_u32 port, _u32 buffer_size=HTTP_BUFFER_SIZE,
					_u32 max_workers=HTTP_MAX_WORKERS,
					_u32 max_connections=HTTP_MAX_CONNECTIONS,
					_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
					SSL_CTX *ssl_context=NULL) {
		iHttpServer *r = 0;
		cHttpServer *chttps = (cHttpServer *)_gpi_repo_->object_by_cname(CLASS_NAME_HTTP_SERVER, RF_CLONE | RF_NONOTIFY);

		if(chttps) {
			if(chttps->_init(port, buffer_size, max_workers, max_connections, connection_timeout, ssl_context))
				r = chttps;
			else
				_gpi_repo_->object_release(chttps);
		}

		return r;
	}

	iHttpClientConnection *create_http_client(_cstr_t host,
						_u32 port,
						_u32 buffer_size=HTTP_BUFFER_SIZE,
						SSL_CTX *ssl_context=NULL) {
		iHttpClientConnection *r = 0;
		iSocketIO *pi_tcpc = create_tcp_client(host, port, ssl_context);

		if(pi_tcpc) {
			cHttpClientConnection *_r = 0;

			if((_r = dynamic_cast<cHttpClientConnection *>(_gpi_repo_->object_by_cname(CLASS_NAME_HTTP_CLIENT_CONNECTION, RF_CLONE | RF_NONOTIFY)))) {
				if(_r->_init(pi_tcpc, buffer_size))
					r = _r;
				else {
					_gpi_repo_->object_release(_r);
					_gpi_repo_->object_release(pi_tcpc);
				}
			} else
				_gpi_repo_->object_release(pi_tcpc);
		}

		return r;
	}
};

static cNet _g_net_;
