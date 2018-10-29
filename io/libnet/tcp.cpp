#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"

bool cTCPServer::_init(_u32 port) {
	bool r = false;

	if((m_server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0) {
		m_port = port;
		m_serveraddr.sin_family = AF_INET;
		m_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_serveraddr.sin_port = htons((unsigned short)port);
		if(bind(m_server_socket, (struct sockaddr *)&m_serveraddr, sizeof(m_serveraddr)) >= 0) {
			if(::listen(m_server_socket, 10) >= 0)
				r = true;
			else
				::close(m_server_socket);
		} else
			::close(m_server_socket);

		if(!r)
			m_server_socket = 0;
	}

	return r;
}

void cTCPServer::_close(void) {
	if(m_server_socket) {
		::close(m_server_socket);
		m_server_socket = 0;
		destroy_ssl();
	}
}

bool cTCPServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;
			mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
			memset(&m_serveraddr, 0, sizeof(struct sockaddr_in));
			m_server_socket = 0;
			m_use_ssl = false;
			mp_sslcxt = 0;
			r = true;
		} break;
		case OCTL_UNINIT: {
			_close();
			iRepository *pi_repo = (iRepository *)arg;
			pi_repo->object_release(mpi_heap);
			r = true;
		} break;
	}

	return r;
}

void cTCPServer::init_ssl(void) {
	if(!mp_sslcxt) {
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		mp_sslcxt = SSL_CTX_new(SSLv23_server_method());
	}
}

void cTCPServer::destroy_ssl(void) {
	if(mp_sslcxt) {
		SSL_CTX_free(mp_sslcxt);
		mp_sslcxt = 0;
		ERR_free_strings();
		EVP_cleanup();
	}
}

bool cTCPServer::enable_ssl(bool enable,  _ulong options) {
	bool r = false;

	if(enable) {
		init_ssl();
		if(mp_sslcxt) {
			if(options)
				SSL_CTX_set_options(mp_sslcxt, options);
			r = true;
			m_use_ssl = true;
		}
	} else {
		destroy_ssl();
		if(!mp_sslcxt) {
			r = true;
			m_use_ssl = false;
		}
	}

	return r;
}

bool cTCPServer::ssl_use(_cstr_t str, _u32 type) {
	bool r = false;
	_s32 s = 0;

	if(mp_sslcxt) {
		switch(type) {
			case SSL_CERT_ASN1:
				s = SSL_CTX_use_certificate_ASN1(mp_sslcxt, strlen(str), (const unsigned char *)str);
				break;
			case SSL_CERT_PEM:
				break;
			case SSL_CERT_ASN1_FILE:
				s = SSL_CTX_use_certificate_file(mp_sslcxt, str, SSL_FILETYPE_ASN1);
				break;
			case SSL_CERT_PEM_FILE:
				s = SSL_CTX_use_certificate_file(mp_sslcxt, str, SSL_FILETYPE_PEM);
				break;
			case SSL_CERT_CHAIN_FILE:
				s = SSL_CTX_use_certificate_chain_file(mp_sslcxt, str);
				break;
			case SSL_PKEY_ASN1:
			case SSL_PKEY_PEM:
				break;
			case SSL_PKEY_ASN1_FILE:
				s = SSL_CTX_use_PrivateKey_file(mp_sslcxt, str, SSL_FILETYPE_ASN1);
				break;
			case SSL_PKEY_PEM_FILE:
				s = SSL_CTX_use_PrivateKey_file(mp_sslcxt, str, SSL_FILETYPE_PEM);
				break;
		}

		if(s == 1)
			r = true;
	}

	return r;
}

iSocketIO *cTCPServer::listen(void) {
	iSocketIO *r = 0;

	if(m_server_socket) {
		struct sockaddr_in caddr;

		memset(&caddr, 0, sizeof(struct sockaddr_in));

		socklen_t addrlen = sizeof(struct sockaddr_in);
		_s32 connect_socket = accept(m_server_socket, (struct sockaddr *)&caddr, &addrlen);

		if(connect_socket > 0) {
			struct sockaddr_in *p_caddr = 0;

			if(mpi_heap)
				p_caddr = (struct sockaddr_in *)mpi_heap->alloc(sizeof(struct sockaddr_in));

			if(p_caddr) {
				memcpy(p_caddr, &caddr, sizeof(struct sockaddr_in));

				cSocketIO *psio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);
				if(psio) {
					if(psio->_init(0, p_caddr, connect_socket,
							(m_use_ssl && mp_sslcxt) ? SOCKET_IO_SSL : SOCKET_IO_TCP,
							mp_sslcxt))
						r = psio;
					else
						/* we assume that socket I/O object should release memory and
						 should close socket handle */
						_gpi_repo_->object_release(psio);
				} else {
					mpi_heap->free(p_caddr, sizeof(struct sockaddr_in));
					::close(connect_socket);
				}
			} else
				::close(connect_socket);
		}
	}

	return r;
}

void cTCPServer::blocking(bool mode) { /* blocking or nonblocking IO */
	if(m_server_socket) {
		_s32 flags = fcntl(m_server_socket, F_GETFL, 0);
		if(flags != -1) {
			flags = (mode) ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
			fcntl(m_server_socket, F_SETFL, flags);
		}
	}
}

void cTCPServer::close(iSocketIO *p_io) {
	cSocketIO *pcsio = dynamic_cast<cSocketIO *>(p_io);

	if(pcsio) {
		pcsio->_close();
		_gpi_repo_->object_release(p_io);
	}
}

static cTCPServer _g_tcp_server_;
