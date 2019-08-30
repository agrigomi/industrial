#include "iGatn.h"
#include "private.h"

_u8 request::method(void) {
	_u8 r = 0;

	if(mpi_httpc)
		r = mpi_httpc->req_method();

	return r;
}

_cstr_t request::header(void) {
	_cstr_t r = NULL;

	if(mpi_httpc)
		r = mpi_httpc->req_header();

	return r;
}

_cstr_t request::utl(void) {
	_cstr_t r = NULL;

	if(mpi_httpc)
		r = mpi_httpc->req_url();

	return r;
}

_cstr_t request::urn(void) {
	_cstr_t r = NULL;

	if(mpi_httpc)
		r = mpi_httpc->req_urn();

	return r;
}

_cstr_t request::var(_cstr_t name) {
	_cstr_t r = NULL;

	if(mpi_httpc)
		r = mpi_httpc->req_var(name);

	return r;
}

_u32 request::content_len(void) {
	_u32 r = 0;

	if(mpi_httpc)
		r = mpi_httpc->req_content_len();

	return r;
}

void *request::data(_u32 *size) {
	void *r = NULL;

	if(mpi_httpc)
		r = mpi_httpc->req_data(size);

	return r;
}

void request::destroy(void) {
	mpi_httpc = NULL;
	mpi_server = NULL;
}

bool request::parse_content(void) {
	return mpi_httpc->req_parse_content();
}
