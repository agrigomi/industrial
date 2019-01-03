#ifndef __I_GATN_H__
#define __I_GATN_H__

#include "iNet.h"
#include "iFS.h"

#define I_GATN	"iGatn"

typedef struct {
	virtual _cstr_t header(void)=0;
	virtual _cstr_t utl(void)=0;
	virtual _cstr_t urn(void)=0;
	virtual _cstr_t var(_cstr_t name)=0;
	virtual _u32 content_len(void)=0;
	virtual void *data(_u32 *size)=0;
	//...
}_request_t;

typedef struct {
	virtual void var(_cstr_t name, _cstr_t value)=0;
	virtual _u32 write(void *data, _u32 size)=0;
	virtual _u32 end(_u16 response_code, void *data, _u32 size)=0;
	//...
}_response_t;

// events
#define ON_CONNECT	HTTP_ON_OPEN
#define ON_REQUEST	HTTP_ON_REQUEST
#define ON_DATA		HTTP_ON_REQUEST_DATA
#define ON_ERROR	HTTP_ON_ERROR
#define ON_DISCONNECT	HTTP_ON_DISCONNECT

typedef void _on_route_event_t(_u8 evt, _request_t *request, _response_t *response, void *udata);

typedef struct {
	virtual bool is_running(void)=0;
	virtual bool start(void)=0;
	virtual void stop(void)=0;
	virtual _cstr_t name(void)=0;
	virtual _u32 port(void)=0;
	virtual void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata)=0;
	virtual void on_event(_u8 evt, _on_http_event_t *pcb, void *udata)=0;
	virtual void remove_route(_u8 method, _cstr_t path)=0;
}_server_t;

class iGatn: public iBase {
public:
	INTERFACE(iGatn, I_GATN);

	virtual _server_t *create_server(_cstr_t name, _u32 port)=0;
	virtual _server_t *server_by_name(_cstr_t name)=0;
	virtual void remove_server(_server_t *p_srv)=0;
	//...
};

#endif
