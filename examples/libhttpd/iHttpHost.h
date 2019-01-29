#include "iBase.h"

#define I_HTTP_HOST	"iHttpHost"

typedef void _enum_http_t(bool running, _cstr_t name, _u32 port, _cstr_t root, void *udata);

typedef struct {
	// buffers
	_u32	b_size;	// buffer size
	_u32	b_all;	// total number of buffers
	_u32	b_free; // number of free buffers
	_u32	b_busy; // number of busy buffers
	_u32	b_dirty; // number of dirty buffers
	// file cache

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
};
