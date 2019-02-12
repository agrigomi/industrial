#include <assert.h>
#include "iRepository.h"
#include "iMemory.h"
#include "sha1.h"
#include "map_alg.h"

#define INITIAL_CAPACITY	255

class cMap: public iMap {
private:
	iHeap *mpi_heap;
	iMutex *mpi_mutex;
	_map_context_t map_cxt;
	SHA1Context sha1_cxt;
	bool m_is_init;
	static void *_alloc(_u32 size, void *udata) {
		void *r = 0;
		cMap *pobj = (cMap *)udata;
		if(pobj)
			r = pobj->mpi_heap->alloc(size);
		return r;
	}

	static void _free(void *ptr, _u32 size, void *udata) {
		cMap *pobj = (cMap *)udata;
		if(pobj)
			pobj->mpi_heap->free(ptr, size);
	}

	static void _hash(void *data, _u32 sz_data, _u8 *result, void *udata) {
		cMap *pobj = (cMap *)udata;
		if(pobj) {
			SHA1Reset(&(pobj->sha1_cxt));
			SHA1Input(&(pobj->sha1_cxt), (_u8 *)data, sz_data);
			SHA1Result(&(pobj->sha1_cxt), result);
		}
	}

public:
	BASE(cMap, "cMap", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				m_is_init = false;
				mpi_mutex = (iMutex *)pi_repo->object_by_iname(I_MUTEX, RF_CLONE);
				mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);

				if(mpi_mutex && mpi_heap)
					r = true;
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				uninit();
				pi_repo->object_release(mpi_heap);
				pi_repo->object_release(mpi_mutex);
				r = true;
			} break;
		}

		return r;
	}

	bool init(_u32 calacity) {
		bool r = false;

		map_cxt.records = map_cxt.collisions = 0;
		map_cxt.capacity = INITIAL_CAPACITY;
		map_cxt.pf_mem_alloc = _alloc;
		map_cxt.pf_mem_free = _free;
		map_cxt.pf_hash = _hash;
		map_cxt.pp_list = 0;
		map_cxt.udata = this;

		if(map_init(&map_cxt) == _true)
			m_is_init = r = true;

		return r;
	}

	void uninit(void) {
		map_destroy(&map_cxt);
	}

	HMUTEX lock(HMUTEX hlock=0) {
		HMUTEX r = 0;

		if(mpi_mutex)
			r = mpi_mutex->lock(hlock);

		return r;
	}

	void unlock(HMUTEX hlock) {
		if(mpi_mutex)
			mpi_mutex->unlock(hlock);
	}

	void *add(const void *key, _u32 sz_key, const void *data, _u32 sz_data, HMUTEX hlock=0) {
		void *r = 0;
		assert(m_is_init);
		HMUTEX hm = lock(hlock);

		r = map_add(&map_cxt, (void *)key, sz_key, (void *)data, sz_data);
		unlock(hm);

		return r;
	}

	void del(const void *key, _u32 sz_key, HMUTEX hlock=0) {
		assert(m_is_init);
		HMUTEX hm = lock(hlock);

		map_del(&map_cxt, (void *)key, sz_key);
		unlock(hm);
	}

	_u32 cnt(void) {
		return map_cxt.records;
	}

	void *get(const void *key, _u32 sz_key, _u32 *sz_data, HMUTEX hlock=0) {
		void *r = 0;
		assert(m_is_init);
		HMUTEX hm = lock(hlock);

		r = map_get(&map_cxt, (void *)key, sz_key, sz_data);
		unlock(hm);

		return r;
	}

	void clr(HMUTEX hlock=0) {
		assert(m_is_init);
		HMUTEX hm = lock(hlock);

		map_clr(&map_cxt);
		unlock(hm);
	}

	_map_enum_t enum_open(void) {
		assert(m_is_init);
		return map_enum_open(&map_cxt);
	}

	void enum_close(_map_enum_t en) {
		map_enum_close(en);
	}

	void *enum_first(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0) {
		void *r = 0;
		HMUTEX hm = lock(hlock);

		r = map_enum_first(en, sz_data);
		unlock(hm);

		return r;
	}

	void *enum_next(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0) {
		void *r = 0;
		HMUTEX hm = lock(hlock);

		r = map_enum_next(en, sz_data);
		unlock(hm);

		return r;
	}

	void *enum_current(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0) {
		void *r = 0;
		HMUTEX hm = lock(hlock);

		r = map_enum_current(en, sz_data);
		unlock(hm);

		return r;
	}

	void enum_del(_map_enum_t en, HMUTEX hlock=0) {
		HMUTEX hm = lock(hlock);

		map_enum_del(en);
		unlock(hm);
	}

	void status(_map_status_t *p_st) {
		p_st->capacity = map_cxt.capacity;
		p_st->count = map_cxt.records;
		p_st->collisions = map_cxt.collisions;
	}
};

static cMap _g_map_;

