#ifndef __TMAP_H__
#define __TMAP_H__

#include <string.h>
#include "iMemory.h"
#include "iRepository.h"

class _LOCAL_ tMap_base {
protected:
	iMap *mpi_map;
	bool m_is_init;

	tMap_base() {
		m_is_init = false;
		mpi_map = NULL;
	}

	~tMap_base() {
		destroy();
	}

	tMap_base(_u32 capacity, iHeap *pi_heap = NULL) {
		init(capacity, pi_heap);
	}

public:
	bool init(_u32 capacity, iHeap *pi_heap) {
		bool r = false;

		if(!m_is_init) {
			if((mpi_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
				r = m_is_init = mpi_map->init(capacity, pi_heap);
		}

		return r;
	}

	void destroy(void) {
		if(m_is_init) {
			_gpi_repo_->object_release(mpi_map);
			mpi_map = NULL;
			m_is_init = false;
		}
	}
};

template <typename _K, typename _V>
class _LOCAL_ tMap: public tMap_base {
public:
	tMap(): tMap_base() {}
	tMap(_u32 capacity, iHeap *pi_heap=NULL) : tMap_base(capacity, pi_heap) {}

	_V& get(const _K &key) {
		_V *r = NULL;
		_u32 sz = 0;

		if(m_is_init)
			r = static_cast<_V *>(mpi_map->get(&key, sizeof(key), &sz));

		return *r;
	}

	_V& add(const _K &key, const _V &value) {
		_V *r = NULL;

		if(m_is_init)
			r = static_cast<_V *>(mpi_map->add(&key, sizeof(key), &value, sizeof(value)));

		return *r;
	}

	_V& operator [](const _K &key) {
		return get(key);
	}
};

template<>
class _LOCAL_ tMap<_cstr_t, _cstr_t>: public tMap_base {
public:
	tMap(): tMap_base() {}
	tMap(_u32 capacity, iHeap *pi_heap=NULL) : tMap_base(capacity, pi_heap) {}

	_cstr_t get(_cstr_t key) {
		_cstr_t r = NULL;
		_u32 sz = 0;

		if(m_is_init)
			r = static_cast<_cstr_t>(mpi_map->get(key, strlen(key), &sz));

		return r;
	}

	_cstr_t add(_cstr_t key, _cstr_t value) {
		_cstr_t r = NULL;

		if(m_is_init)
			r = static_cast<_cstr_t>(mpi_map->add(key, strlen(key), value, strlen(value)));

		return r;
	}

	_cstr_t operator [](_cstr_t key) {
		return get(key);
	}
};

template<typename _V>
class _LOCAL_ tMap<_cstr_t, _V>: public tMap_base {
public:
	tMap(): tMap_base() {}
	tMap(_u32 capacity, iHeap *pi_heap=NULL) : tMap_base(capacity, pi_heap) {}

	_V& get(_cstr_t key) {
		_V *r = NULL;
		_u32 sz = 0;

		if(m_is_init)
			r = static_cast<_V *>(mpi_map->get(key, strlen(key), &sz));

		return *r;
	}

	_V& add(_cstr_t key, const _V &value) {
		_V *r = NULL;

		if(m_is_init)
			r = static_cast<_V *>(mpi_map->add(key, strlen(key), &value, sizeof(value)));

		return *r;
	}

	_V& operator [](_cstr_t key) {
		return get(key);
	}
};

#endif
