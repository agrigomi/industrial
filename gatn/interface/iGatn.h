#ifndef __I_GATN_H__
#define __I_GATN_H__

#include "iNet.h"
#include "iFS.h"

#define I_GATN			"iGatn"
#define I_GATN_EXTENSION	"iGatnExtension"

typedef struct gatn_server _server_t;

typedef struct {
	virtual iHttpServerConnection *connection(void)=0;
	virtual _server_t *server(void)=0;
	virtual _u8 method(void)=0;
	virtual _cstr_t header(void)=0;
	virtual _cstr_t utl(void)=0;
	virtual _cstr_t urn(void)=0;
	virtual _cstr_t var(_cstr_t name)=0;
	virtual _cstr_t cookie(_cstr_t name)=0;
	virtual _u32 content_len(void)=0;
	virtual void *data(_u32 *size)=0;
	virtual bool parse_content(void)=0;
}_request_t;

typedef struct {
	virtual iHttpServerConnection *connection(void)=0;
	virtual _server_t *server(void)=0;
	virtual void var(_cstr_t name, _cstr_t value)=0;
	virtual void _var(_cstr_t name, _cstr_t fmt, ...)=0;
	virtual _u32 write(void *data, _u32 size)=0;
	virtual _u32 write(_cstr_t str)=0;
	virtual _u32 _write(_cstr_t fmt, ...)=0;
	virtual _u32 end(_u16 response_code, void *data, _u32 size)=0;
	virtual _u32 end(_u16 response_code, _cstr_t str)=0;
	virtual _u32 _end(_u16 response_code, _cstr_t fmt, ...)=0;
	virtual void redirect(_cstr_t uri)=0;
	virtual _cstr_t text(_u16 rc)=0;
	virtual _u16 error(void)=0;

#define RNDR_DONE		(1<<0) // done flag (end of transmission)
#define RNDR_CACHE		(1<<1) // use file cache
#define RNDR_RESOLVE_MT		(1<<2) // auto resolve mime (content) type if RNDR_DONE is set
#define RNDR_SET_MTIME		(1<<3) // auto set modify time if RNDR_DONE is set
#define RNDR_USE_DOCROOT	(1<<4) // use documents root
	virtual bool render(_cstr_t fname, _u8 flags=RNDR_DONE|RNDR_CACHE|RNDR_RESOLVE_MT|RNDR_SET_MTIME|RNDR_USE_DOCROOT)=0;

#define CF_SECURE		(1<<0)
#define CF_HTTP_ONLY		(1<<1)
#define CF_SAMESITE_STRICT	(1<<2)
#define CF_SAMESITE_LAX		(1<<3)
	virtual void cookie(_cstr_t name,
			_cstr_t value,
			_u8 flags=0,
			_cstr_t expires=NULL,
			_cstr_t max_age=NULL,
			_cstr_t path=NULL,
			_cstr_t domain=NULL)=0;
}_response_t;

// events
#define ON_CONNECT	HTTP_ON_OPEN
#define ON_REQUEST	HTTP_ON_REQUEST
#define ON_DATA		HTTP_ON_REQUEST_DATA
#define ON_ERROR	HTTP_ON_ERROR
#define ON_DISCONNECT	HTTP_ON_CLOSE

// event handler return values
#define	EHR_CONTINUE	0
#define EHR_BREAK	1

typedef void _gatn_route_event_t(_u8, _request_t *, _response_t *, void *);
typedef _s32 _gatn_http_event_t(_request_t *, _response_t *, void *);

struct gatn_server {
	virtual bool is_running(void)=0;
	virtual bool start(void)=0;
	virtual void stop(void)=0;
	virtual _cstr_t name(void)=0;
	virtual _u32 port(void)=0;
	virtual void on_route(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata=NULL, _cstr_t host=NULL)=0;
	virtual void on_event(_u8 evt, _gatn_http_event_t *pcb, void *udata=NULL, _cstr_t host=NULL)=0;
	virtual _gatn_http_event_t *get_event_handler(_u8 evt, void **pp_udata, _cstr_t host=NULL)=0;
	virtual SSL_CTX *get_SSL_context(void)=0;
	virtual void remove_route(_u8 method, _cstr_t path, _cstr_t host=NULL)=0;
	virtual bool add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key,
				_cstr_t cache_exclude=NULL, _cstr_t path_disable=NULL)=0;
	virtual bool remove_virtual_host(_cstr_t host)=0;
	virtual bool start_virtual_host(_cstr_t host)=0;
	virtual bool stop_virtual_host(_cstr_t host)=0;
	virtual bool attach_class(_cstr_t cname, _cstr_t options=NULL, _cstr_t host=NULL)=0;
	virtual bool detach_class(_cstr_t cname, _cstr_t host=NULL, bool remove=true)=0;
};

class iGatnExtension: public iBase {
public:
	INTERFACE(iGatnExtension, I_GATN_EXTENSION);
	virtual bool options(_cstr_t opt)=0;
	virtual bool attach(_server_t *p_srv, _cstr_t host=NULL)=0;
	virtual void detach(_server_t *p_srv, _cstr_t host=NULL)=0;
};

class iGatn: public iBase {
public:
	INTERFACE(iGatn, I_GATN);

	virtual bool configure(_cstr_t json_fname)=0;
	virtual void configure(void *json_context)=0;
	virtual bool reload(_cstr_t server_name=NULL)=0;
	virtual _server_t *create_server(_cstr_t name, _u32 port,
					_cstr_t doc_root, // path to documents root
					_cstr_t cache_path, // path to cache folder (by example: /tmp/)
					_cstr_t no_cache="", // non cacheable area inside documents root (by example: /folder1/:/folder2:...)
					_cstr_t path_disable="", // disabled area inside documents root (by example: /folder1/:/folder2/:...)
					_u32 buffer_size=HTTP_BUFFER_SIZE,
					_u32 max_workers=HTTP_MAX_WORKERS,
					_u32 max_connections=HTTP_MAX_CONNECTIONS,
					_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
					SSL_CTX *ssl_context=NULL)=0;
	virtual _server_t *server_by_name(_cstr_t name)=0;
	virtual void remove_server(_server_t *p_srv)=0;
	virtual bool stop_server(_server_t *p_srv)=0;
	virtual bool start_server(_server_t *p_srv)=0;
	virtual void enum_servers(void (*)(_server_t *, void *), void *udata=NULL)=0;
	virtual SSL_CTX *create_ssl_context(_cstr_t method, _cstr_t cert, _cstr_t key)=0;
	virtual void destroy_ssl_context(SSL_CTX *)=0;
};

#endif
