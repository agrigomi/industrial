#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "private.h"

extension::extension() {
	memset(m_alias, 0, sizeof(m_alias));
	memset(m_file, 0, sizeof(m_file));
	m_handle = NULL;
	m_get_base_array = NULL;
	m_init = NULL;
}

_err_t extension::load(_cstr_t file, _cstr_t alias) {
	_err_t r = ERR_UNKNOWN;

	if((m_handle = dlopen(file, RTLD_NOW | RTLD_DEEPBIND))) {
		m_init = (_init_t *)dlsym(m_handle, "init");
		m_get_base_array = (_get_base_array_t *)dlsym(m_handle, "get_base_array");
		if(m_init) {
			strncpy(m_file, file, sizeof(m_file)-1);
			strncpy(m_alias, (alias && strlen(alias)) ? alias : basename(file), sizeof(m_alias)-1);
			r = ERR_NONE;
		} else {
			dlclose(m_handle);
			m_handle = NULL;
		}
	} else
		r = ERR_LOADEXT;

	return r;
}

_err_t extension::unload(void) {
	_err_t r = ERR_UNKNOWN;

	if(m_handle) {
		if(dlclose(m_handle) == 0) {
			r = ERR_NONE;
			m_handle = NULL;
		}
	}

	return r;
}

_base_entry_t *extension::array(_u32 *count, _u32 *limit) {
	_base_entry_t *r = 0;

	if(m_handle && m_get_base_array)
		r = m_get_base_array(count, limit);

	return r;
}

_err_t extension::init(iRepository *pi_repo) {
	_err_t r = ERR_UNKNOWN;

	if(m_handle && m_init)
		r = m_init(pi_repo);

	return r;
}
