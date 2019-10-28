#include "iGatn.h"
#include "iRepository.h"
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

bool request::parse_cookies(_cstr_t cookies) {
	bool r = false;

	//...

	return r;
}

bool request::parse_cookies(void) {
	bool r = false;

	if(!mpi_cookie_map) {
		_cstr_t cookies = var("Cookie");

		if(cookies) {
			mpi_cookie_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE));
			r = parse_cookies(cookies);
		}
	} else
		r = true;

	return r;
}

_cstr_t request::cookie(_cstr_t name) {
	_cstr_t r = NULL;
	_u32 sz = 0;

	if(parse_cookies())
		r = (_cstr_t)mpi_cookie_map->get(name, strlen(name), &sz);

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
	if(mpi_cookie_map) {
		_gpi_repo_->object_release(mpi_cookie_map);
		mpi_cookie_map = NULL;
	}
}

bool request::parse_content(void) {
	return mpi_httpc->req_parse_content();
}
