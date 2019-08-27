#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "dtype.h"

static _char_t _g_error_string_[2048] = "";

void ssl_init(void) {
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
}

const SSL_METHOD *ssl_select_method(_cstr_t method) {
	const SSL_METHOD *r = NULL;
	typedef const SSL_METHOD *_ssl_methodcb_t(void);
	typedef struct {
		_cstr_t	ind;
		_ssl_methodcb_t *cb_method;
	}_ssl_method_t;

	_ssl_method_t mm[] = {
		{"TLSv1",	TLSv1_server_method},
		{"SSLv23",	SSLv23_server_method},
		{"TLSv1_1",	TLSv1_1_server_method},
		{"TLSv1_2",	TLSv1_2_server_method},
		{"DTLSv1",	DTLSv1_server_method},
		{NULL,		NULL}
	};

	_u32 n = 0;

	while(mm[n].ind) {
		if(strcmp(mm[n].ind, method) == 0) {
			r = mm[n].cb_method();
			break;
		}

		n++;
	}

	if(!mm[n].ind)
		snprintf(_g_error_string_, sizeof(_g_error_string_),
			"Unsupported SSL method '%s'", method);

	return r;
}

bool ssl_load_cert(SSL_CTX *ssl_cxt, _cstr_t cert) {
	bool r = false;

	if(SSL_CTX_use_certificate_file(ssl_cxt, cert, SSL_FILETYPE_PEM) > 0)
		r = true;

	return r;
}

bool ssl_load_key(SSL_CTX *ssl_cxt, _cstr_t key) {
	bool r = false;

	if(SSL_CTX_use_PrivateKey_file(ssl_cxt, key, SSL_FILETYPE_PEM) > 0) {
		if(SSL_CTX_check_private_key(ssl_cxt))
			r = true;
	}

	return r;
}

SSL_CTX *ssl_create_context(const SSL_METHOD *method) {
	return SSL_CTX_new(method);
}

_cstr_t ssl_error_string(void) {
	_ulong err = ERR_get_error();

	if(err)
		ERR_error_string_n(err, _g_error_string_, sizeof(_g_error_string_));

	return _g_error_string_;
}
