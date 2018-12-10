#include "iBase.h"

#define I_HTTP_HOST	"iHttpHost"

class iHttpHost: public iBase {
public:
	INTERFACE(iHttpHost, I_HTTP_HOST);
	// Create new http server
	virtual bool create(_cstr_t name, _u32 port, _cstr_t doc_root)=0;
	// Start http server
	virtual bool start(_cstr_t name)=0;
	// Stop http server
	virtual bool stop(_cstr_t name)=0;
	// Remove http server
	virtual bool remove(_cstr_t name)=0;
};
