#include <string.h>
#include "iRepository.h"
#include "iMemory.h"
#include "ll_alg.h"

extern "C" {
	void *mem_alloc(_u32 size, _ulong limit, void *p_udata);
	void mem_free(void *ptr, _u32 size, void *p_udata);
	HMUTEX ll_lock(HMUTEX hlock, void *p_udata);
	void ll_unlock(HMUTEX hlock, void *p_udata);
}

class cLlist:public iLlist {
private:
	_ll_context_t m_cxt;
	iMutex *mpi_mutex;
	iHeap  *mpi_heap;

	friend void *mem_alloc(_u32 size, _ulong limit, void *p_udata);
	friend void mem_free(void *ptr, _u32 size, void *p_udata);

public:
	INFO(cLlist, "cLlist", RF_CLONE, 1, 0, 0);
	CONSTRUCTOR(cLlist) {
		mpi_mutex = 0;
		mpi_heap = 0;
		register_object(this);
	}
	DESTRUCTOR(cLlist) {}

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT: {
				iRepository *p_repo = (iRepository *)arg;
				if((mpi_mutex = (iMutex*)p_repo->object_by_iname(I_MUTEX, RF_CLONE)))
					r = true;
				break;
			}
			case OCTL_UNINIT: {
				iRepository *p_repo = (iRepository *)arg;
				ll_uninit(&m_cxt);
				p_repo->object_release(mpi_mutex);
				p_repo->object_release(mpi_heap);
				mpi_mutex = 0;
				mpi_heap = 0;
				r = true;
				break;
			}
		}
		return r;
	}

	bool init(_u8 mode, _u8 ncol, iHeap *pi_heap=0) {
		if(!(mpi_heap = pi_heap))
			mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL);

		memset(&m_cxt, 0, sizeof(_ll_context_t));
		m_cxt.mode = mode;
		m_cxt.ccol = 0;
		m_cxt.ncol = ncol;
		m_cxt.state = 0;
		m_cxt.p_alloc = mem_alloc;
		m_cxt.p_free = mem_free;
		m_cxt.p_lock = ll_lock;
		m_cxt.p_unlock = ll_unlock;
		m_cxt.addr_limit = 0xffffffffffffffffLLU;
		m_cxt.p_udata = this;

		return (ll_init(&m_cxt, mode, ncol, m_cxt.addr_limit)) ? true : false;
	}

	void uninit(void) {
		ll_uninit(&m_cxt);
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

	// get record by index
	void *get(_u32 index, _u32 *p_size, HMUTEX hlock=0) {
		return ll_get(&m_cxt, index, p_size, hlock);
	}

	// add record at end of list
	void *add(void *rec, _u32 size, HMUTEX hlock=0) {
		return ll_add(&m_cxt, rec, size, hlock);
	}

	// insert record
	void *ins(_u32 index, void *rec, _u32 size, HMUTEX hlock=0) {
		return ll_ins(&m_cxt, index, rec, size, hlock);
	}

	// remove record by index
	void rem(_u32 index, HMUTEX hlock=0) {
		ll_rem(&m_cxt, index, hlock);
	}

	// delete current record
	void del(HMUTEX hlock=0) {
		ll_del(&m_cxt, hlock);
	}

	// delete all records
	void clr(HMUTEX hlock=0) {
		ll_clr(&m_cxt, hlock);
	}

	// return number of records
	_u32 cnt(HMUTEX hlock=0) {
		return ll_cnt(&m_cxt, hlock);
	}

	// select column
	void col(_u8 col, HMUTEX hlock=0) {
		ll_col(&m_cxt, col, hlock);
	}

	// set current record by content
	bool sel(void *rec, HMUTEX hlock=0) {
		return (ll_sel(&m_cxt, rec, hlock)) ? true : false;
	}

	// move record to other column
	bool mov(void *rec, _u8 col, HMUTEX hlock=0) {
		return (ll_mov(&m_cxt, rec, col, hlock)) ? true : false;
	}

	// get next record
	void *next(_u32 *p_size, HMUTEX hlock=0) {
		return ll_next(&m_cxt, p_size, hlock);
	}

	// get current record
	void *current(_u32 *p_size, HMUTEX hlock=0) {
		return ll_current(&m_cxt, p_size, hlock);
	}

	// get first record
	void *first(_u32 *p_size, HMUTEX hlock=0) {
		return ll_first(&m_cxt, p_size, hlock);
	}

	// get last record
	void *last(_u32 *p_size, HMUTEX hlock=0) {
		return ll_last(&m_cxt, p_size, hlock);
	}

	// get prev. record
	void *prev(_u32 *p_size, HMUTEX hlock=0) {
		return ll_prev(&m_cxt, p_size, hlock);
	}

	// queue mode specific (last-->first-->seccond ...)
	void roll(HMUTEX hlock=0) {
		ll_roll(&m_cxt, hlock);
	}
};

static cLlist _g_object_;


void *mem_alloc(_u32 size, _ulong limit, void *p_udata) {
	void *r = 0;
	cLlist *obj = (cLlist *)p_udata;
	if(obj->mpi_heap)
		r = obj->mpi_heap->alloc(size);
	return r;
}

void mem_free(void *ptr, _u32 size, void *p_udata) {
	cLlist *obj = (cLlist *)p_udata;
	if(obj->mpi_heap)
		obj->mpi_heap->free(ptr, size);
}

HMUTEX ll_lock(HMUTEX hlock, void *p_udata) {
	cLlist *obj = (cLlist *)p_udata;
	return obj->lock(hlock);
}

void ll_unlock(HMUTEX hlock, void *p_udata) {
	cLlist *obj = (cLlist *)p_udata;
	obj->unlock(hlock);
}
