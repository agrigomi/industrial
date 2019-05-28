#ifndef __I_NET_H__
#define __I_NET_H__

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "iIO.h"

#define I_SOCKET_IO			"iSocketIO"
#define I_NET				"iNet"
#define I_TCP_SERVER			"iTCPServer"
#define I_HTTP_SERVER			"iHttpServer"
#define I_HTTP_CLIENT_CONNECTION	"iHttpClientConnection"
#define I_HTTP_SERVER_CONNECTION	"iHttpServerConnection"

class iSocketIO: public iIO {
public:
	INTERFACE(iSocketIO, I_SOCKET_IO);
	virtual void blocking(bool)=0; /* blocking or nonblocking IO */
	virtual bool alive(void)=0;
	virtual _u32 peer_ip(void)=0;
	virtual bool peer_ip(_str_t strip, _u32 len)=0;
};

class iTCPServer: public iBase {
public:
	INTERFACE(iTCPServer, I_TCP_SERVER);
	virtual iSocketIO *listen(void)=0;
	virtual void blocking(bool)=0; /* blocking or nonblocking IO */
	virtual void close(iSocketIO *p_io)=0;
};

// HTTP response code
#define HTTPRC_CONTINUE			100 // Continue
#define HTTPRC_SWITCHING_PROTOCOL	101 // Switching Protocols
#define HTTPRC_OK			200 // OK
#define HTTPRC_CREATED			201 // Created
#define HTTPRC_ACCEPTED			202 // Accepted
#define HTTPRC_NON_AUTH			203 // Non-Authoritative Information
#define HTTPRC_NO_CONTENT		204 // No Content
#define HTTPRC_RESET_CONTENT		205 // Reset Content
#define HTTPRC_PART_CONTENT		206 // Partial Content
#define HTTPRC_MULTICHOICES		300 // Multiple Choices
#define HTTPRC_MOVED_PERMANENTLY	301 // Moved Permanently
#define HTTPRC_FOUND			302 // Found
#define HTTPRC_SEE_OTHER		303 // See Other
#define HTTPRC_NOT_MODIFIED		304 // Not Modified
#define HTTPRC_USE_PROXY		305 // Use proxy
#define HTTPRC_TEMP_REDIRECT		307 // Temporary redirect
#define HTTPRC_BAD_REQUEST		400 // Bad Request
#define HTTPRC_UNAUTHORIZED		401 // Unauthorized
#define HTTPRC_PAYMENT_REQUIRED		402 // Payment Required
#define HTTPRC_FORBIDDEN		403 // Forbidden
#define HTTPRC_NOT_FOUND		404 // Not Found
#define HTTPRC_METHOD_NOT_ALLOWED	405 // Method Not Allowed
#define HTTPRC_NOT_ACCEPTABLE		406 // Not Acceptable
#define HTTPRC_PROXY_AUTH_REQUIRED	407 // Proxy Authentication Required
#define HTTPRC_REQUEST_TIMEOUT		408 // Request Time-out
#define HTTPRC_CONFLICT			409 // Conflict
#define HTTPRC_GONE			410 // Gone
#define HTTPRC_LENGTH_REQUIRED		411 // Length Required
#define HTTPRC_PRECONDITION_FAILED	412 // Precondition Failed
#define HTTPRC_REQ_ENTITY_TOO_LARGE	413 // Request Entity Too Large
#define HTTPRC_REQ_URI_TOO_LARGE	414 // Request-URI Too Large
#define HTTPRC_UNSUPPORTED_MEDIA_TYPE	415 // Unsupported Media Type
#define HTTPRC_EXPECTATION_FAILED	417 // Expectation Failed
#define HTTPRC_INTERNAL_SERVER_ERROR	500 // Internal Server Error
#define HTTPRC_NOT_IMPLEMENTED		501 // Not Implemented
#define HTTPRC_BAD_GATEWAY		502 // Bad Gateway
#define HTTPRC_SERVICE_UNAVAILABLE	503 // Service Unavailable
#define HTTPRC_GATEWAY_TIMEOUT		504 // Gateway Time-out
#define HTTPRC_VERSION_NOT_SUPPORTED	505 // HTTP Version not supported

// request method
#define HTTP_METHOD_GET		1
#define HTTP_METHOD_HEAD	2
#define HTTP_METHOD_POST	3
#define HTTP_METHOD_PUT		4
#define HTTP_METHOD_DELETE	5
#define HTTP_METHOD_CONNECT	6
#define HTTP_METHOD_OPTIONS	7
#define HTTP_METHOD_TRACE	8

class iHttpServerConnection: public iBase {
public:
	INTERFACE(iHttpServerConnection, I_HTTP_SERVER_CONNECTION);
	// verify I/O
	virtual bool alive(void)=0;
	// close connection
	virtual void close(void)=0;
	// get peer IP in string format
	virtual bool peer_ip(_str_t strip, _u32 len)=0;
	// get peer IP in integer format
	virtual _u32 peer_ip(void)=0;
	// set user data
	virtual void set_udata(_ulong, _u8 index=0)=0;
	// get user data
	virtual _ulong get_udata(_u8 index=0)=0;
	// get request method
	virtual _u8 req_method(void)=0;
	// retuen request HEADER
	virtual _cstr_t req_header(void)=0;
	// retuen request URI
	virtual _cstr_t req_uri(void)=0;
	// retuen request URL
	virtual _cstr_t req_url(void)=0;
	// return request URN
	virtual _cstr_t req_urn(void)=0;
	// get request variable
	virtual _cstr_t req_var(_cstr_t name)=0;
	// get request data
	virtual _u8 *req_data(_u32 *size)=0;
	// get requested protocol
	virtual _cstr_t req_protocol(void)=0;
	// set variable in response header
	virtual bool res_var(_cstr_t name, _cstr_t value)=0;
	// get error code
	virtual _u16 error_code(void)=0;
	// set response code
	virtual void res_code(_u16 httprc)=0;
	// set response protocol as string like 'HTTP/1.1'
	virtual void res_protocol(_cstr_t protocol)=0;
	// set Content-Length variable
	virtual bool res_content_len(_u32 content_len)=0;
	// return content len of response
	virtual _u32 res_content_len(void)=0;
	// return nimber of sent bytes for response content
	virtual _u32 res_content_sent(void)=0;
	// set last modify time in response header
	virtual void res_mtime(time_t mtime)=0;
	// returns the content length of  request
	virtual _u32 req_content_len(void)=0;
	// return remainder pard of response data in bytes (ContentLength - Sent)
	virtual _u32 res_remainder(void)=0;
	// write response
	virtual _u32 res_write(_u8 *data, _u32 size)=0;
	virtual _u32 res_write(_cstr_t str)=0;
};

// HTTP event prototype
typedef void _on_http_event_t(iHttpServerConnection *, void *);

// http events
#define HTTP_ON_OPEN		1
#define HTTP_ON_REQUEST		2
#define HTTP_ON_REQUEST_DATA	3
#define HTTP_ON_RESPONSE_DATA	4
#define HTTP_ON_ERROR		5
#define HTTP_ON_CLOSE		6
#define HTTP_ON_RESERVED1	7
#define HTTP_ON_RESERVED2	8

class iHttpServer: public iBase {
public:
	INTERFACE(iHttpServer, I_HTTP_SERVER);
	virtual void on_event(_u8 evt, _on_http_event_t *handler, void *udata=NULL)=0;
	virtual bool is_running(void)=0;
};

typedef void _on_http_response_t(void *data, _u32 size, void *udata);

class iHttpClientConnection: public iBase {
public:
	INTERFACE(iHttpClientConnection, I_HTTP_CLIENT_CONNECTION);
	// connect to host
	virtual bool connect(void)=0;
	// disconnect from host
	virtual void disconnect(void)=0;
	// reset object members
	virtual void reset(void)=0;
	// check connection
	virtual bool alive(void)=0;
	// set request method
	virtual void req_method(_u8 method)=0;
	// set request URL
	virtual void req_url(_cstr_t url)=0;
	// set request variable
	virtual void req_var(_cstr_t name, _cstr_t value)=0;
	// Write request content
	virtual _u32 req_write(_u8 *data, _u32 size)=0;
	virtual _u32 req_write(_cstr_t str)=0;
	virtual _u32 req_write(_cstr_t fmt, ...)=0;
	// send request with timeout in milliseconds
	virtual bool send(_u32 timeout_ms, _on_http_response_t *p_cb_resp=NULL, void *udata=NULL)=0;
	// get response code
	virtual _u16 res_code(void)=0;
	// get value from response variable
	virtual _cstr_t res_var(_cstr_t name, _u32 *sz)=0;
	// get response content len
	virtual _u32 res_content_len(void)=0;
	// get response content
	virtual void res_content(_on_http_response_t *p_cb_resp, void *udata=NULL)=0;
};

#define HTTP_BUFFER_SIZE	8192
#define HTTP_MAX_WORKERS	32
#define HTTP_MAX_CONNECTIONS	500
#define HTTP_CONNECTION_TIMEOUT	10 // in sec.

class iNet: public iBase {
public:
	INTERFACE(iNet, I_NET);
	virtual iSocketIO *create_udp_server(_u32 port)=0;
	virtual iSocketIO *create_udp_client(_cstr_t dst_ip, _u32 port)=0;
	virtual iSocketIO *create_multicast_sender(_cstr_t group, _u32 port)=0;
	virtual iSocketIO *create_multicast_listener(_cstr_t group, _u32 port)=0;
	virtual void close_socket(iSocketIO *p_sio)=0;
	virtual iTCPServer *create_tcp_server(_u32 port, SSL_CTX *ssl_context=NULL)=0;
	virtual iSocketIO *create_tcp_client(_cstr_t host, _u32 port, SSL_CTX *ssl_context=NULL)=0;
	virtual iHttpServer *create_http_server(_u32 port,
						_u32 buffer_size=HTTP_BUFFER_SIZE,
						_u32 max_workers=HTTP_MAX_WORKERS,
						_u32 max_connections=HTTP_MAX_CONNECTIONS,
						_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
						SSL_CTX *ssl_context=NULL)=0;
	virtual iHttpClientConnection *create_http_client(_cstr_t host,
						_u32 port,
						_u32 buffer_size=HTTP_BUFFER_SIZE,
						SSL_CTX *ssl_context=NULL)=0;
};

#endif

