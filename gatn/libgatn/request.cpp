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
	bool r = true;
	_cstr_t var = NULL;
	_cstr_t val = NULL;
	_u32 	sz_var = 0;
	_u32	sz_val = 0;
	_u32	i = 0, len = strlen(cookies);
	_char_t	c = 0, _c = 0;

#define CF_STROPHE	(1<<0)
#define CF_QUOTES	(1<<1)
#define CF_VAR		(1<<2)
#define CF_VAL		(1<<3)
#define CF_SYMBOL	(1<<4)

	_u8	state = 0;

	for(i = 0; i <= len; i++) {
		c = cookies[i];

		switch(c) {
			case '"':
				if(_c != '\\')
					state ^= CF_QUOTES;
				state |= CF_SYMBOL;
				break;
			case '\'':
				if(_c != '\\')
					state ^= CF_STROPHE;
				state |= CF_SYMBOL;
				break;
			case ';':
			case '\r':
			case 0:
				if(!(state & (CF_STROPHE | CF_QUOTES))) {
					if(var && sz_var) {
						if(val && sz_val)
							mpi_cookie_map->add(var, sz_var, val, sz_val);
						else
							mpi_cookie_map->del(var, sz_var);
					}

					state = 0;
					var = val = NULL;
					sz_var = sz_val = 0;
				} else {
					if(c)
						state |= CF_SYMBOL;
				}
				break;
			case '=':
				if(!(state & (CF_STROPHE | CF_QUOTES))) {
					state = CF_VAL;
					val = NULL;
					sz_val = 0;
				} else
					state |= CF_SYMBOL;
				break;
			case '\n':
				state &= ~CF_SYMBOL;
				break;
			case ' ':
				if(state & (CF_VAR |CF_VAL))
					state |= CF_SYMBOL;
				break;
			default:
				state |= CF_SYMBOL;
				break;
		}

		if(state & CF_SYMBOL) {
			if(state & CF_VAL) {
				if(!val)
					val = &cookies[i];
				sz_val++;
			} else {
				if(!var) {
					var = &cookies[i];
					state |= CF_VAR;
				}
				sz_var++;
			}

			state &= ~CF_SYMBOL;
		}

		_c = c;
	}

	return r;
}

bool request::parse_cookies(void) {
	bool r = false;

	if(!mpi_cookie_map) {
		_cstr_t cookies = var("Cookie");

		if(cookies) {
			if((mpi_cookie_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE)))) {
				if((r = mpi_cookie_map->init(15)))
					r = parse_cookies(cookies);
				else {
					_gpi_repo_->object_release(mpi_cookie_map);
					mpi_cookie_map = NULL;
				}
			}
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
