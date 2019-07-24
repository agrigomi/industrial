#include <string.h>
#include "iMemory.h"
#include "iRepository.h"

#define COL_FREE	0
#define COL_BUSY	1

class cPool: public iPool {
private:
	iLlist	*mpi_list;
	_u32 	m_data_size;
	void *mp_udata;
	void (*mp_cb)(_u8, void *, void *);

	void destroy(void) {
		HMUTEX hm = mpi_list->lock();
		_u32 sz = 0;

		_free_all(hm);

		if(mp_cb) {
			mpi_list->col(COL_FREE, hm);
			void *rec = mpi_list->first(&sz, hm);

			while(rec) {
				mp_cb(POOL_OP_DELETE, rec, mp_udata);
				rec = mpi_list->next(&sz, hm);
			}
		}

		mpi_list->unlock(hm);

		_gpi_repo_->object_release(mpi_list, false);
	}

	void _clear(HMUTEX hlock=0) {
		HMUTEX hm = mpi_list->lock(hlock);
		_u32 sz = 0;
		void *rec = 0;

		_free_all(hm);

		mpi_list->col(COL_FREE, hm);

		while((rec = mpi_list->first(&sz, hm))) {
			if(mp_cb)
				mp_cb(POOL_OP_DELETE, rec, mp_udata);
			mpi_list->del(hm);
		}

		mpi_list->unlock(hm);
	}

	void _free_all(HMUTEX hlock=0) {
		HMUTEX hm = mpi_list->lock(hlock);
		_u32 sz = 0;
		void *rec = NULL;

		mpi_list->col(COL_BUSY, hm);

		while((rec = mpi_list->first(&sz, hm))) {
			if(mp_cb)
				mp_cb(POOL_OP_FREE, rec, mp_udata);
			mpi_list->mov(rec, COL_FREE, hm);
		}

		mpi_list->unlock(hm);
	}
public:
	BASE(cPool, "cPool", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				m_data_size = 0;
				mpi_list = NULL;
				r = true;
				break;

			case OCTL_UNINIT:
				destroy();
				r = true;
				break;
		}

		return r;
	}

	bool init(_u32 data_size,
			void (*cb)(_u8, void *, void *),
			void *udata,
			iHeap *pi_heap) {
		bool r = false;

		if((mpi_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE|RF_NONOTIFY)))) {
			m_data_size = data_size;
			mp_cb = cb;
			mp_udata = udata;
			r = mpi_list->init(LL_VECTOR, 2, pi_heap);
		}

		return r;
	}

	_u32 size(void) {
		return m_data_size;
	}

	void *alloc(void) {
		void *r = NULL;
		_u32 sz = 0;
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_FREE, hm);
		if((r = mpi_list->first(&sz, hm))) {
			if(mp_cb)
				mp_cb(POOL_OP_BUSY, r, mp_udata);
			mpi_list->mov(r, COL_BUSY, hm);
		} else {
			mpi_list->col(COL_BUSY, hm);
			if((r = mpi_list->add(m_data_size, hm))) {
				memset(r, 0, m_data_size);
				if(mp_cb)
					mp_cb(POOL_OP_NEW, r, mp_udata);
			}
		}

		mpi_list->unlock(hm);

		return r;
	}

	void free(void *rec) {
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_BUSY, hm);
		if(mpi_list->sel(rec, hm)) {
			if(mp_cb)
				mp_cb(POOL_OP_FREE, rec, mp_udata);
			mpi_list->mov(rec, COL_FREE, hm);
		}

		mpi_list->unlock(hm);
	}

	void free_all(void) {
		_free_all(0);
	}

	void clear(void) {
		_clear(0);
	}

	_u32 num_busy(void) {
		_u32 r = 0;
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_BUSY, hm);
		r = mpi_list->cnt(hm);

		mpi_list->unlock(hm);

		return r;
	}

	_u32 num_free(void) {
		_u32 r = 0;
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_FREE, hm);
		r = mpi_list->cnt(hm);

		mpi_list->unlock(hm);

		return r;
	}
};

static cPool _g_pool_;
