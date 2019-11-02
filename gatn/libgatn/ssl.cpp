#include <string.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "dtype.h"

static _char_t _g_error_string_[2048] = "";
typedef const SSL_METHOD *_ssl_methodcb_t(void);

typedef struct {
	_cstr_t	ind;
	_ssl_methodcb_t *cb_method;
}_ssl_method_t;

static _ssl_method_t _g_ssl_method_map[] = {
	{"SSLv23",	SSLv23_server_method},
	{"TLS",		TLS_server_method},
	{"DTLS",	DTLS_server_method},
	{NULL,		NULL}
};

void ssl_init(void) {
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
}

const SSL_METHOD *ssl_select_method(_cstr_t method) {
	const SSL_METHOD *r = NULL;

	_u32 n = 0;

	while(_g_ssl_method_map[n].ind) {
		if(strcmp(_g_ssl_method_map[n].ind, method) == 0) {
			r = _g_ssl_method_map[n].cb_method();
			break;
		}

		n++;
	}

	if(!_g_ssl_method_map[n].ind)
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
