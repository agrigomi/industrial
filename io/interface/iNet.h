#ifndef __I_NET_H__
#define __I_NET_H__

#include "iIO.h"

#define I_NET		"iNet"
#define I_UDP_SERVER	"iUDPServer"
#define I_UDP_CLIENT	"iUDPClient"
#define I_TCP_SERVER	"iTCPServer"
#define I_TCP_CLIENT	"iTCPClient"

#define  _non_blocking_	false
#define _blocking_	true

class iTCPServer: public iBase {
public:
	INTERFACE(iTCPServer, I_TCP_SERVER);
	virtual iSocketIO *listen(bool blocking=_blocking_)=0;
	virtual void close(iSocketIO *p_io)=0;
};

class iTCPClient: public iBase {
public:
	INTERFACE(iTCPClient, I_TCP_CLIENT);
	virtual iSocketIO *connect(void)=0;
};

class iNet: public iBase {
public:
	INTERFACE(iNet, I_NET);
	virtual iSocketIO *create_udp_server(_u32 port)=0;
	virtual iSocketIO *create_udp_client(_str_t dst_ip, _u32 port)=0;
	virtual void close_socket(iSocketIO *p_sio)=0;
	virtual iTCPServer *create_tcp_server(_u32 port)=0;
	virtual iTCPClient *create_tcp_client(_str_t dst_ip, _u32 port)=0;
};

#endif

