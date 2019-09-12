#include <string.h>
#include "iMemory.h"
#include "iGatn.h"
#include "iLog.h"
#include "iStr.h"
#include "iSync.h"

struct request: public _request_t{
	iHttpServerConnection *mpi_httpc;
	_server_t *mpi_server;

	iHttpServerConnection *connection(void) {
		return mpi_httpc;
	}

	_server_t *server(void) {
		return mpi_server;
	}

	_u8 method(void);
	_cstr_t header(void);
	_cstr_t utl(void);
	_cstr_t urn(void);
	_cstr_t var(_cstr_t name);
	_u32 content_len(void);
	void *data(_u32 *size);
	void destroy(void);
	bool parse_content(void);
};

#define MAX_SERVER_NAME		32
#define SERVER_BUFFER_SIZE	16384
#define HTTP_MAX_EVENTS		10
#define MAX_DOC_ROOT_PATH	512
#define MAX_CACHE_PATH		512
#define MAX_CACHE_KEY		32
#define MAX_HOSTNAME		256
#define MAX_ROUTE_PATH		256
#define IDX_CONNECTION		7

typedef struct {
	HFCACHE	hfc; // handle from file cache
	iFileIO	*pi_fio;
	_cstr_t mime;
}_handle_t;

typedef _handle_t* HDOCUMENT;
typedef struct root _root_t;
typedef struct vhost _vhost_t;

typedef struct {
	_u8	method;
	_char_t	path[MAX_ROUTE_PATH];

	_u32 size(void) {
		return sizeof(_u8) + strlen(path);
	}
}_route_key_t;

typedef struct {
	_gatn_route_event_t	*pcb;
	void			*udata;
	_char_t			path[MAX_ROUTE_PATH];

	_u32 size(void) {
		return (sizeof(_gatn_route_event_t *) + sizeof(void *) + strlen(path));
	}
}_route_data_t;


struct root { // document root
private:
	volatile bool	m_enable;
	_char_t		m_root_path[MAX_DOC_ROOT_PATH];
	_char_t		m_cache_path[MAX_CACHE_PATH];
	_char_t		m_cache_key[MAX_CACHE_KEY];
	iFS		*mpi_fs;
	iHeap		*mpi_heap;
	iFileCache	*mpi_fcache;
	_str_t		m_nocache;
	_u32		m_sz_nocache;
	_str_t		m_disabled;
	_u32		m_sz_disabled;
	iMutex		*mpi_mutex;
	iPool		*mpi_handle_pool;
	iStr		*mpi_str;
	HMUTEX		m_hlock;
	bool		m_my_heap;

	void object_release(iBase **ppi);
	void parse_nocache_list(_cstr_t nocache);
	_u32 get_url_path(_cstr_t url);
	bool cacheable(_cstr_t path, _u32 len);
	bool disabled(_cstr_t path, _u32 len);
	_str_t realloc_nocache(_u32 sz);
	_str_t realloc_path_disable(_u32 sz);
public:
	bool init(_cstr_t doc_root, _cstr_t cache_path,
		_cstr_t cache_key,
		_cstr_t cache_exclude_path, // example: /folder1:/foldef2:...
		_cstr_t path_disable, iHeap *pi_heap=NULL);
	void cache_exclude(_cstr_t path);
	void disable_path(_cstr_t path);
	void destroy(void);
	HDOCUMENT open(_cstr_t url);
	void *ptr(HDOCUMENT, _ulong*);
	void close(HDOCUMENT);
	time_t mtime(HDOCUMENT);
	_cstr_t mime(HDOCUMENT);
	void stop(void);
	void start(void);
	volatile bool is_enabled(void) {
		return m_enable;
	}
	_cstr_t get_doc_root(void) {
		return m_root_path;
	}
	iFileCache *get_file_cache(void) {
		return mpi_fcache;
	}
};

struct response: public _response_t {
	iHttpServerConnection *mpi_httpc;
	_server_t *mpi_server;
	iHeap *mpi_heap;
	iBufferMap *mpi_bmap;
	HBUFFER	*mp_hbarray;
	_u32 m_hbcount; // array capacity
	_u32 m_buffers; // allocated buffers
	_u32 m_content_len;
	_root_t *mpi_root;
	iFileCache *mpi_fcache;
	iFS *mpi_fs;

	iHttpServerConnection *connection(void) {
		return mpi_httpc;
	}

	_server_t *server(void) {
		return mpi_server;
	}

	bool alloc_buffer(void);
	void clear(void);
	_u32 capacity(void);
	_u32 remainder(void);
	bool resize_array(void);
	void var(_cstr_t name, _cstr_t value);
	void _var(_cstr_t name, _cstr_t fmt, ...);
	_u32 write(void *data, _u32 size);
	_u32 write(_cstr_t str);
	_u32 _write(_cstr_t fmt, ...);
	_u32 end(_u16 response_code, void *data, _u32 size);
	_u32 end(_u16 response_code, _cstr_t str);
	_u32 _end(_u16 response_code, _cstr_t fmt, ...);
	void destroy(void);
	void process_content(void);
	void redirect(_cstr_t uri);
	_cstr_t text(_u16 rc);
	_u16 error(void);
	bool render(_cstr_t fname,
		_u8 flags=RNDR_DONE|RNDR_CACHE|RNDR_RESOLVE_MT|RNDR_SET_MTIME|RNDR_USE_DOCROOT);
};

void init_mime_type_resolver(void);
void uninit_mime_type_resolver(void);
_cstr_t resolve_mime_type(_cstr_t fname);

typedef struct {
	_gatn_http_event_t	*pcb;
	void			*udata;
}_event_data_t;

struct vhost {
private:
	iMutex		*pi_mutex;
	HMUTEX		m_hlock;
	_char_t		host[MAX_HOSTNAME];	// host name
	_server_t	*pi_server;
	_root_t		root;			// document root
	iMap		*pi_route_map;		// URL routing map
	iMap		*pi_class_map;		// class (plugin) map
	iHeap		*pi_heap;
	iLog		*pi_log;
	_event_data_t 	event[HTTP_MAX_EVENTS];	// HTTP event handlers
	volatile bool	m_running;

	iMutex *get_mutex(void);
	iMap *get_route_map(void);
	iMap *get_class_map(void);
	HMUTEX lock(HMUTEX hlock=0);
	void unlock(HMUTEX hlock);
	void _lock(void);
	void _unlock(void);
	void clear_events(void);
	void start_extensions(HMUTEX hlock=0);
	void stop_extensions(HMUTEX hlock=0);
	void remove_extensions(void);
public:

	vhost();
	bool init(_server_t *server, _cstr_t name, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_key, _cstr_t cache_exclude,
		_cstr_t root_exclude, iHeap *pi_heap=NULL);
	void destroy(void);
	_cstr_t name(void) {
		return  host;
	}
	_root_t *get_root(void) {
		return &root;
	}
	volatile bool is_running(void) {
		return m_running;
	}
	bool start(void);
	void stop(void);
	void add_route_handler(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata);
	void remove_route_handler(_u8 method, _cstr_t path);
	void set_event_handler(_u8 evt, _gatn_http_event_t *pcb, void *udata);
	_gatn_http_event_t *get_event_handler(_u8 evt, void **pp_udata);
	bool attach_class(_cstr_t cname, _cstr_t options);
	bool detach_class(_cstr_t cname);
	void call_handler(_u8 evt, iHttpServerConnection *p_httpc);
	void call_route_handler(_u8 evt, iHttpServerConnection *p_httpc);
};

typedef struct {
	request		req;
	response	res;
	_cstr_t		url;
	_vhost_t	*p_vhost;
	HDOCUMENT	hdoc;

	void clear(void) {
		if(hdoc && p_vhost) // close handle
			p_vhost->get_root()->close(hdoc);
		res.clear();
		url = NULL;
		hdoc = NULL;
		p_vhost = NULL;
	}

	void destroy(void) {
		if(hdoc && p_vhost) // close handle
			p_vhost->get_root()->close(hdoc);
		req.destroy();
		res.destroy();
		url = NULL;
		hdoc = NULL;
		p_vhost = NULL;
	}
}_connection_t;

struct server: public _server_t {
	_char_t 	m_name[MAX_SERVER_NAME];
	_u32 		m_port;
	iHttpServer 	*mpi_server;
	_vhost_t	host; // default host
	iMap		*mpi_vhost_map; // virtual hosts map
	iNet		*mpi_net; // networking
	iFS		*mpi_fs; // FS support
	iLog		*mpi_log; // system log
	iHeap		*mpi_heap;
	iPool		*mpi_pool;
	bool		m_autorestore;
	iBufferMap	*mpi_bmap;
	_u32		m_buffer_size;
	_u32		m_max_workers;
	_u32		m_max_connections;
	_u32		m_connection_timeout;
	SSL_CTX		*m_ssl_context;

	bool init(_cstr_t name, _u32 port, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_exclude,
		_cstr_t path_disable, _u32 buffer_size,
		_u32 max_workers, _u32 max_connections,
		_u32 connection_timeout, SSL_CTX *ssl_context);

	void destroy(void);
	void destroy(_vhost_t *pvhost);
	bool is_running(void) {
		return (mpi_server) ? true : false;
	}
	bool start(void);
	void stop(void);
	_vhost_t *get_host(_cstr_t host);
	void stop(_vhost_t *pvhost);
	bool start(_vhost_t *pvhost);
	_cstr_t name(void) {
		return m_name;
	}
	_u32 port(void) {
		return m_port;
	}
	void attach_network(void);
	void attach_fs(void);
	void release_network(void);
	void release_fs(void);
	bool create_connection(iHttpServerConnection *p_httpc);
	void destroy_connection(iHttpServerConnection *p_httpc);
	void set_handlers(void);
	void call_handler(_u8 evt, iHttpServerConnection *p_httpc);
	void call_route_handler(_u8 evt, iHttpServerConnection *p_httpc);
	void update_response(iHttpServerConnection *p_httpc);
	void on_route(_u8 method, _cstr_t path, _gatn_route_event_t *pcb, void *udata=NULL, _cstr_t host=NULL);
	void on_event(_u8 evt, _gatn_http_event_t *pcb, void *udata=NULL, _cstr_t host=NULL);
	_gatn_http_event_t *get_event_handler(_u8 evt, void **pp_udata, _cstr_t host=NULL);
	SSL_CTX *get_SSL_context(void) {
		return m_ssl_context;
	}
	void remove_route(_u8 method, _cstr_t path, _cstr_t host=NULL);
	bool add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key,
				_cstr_t cache_exclude=NULL, _cstr_t path_disable=NULL);
	_vhost_t *get_virtual_host(_cstr_t host);
	bool remove_virtual_host(_cstr_t host);
	bool start_virtual_host(_cstr_t host);
	bool stop_virtual_host(_cstr_t host);
	void enum_virtual_hosts(void (*)(_vhost_t *, void *udata), void *udata=NULL);
	bool attach_class(_cstr_t cname, _cstr_t options=NULL, _cstr_t host=NULL);
	bool detach_class(_cstr_t cname, _cstr_t host=NULL);
	void release_class(_cstr_t cname);
};

// SSL
void ssl_init(void);
const SSL_METHOD *ssl_select_method(_cstr_t method);
bool ssl_load_cert(SSL_CTX *ssl_cxt, _cstr_t cert);
bool ssl_load_key(SSL_CTX *ssl_cxt, _cstr_t key);
SSL_CTX *ssl_create_context(const SSL_METHOD *method);
_cstr_t ssl_error_string(void);



