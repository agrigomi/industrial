#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "private.h"

static _map_t	_g_ext_map_;

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

_err_t load_extension(_cstr_t file, _cstr_t alias) {
	_err_t r = ERR_UNKNOWN;
	_extension_t ext;

	if(!find_extension(alias)) {
		if((r = ext.load(file, alias)) == ERR_NONE) {
			_cstr_t alias = ext.alias();

			_g_ext_map_.add((void *)alias, strlen(alias), &ext, sizeof(ext));
		}
	} else
		r = ERR_DUPLICATED;

	return r;
}

_err_t unload_extension(_cstr_t alias) {
	_err_t r = ERR_UNKNOWN;
	_u32 sz = 0;
	_extension_t *pext = (_extension_t *)_g_ext_map_.get((void *)alias, strlen(alias), &sz);

	if(pext) {
		if((r = pext->unload()) == ERR_NONE)
			_g_ext_map_.del((void *)alias, strlen(alias));
	} else
		r = ERR_MISSING;

	return r;
}

_extension_t *find_extension(_cstr_t alias) {
	_u32 sz = 0;

	return (_extension_t *)_g_ext_map_.get((void *)alias, strlen(alias), &sz);
}

_err_t init_extension(_cstr_t alias, iRepository *pi_repo) {
	_err_t r = ERR_UNKNOWN;
	_extension_t *pext = find_extension(alias);

	if(pext)
		r = pext->init(pi_repo);
	else
		r = ERR_MISSING;

	return r;
}

_base_entry_t *get_base_array(_cstr_t alias, _u32 *count, _u32 *limit) {
	_base_entry_t *r = NULL;
	_extension_t *pext = find_extension(alias);

	if(pext)
		r = pext->array(count, limit);

	return r;
}
