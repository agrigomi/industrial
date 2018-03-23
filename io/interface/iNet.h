#ifndef __I_NET_H__
#define __I_NET_H__

#include "iIO.h"

#define I_NET		"iNet"
#define I_UDP_SERVER	"iUDPServer"
#define I_UDP_CLIENT	"iUDPClient"
#define I_TCP_SERVER	"iTCPServer"
#define I_TCP_CLIENT	"iTCPClient"

class iUDPServer: public iBase {
public:
	INTERFACE(iUDPServer, I_UDP_SERVER);
};

class iUDPClient: public iBase {
public:
	INTERFACE(iUDPClient, I_UDP_CLIENT);
};

class iTCPServer: public iBase {
public:
	INTERFACE(iTCPServer, I_TCP_SERVER);
};

class iTCPClient: public iBase {
public:
	INTERFACE(iTCPClient, I_TCP_CLIENT);
};

class iNet: public iBase {
public:
	INTERFACE(iNet, I_NET);
	virtual iUDPServer *create_udp_server(_u32 port)=0;
	virtual iUDPClient *create_udp_client(_u32 port)=0;
	virtual iTCPServer *create_tcp_server(_u32 port)=0;
	virtual iTCPClient *create_tcp_client(_u32 port)=0;
};

#endif

