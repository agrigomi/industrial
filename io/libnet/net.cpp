#include "startup.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "private.h"

IMPLEMENT_BASE_ARRAY(10)

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

				if(mpi_heap) {
					if((p_caddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in))))
						memset(p_caddr, 0, sizeof(struct sockaddr_in));
				}
				p_caddr->sin_family = AF_INET;
				p_caddr->sin_addr.s_addr = inet_addr(dst_ip);
				p_caddr->sin_port = htons((unsigned short)port);
				pcsio->_init(0, p_caddr, sfd, SOCKET_IO_UDP);
				r = pcsio;
			} else
				::close(sfd);
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
		//...
		return r;
	}

	iTCPClient *create_tcp_client(_str_t dst_ip, _u32 port) {
		iTCPClient *r = 0;
		//...
		return r;
	}
};

static cNet _g_net_;
