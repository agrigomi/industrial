#ifndef __I_NET_H__
#define __I_NET_H__

#include "iIO.h"

#define I_SOCKET_IO	"iSocketIO"
#define I_NET		"iNet"
#define I_TCP_SERVER	"iTCPServer"

class iSocketIO: public iIO {
public:
	INTERFACE(iSocketIO, I_SOCKET_IO);
	virtual void blocking(bool)=0; /* blocking or nonblocking IO */
	virtual bool alive(void)=0;
};

#define SSL_CERT_ANSI1		1
#define SSL_CERT_PEM		2
#define SSL_CERT_ANSI1_FILE	3
#define SSL_CERT_PEM_FILE	4
#define SSL_CERT_CHAIN_FILE	5
#define SSL_PKEY_ANSI1		11
#define SSL_PKEY_PEM		12
#define SSL_PKEY_ANSI1_FILE	13
#define SSL_PKEY_PEM_FILE	14

class iTCPServer: public iBase {
public:
	INTERFACE(iTCPServer, I_TCP_SERVER);
	virtual iSocketIO *listen(void)=0;
	virtual bool enable_ssl(bool, _ulong options=0)=0;
	virtual bool ssl_use(_str_t str, _u32 type)=0;
	virtual void blocking(bool)=0; /* blocking or nonblocking IO */
	virtual void close(iSocketIO *p_io)=0;
};

class iNet: public iBase {
public:
	INTERFACE(iNet, I_NET);
	virtual iSocketIO *create_udp_server(_u32 port)=0;
	virtual iSocketIO *create_udp_client(_str_t dst_ip, _u32 port)=0;
	virtual iSocketIO *create_multicast_sender(_str_t group, _u32 port)=0;
	virtual iSocketIO *create_multicast_listener(_str_t group, _u32 port)=0;
	virtual void close_socket(iSocketIO *p_sio)=0;
	virtual iTCPServer *create_tcp_server(_u32 port)=0;
	virtual iSocketIO *create_tcp_client(_str_t host, _u32 port)=0;
};

#endif

