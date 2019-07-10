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
	virtual _u32 content_len(void)=0;
	virtual void *data(_u32 *size)=0;
	//...
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
#define RNDR_DONE	(1<<0) // done flag (end of transmission)
#define RNDR_CACHE	(1<<1) // use file cache
#define RNDR_RESOLVE_MT	(1<<2) // auto resolve mime (content) type if RNDR_DONE is set
#define RNDR_SET_MTIME	(1<<3) // auto set modify time if RNDR_DONE is set
	virtual bool render(_cstr_t fname, _u8 flags=RNDR_DONE|RNDR_CACHE|RNDR_RESOLVE_MT|RNDR_SET_MTIME)=0;
	//...
}_response_t;

// events
#define ON_CONNECT	HTTP_ON_OPEN
#define ON_REQUEST	HTTP_ON_REQUEST
#define ON_DATA		HTTP_ON_REQUEST_DATA
#define ON_ERROR	HTTP_ON_ERROR
#define ON_DISCONNECT	HTTP_ON_CLOSE
#define ON_NOT_FOUND	HTTP_ON_RESERVED1

typedef void _on_route_event_t(_u8 evt, _request_t *request, _response_t *response, void *udata);

struct gatn_server {
	virtual bool is_running(void)=0;
	virtual bool start(void)=0;
	virtual void stop(void)=0;
	virtual _cstr_t name(void)=0;
	virtual _u32 port(void)=0;
	virtual void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata=NULL)=0;
	virtual void on_event(_u8 evt, _on_http_event_t *pcb, void *udata=NULL)=0;
	virtual void remove_route(_u8 method, _cstr_t path)=0;
	virtual _request_t *get_request(iHttpServerConnection *pi_httpc)=0;
	virtual _response_t *get_response(iHttpServerConnection *pi_httpc)=0;
	virtual iFileCache *get_file_cache(void)=0;
	virtual void enum_route(void (*)(_cstr_t path, _on_route_event_t *pcb, void *udata), void *udata=NULL)=0;
	virtual bool add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key, _cstr_t cache_exclude=NULL)=0;
	virtual bool remove_virtual_host(_cstr_t host)=0;
	virtual bool start_virtual_host(_cstr_t host)=0;
	virtual bool stop_virtual_host(_cstr_t host)=0;
};

class iGatnExtension: public iBase {
public:
	INTERFACE(iGatnExtension, I_GATN_EXTENSION);
	virtual _cstr_t name(void)=0; // returns extension name
	virtual bool init(_server_t *p_srv, _cstr_t host=NULL)=0;
	virtual void uninit(_server_t *p_srv, _cstr_t host=NULL)=0;
};

class iGatn: public iBase {
public:
	INTERFACE(iGatn, I_GATN);

	virtual bool configure(_cstr_t json_fname)=0;
	virtual _server_t *create_server(_cstr_t name, _u32 port,
					_cstr_t doc_root, // path to documents root
					_cstr_t cache_path, // path to cache folder (by example: /tmp/)
					_cstr_t no_cache="", // non cacheable area inside documents root (by example: /folder1:/folder2:...)
					_u32 buffer_size=HTTP_BUFFER_SIZE,
					_u32 max_workers=HTTP_MAX_WORKERS,
					_u32 max_connections=HTTP_MAX_CONNECTIONS,
					_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
					SSL_CTX *ssl_context=NULL)=0;
	virtual _server_t *server_by_name(_cstr_t name)=0;
	virtual void remove_server(_server_t *p_srv)=0;
	virtual bool stop_server(_server_t *p_srv)=0;
	virtual bool start_server(_server_t *p_srv)=0;
};

#endif
