#include <string.h>
#include "private.h"

static _map_t _g_base_map_;


static bool add_iname(_base_entry_t *pb_entry) {
	bool r = false;
	_base_key_t key;
	_object_info_t oi;
	iBase *pi_base = pb_entry->pi_base;
	_u32 sz = 0;

	memset(&key, 0, sizeof(_base_key_t));
	pi_base->object_info(&oi);
	strncpy(key.iname, oi.iname, sizeof(key.iname)-1);

	_base_entry_t **pp_bentry = (_base_entry_t **)_g_base_map_.get(&key, sizeof(_base_key_t), &sz);

	if(pp_bentry) { // already exists
		// check class name
		_object_info_t _oi;

		(*pp_bentry)->pi_base->object_info(&_oi);
		if(strcmp(oi.cname, _oi.cname) == 0) {
			// check version
			if(oi.version.version > _oi.version.version) {
				if(_g_base_map_.set(&key, sizeof(_base_key_t), &pb_entry, sizeof(pb_entry)))
					r = true;
			}
		}
	} else {
		if(_g_base_map_.add(&key, sizeof(_base_key_t), &pb_entry, sizeof(pb_entry)))
			r = true;
	}

	return r;
}

static bool add_cname(_base_entry_t *pb_entry) {
	bool r = false;
	_base_key_t key;
	_object_info_t oi;
	iBase *pi_base = pb_entry->pi_base;
	_u32 sz = 0;

	memset(&key, 0, sizeof(_base_key_t));
	pi_base->object_info(&oi);
	strncpy(key.cname, oi.cname, sizeof(key.cname)-1);

	_base_entry_t **pp_bentry = (_base_entry_t **)_g_base_map_.get(&key, sizeof(_base_key_t), &sz);

	if(pp_bentry) { // already exists
		// check version
		_object_info_t _oi;

		(*pp_bentry)->pi_base->object_info(&_oi);
		if(oi.version.version > _oi.version.version && strcmp(oi.iname, _oi.iname) == 0) {
			if(_g_base_map_.set(&key, sizeof(_base_key_t), &pb_entry, sizeof(pb_entry)))
				r = true;
		}
	} else {
		if(_g_base_map_.add(&key, sizeof(_base_key_t), &pb_entry, sizeof(pb_entry)))
			r = true;
	}

	return r;
}

static bool add_pointer(_base_entry_t *pb_entry) {
	bool r = false;
	_base_key_t key;

	memset(&key, 0, sizeof(_base_key_t));
	key.pi_base = pb_entry->pi_base;

	if(_g_base_map_.add(&key, sizeof(_base_key_t), &pb_entry, sizeof(pb_entry)))
		r = true;

	return r;
}

void add_base_array(_base_entry_t *pb_entry, _u32 count) {
	for(_u32 i = 0; i < count; i++) {
		_base_entry_t *p = &pb_entry[i];

		add_iname(p);
		add_cname(p);
		add_pointer(p);
	}
}

static void remove_iname(_base_entry_t *pb_entry) {
	_base_key_t key;
	_object_info_t oi;
	iBase *pi_base = pb_entry->pi_base;
	_u32 sz = 0;

	memset(&key, 0, sizeof(_base_key_t));
	pi_base->object_info(&oi);
	strncpy(key.iname, oi.iname, sizeof(key.iname)-1);

	_base_entry_t **pp_bentry = (_base_entry_t **)_g_base_map_.get(&key, sizeof(_base_key_t), &sz);

	if(pp_bentry) { // exists
		if((*pp_bentry)->pi_base == pb_entry->pi_base)
			_g_base_map_.del(&key, sizeof(_base_key_t));
	}
}

static void remove_cname(_base_entry_t *pb_entry) {
	_base_key_t key;
	_object_info_t oi;
	iBase *pi_base = pb_entry->pi_base;
	_u32 sz = 0;

	memset(&key, 0, sizeof(_base_key_t));
	pi_base->object_info(&oi);
	strncpy(key.cname, oi.cname, sizeof(key.cname)-1);

	_base_entry_t **pp_bentry = (_base_entry_t **)_g_base_map_.get(&key, sizeof(_base_key_t), &sz);

	if(pp_bentry) { // exists
		if((*pp_bentry)->pi_base == pb_entry->pi_base)
			_g_base_map_.del(&key, sizeof(_base_key_t));
	}
}

static void remove_pointer(_base_entry_t *pb_entry) {
	_base_key_t key;

	memset(&key, 0, sizeof(_base_key_t));
	key.pi_base = pb_entry->pi_base;

	_g_base_map_.del(&key, sizeof(_base_key_t));
}

void remove_base_array(_base_entry_t *pb_entry, _u32 count) {
	for(_u32 i = 0; i < count; i++) {
		_base_entry_t *p = &pb_entry[i];

		remove_iname(p);
		remove_cname(p);
		remove_pointer(p);
	}
}

_base_entry_t *find_object(_base_key_t *p_key) {
	_base_entry_t *r = NULL;
	_u32 sz = 0;
	_base_entry_t **pp_bentry = (_base_entry_t **)_g_base_map_.get(p_key, sizeof(_base_key_t), &sz);

	if(pp_bentry)
		r = *pp_bentry;

	return r;
}

_base_entry_t *find_object_by_iname(_cstr_t iname) {
	_base_key_t key;

	memset(&key, 0, sizeof(_base_key_t));
	strncpy(key.iname, iname, sizeof(key.iname)-1);
	return find_object(&key);
}

_base_entry_t *find_object_by_cname(_cstr_t cname) {
	_base_key_t key;

	memset(&key, 0, sizeof(_base_key_t));
	strncpy(key.cname, cname, sizeof(key.cname)-1);
	return find_object(&key);
}

_base_entry_t *find_object_by_pointer(iBase *pi_base) {
	_base_key_t key;

	memset(&key, 0, sizeof(_base_key_t));
	key.pi_base = pi_base;
	return find_object(&key);
}

void destroy_base_array_storage(void) {
	_g_base_map_.destroy();
}
