#include "startup.h"
#include "private.h"

IMPLEMENT_BASE_ARRAY(10)

class cNet: public iNet {
public:
	BASE(cNet, "cNet", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	iUDPServer *create_udp_server(_u32 port) {
		iUDPServer *r = 0;
		//...
		return r;
	}

	iUDPClient *create_udp_client(_str_t dst_ip, _u32 port) {
		iUDPClient *r = 0;
		//...
		return r;
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
