#include "iBase.h"
#include "iMemory.h"
#include "iFS.h"

#define I_HTTP_HOST	"iHttpHost"

typedef void _enum_http_t(bool running, _cstr_t name, _u32 port, _cstr_t root, void *udata);

typedef struct {
	// buffers
	_bmap_status_t		bmap_status;
	// file cache
	_fcache_status_t	fcache_status;
	// heap status
	_heap_status_t		heap_status;
}_httpd_status_t;

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
	// Enumeration of http servers
	virtual void enumerate(_enum_http_t *pcb, void *udata=NULL)=0;
	virtual void status(_httpd_status_t *p_st)=0;
};
