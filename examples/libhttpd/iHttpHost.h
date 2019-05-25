#include "iBase.h"
#include "iMemory.h"
#include "iFS.h"
#include "iNet.h"

#define I_HTTP_HOST	"iHttpHost"

typedef void _enum_http_t(bool running, _cstr_t name, _u32 port, _cstr_t root, void *udata);

class iHttpHost: public iBase {
public:
	INTERFACE(iHttpHost, I_HTTP_HOST);
	// Create new http server
	virtual bool create(_cstr_t name, _u32 port, _cstr_t doc_root, _u32 max_threads=HTTP_MAX_WORKERS,
				_u32 max_connections=HTTP_MAX_CONNECTIONS, _u32 connection_timeout=HTTP_CONNECTION_TIMEOUT)=0;
	// Start http server
	virtual bool start(_cstr_t name)=0;
	// Stop http server
	virtual bool stop(_cstr_t name)=0;
	// Remove http server
	virtual bool remove(_cstr_t name)=0;
	// Enumeration of http servers
	virtual void enumerate(_enum_http_t *pcb, void *udata=NULL)=0;
};
