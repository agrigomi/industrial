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

_mutex_handle_t lock_extensions(void) {
	return _g_ext_map_.lock();
}

void unlock_extensions(_mutex_handle_t hlock) {
	_g_ext_map_.unlock(hlock);
}

_err_t load_extension(_cstr_t file, _cstr_t alias, _extension_t **pp_ext, _mutex_handle_t hlock) {
	_err_t r = ERR_UNKNOWN;
	_extension_t ext;
	_cstr_t _alias = (alias) ? alias : "";

	if(!find_extension(_alias, hlock)) {
		if((r = ext.load(file, _alias)) == ERR_NONE) {
			_alias = ext.alias();
			_extension_t *p_ext = (_extension_t *)_g_ext_map_.add((void *)_alias, strlen(_alias), &ext, sizeof(ext), hlock);

			if(pp_ext)
				*pp_ext = p_ext;
		}
	} else
		r = ERR_DUPLICATED;

	return r;
}

_err_t unload_extension(_cstr_t alias, _mutex_handle_t hlock) {
	_err_t r = ERR_UNKNOWN;
	_u32 sz = 0;
	_extension_t *pext = (_extension_t *)_g_ext_map_.get((void *)alias,
				strlen(alias), &sz, hlock);

	if(pext) {
		if((r = pext->unload()) == ERR_NONE)
			_g_ext_map_.del((void *)alias, strlen(alias), hlock);
	} else
		r = ERR_MISSING;

	return r;
}

void unload_extensions(_mutex_handle_t hlock) {
	enum_extensions([](_extension_t *pext, void *udata)->_s32 {
		if(pext->unload() == ERR_NONE)
			return MAP_ENUM_DELETE;
		else
			return MAP_ENUM_CONTINUE;
	}, NULL, hlock);
}

_extension_t *find_extension(_cstr_t alias, _mutex_handle_t hlock) {
	_u32 sz = 0;

	return (_extension_t *)_g_ext_map_.get((void *)alias, strlen(alias), &sz, hlock);
}

_err_t init_extension(_cstr_t alias, iRepository *pi_repo, _mutex_handle_t hlock) {
	_err_t r = ERR_UNKNOWN;
	_extension_t *pext = find_extension(alias, hlock);

	if(pext)
		r = pext->init(pi_repo);
	else
		r = ERR_MISSING;

	return r;
}

_base_entry_t *get_base_array(_cstr_t alias, _u32 *count, _u32 *limit, _mutex_handle_t hlock) {
	_base_entry_t *r = NULL;
	_extension_t *pext = find_extension(alias, hlock);

	if(pext)
		r = pext->array(count, limit);

	return r;
}

void enum_extensions(_s32 (*enum_cb)(_extension_t *, void *), void *udata, _mutex_handle_t hlock) {
	typedef struct {
		_s32 (*_enum_cb)(_extension_t *, void *);
		void *udata;
	}_ext_enum_t;

	_ext_enum_t enum_data = {enum_cb, udata};

	_g_ext_map_.enm([](void *p, _u32 sz, void *udata)->_s32 {
		_ext_enum_t *p_ext_enum = (_ext_enum_t *)udata;

		return p_ext_enum->_enum_cb((_extension_t *)p, p_ext_enum->udata);
	}, &enum_data, hlock);
}

void destroy_extension_storage(_mutex_handle_t hlock) {
	unload_extensions(hlock);
	_g_ext_map_.destroy(hlock);
}

