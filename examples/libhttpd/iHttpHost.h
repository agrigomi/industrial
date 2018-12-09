#include "iBase.h"

#define I_HTTP_HOST	"iHttpHost"

class iHttpHost: public iBase {
public:
	INTERFACE(iHttpHost, I_HTTP_HOST);
	virtual bool create_http_server(_cstr_t name, _u32 port, _cstr_t doc_root)=0;
	//...
};
