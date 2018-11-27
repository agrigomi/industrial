#ifndef __I_NET_H__
#define __I_NET_H__

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "iIO.h"

#define I_SOCKET_IO		"iSocketIO"
#define I_NET			"iNet"
#define I_TCP_SERVER		"iTCPServer"
#define I_HTTP_SERVER		"iHttpServer"
#define I_HTTP_CONNECTION	"iHttpConnection"

class iSocketIO: public iIO {
public:
	INTERFACE(iSocketIO, I_SOCKET_IO);
	virtual void blocking(bool)=0; /* blocking or nonblocking IO */
	virtual bool alive(void)=0;
	virtual _u32 peer_ip(void)=0;
	virtual bool peer_ip(_str_t strip, _u32 len)=0;
};

#define SSL_CERT_ASN1		1
#define SSL_CERT_PEM		2
#define SSL_CERT_ASN1_FILE	3
#define SSL_CERT_PEM_FILE	4
#define SSL_CERT_CHAIN_FILE	5
#define SSL_PKEY_ASN1		11
#define SSL_PKEY_PEM		12
#define SSL_PKEY_ASN1_FILE	13
#define SSL_PKEY_PEM_FILE	14

class iTCPServer: public iBase {
public:
	INTERFACE(iTCPServer, I_TCP_SERVER);
	virtual iSocketIO *listen(void)=0;
	virtual bool enable_ssl(bool, _ulong options=0)=0;
	virtual bool ssl_use(_cstr_t str, _u32 type)=0;
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

class iHttpConnection: public iBase {
public:
	INTERFACE(iHttpConnection, I_HTTP_CONNECTION);
	// verify I/O
	virtual bool alive(void)=0;
	// close connection
	virtual void close(void)=0;
	// get peer IP in string format
	virtual bool peer_ip(_str_t strip, _u32 len)=0;
	// get peer IP in integer format
	virtual _u32 peer_ip(void)=0;
	// set user data
	virtual void set_udata(_ulong)=0;
	// get user data
	virtual _ulong get_udata(void)=0;
	// get request header
	virtual _str_t req_header(_u32 *)=0;
	// get request body
	virtual _str_t req_body(_u32 *)=0;
	// start of response
	virtual _u32 response(_u16 rc, // response code
				_str_t hdr, // response header
				_str_t content, //response content
				/*
				 Size of response content.
				 If zero, string length should be taken.
				 This parameter will be stored in response header as
				   Content-Length.
				 If it's greater than string len of content parameter,
				  ON_HTTP_RES_CONTINUE should be happen */
				_u32 content_len=0
				)=0;
	// continue of response
	virtual _u32 response(_str_t content, // remainder part of response content
				_u32 size // size of response content
				)=0;
	// get size of response remainder pard
	virtual _u32 remainder(void)=0;
};

// HTTP event prototype
typedef void _on_http_event_t(iHttpConnection *pi_httpc);

#define ON_HTTP_CONNECT		1
#define ON_HTTP_REQUEST		2 // request ready (awaiting for response)
#define ON_HTTP_CONTINUE	3 // response continue
#define ON_HTTP_DISCONNECT	4

class iHttpServer: public iBase {
public:
	INTERFACE(iHttpServer, I_HTTP_SERVER);
	virtual void on_event(_u8 evt, _on_http_event_t *handler)=0;
	virtual bool enable_ssl(bool, _ulong options=0)=0;
	virtual bool ssl_use(_cstr_t str, _u32 type)=0;
	virtual bool is_running(void)=0;
};

class iNet: public iBase {
public:
	INTERFACE(iNet, I_NET);
	virtual iSocketIO *create_udp_server(_u32 port)=0;
	virtual iSocketIO *create_udp_client(_str_t dst_ip, _u32 port)=0;
	virtual iSocketIO *create_multicast_sender(_str_t group, _u32 port)=0;
	virtual iSocketIO *create_multicast_listener(_str_t group, _u32 port)=0;
	virtual void close_socket(iSocketIO *p_sio)=0;
	virtual iTCPServer *create_tcp_server(_u32 port)=0;
	virtual iSocketIO *create_tcp_client(_str_t host, _u32 port, SSL_CTX *ssl_context=NULL)=0;
	virtual iHttpServer *create_http_server(_u32 port)=0;
};

#endif

