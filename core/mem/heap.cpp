#include <string.h>
#include <malloc.h>
#include "iRepository.h"
#include "iMemory.h"
#include "iSync.h"
#include "s2.h"

#define PAGE_SIZE	4096
#define S2_LIMIT	0xffffffffffffffffLLU

extern "C" {
	void *page_alloc(_u32 npages, _ulong limit, void *p_udata);
	void page_free(void *ptr, _u32 npages, void *p_udata);
	_s2_hlock_t s2_lock(_s2_hlock_t hlock, void *p_udata);
	void s2_unlock(_s2_hlock_t, void *p_udata);
}

class cHeap:public iHeap {
private:
	iMutex	*mpi_mutex;
	_s2_context_t m_s2c;
	iRepository *mpi_repo;
	bool m_disable_lock;

	iMutex *get_mutex(void) {
		if(!mpi_mutex) {
			m_disable_lock = true;
			mpi_mutex = (iMutex*)_gpi_repo_->object_by_iname(I_MUTEX, RF_CLONE);
			m_disable_lock = false;
		}
		return mpi_mutex;
	}

	friend _s2_hlock_t s2_lock(_s2_hlock_t hlock, void *p_udata);
	friend void s2_unlock(_s2_hlock_t, void *p_udata);

public:
	INFO(cHeap, "cHeap", RF_CLONE|RF_ORIGINAL, 1, 0, 0);
	CONSTRUCTOR(cHeap) {
		register_object(this);
	}
	DESTRUCTOR(cHeap) {
		m_disable_lock = true;
		s2_destroy(&m_s2c);
	}

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				mpi_repo = (iRepository *)arg;
				mpi_mutex = 0;
				m_disable_lock = false;
				// init s2 context
				m_s2c.page_size = PAGE_SIZE;
				m_s2c.p_udata = this;
				m_s2c.p_s2 = 0;
				m_s2c.p_mem_alloc = page_alloc;
				m_s2c.p_mem_free = page_free;
				m_s2c.p_mem_cpy = (_mem_cpy_t *)memcpy;
				m_s2c.p_mem_set = (_mem_set_t *)memset;
				m_s2c.p_lock = s2_lock;
				m_s2c.p_unlock = s2_unlock;
				if(s2_init(&m_s2c))
					r = true;
				break;
			case OCTL_UNINIT: {
				iRepository *repo = (iRepository *)arg;
				repo->object_release(mpi_mutex);
				m_disable_lock = true;
				s2_destroy(&m_s2c);
				memset(&m_s2c, 0, sizeof(_s2_context_t));
				r = true;
				break;
			}
		}
		return r;
	}

	void *alloc(_u32 size) {
		return s2_alloc(&m_s2c, size, S2_LIMIT);
	}

	void free(void *ptr, _u32 size) {
		s2_free(&m_s2c, ptr, size);
	}

	bool verify(void *ptr, _u32 size) {
		return (s2_verify(&m_s2c, ptr, size)) ? true : false;
	}

	void status(_heap_status_t *p_hs) {
		_s2_status_t s2_sts;

		s2_status(&m_s2c, &s2_sts);
		p_hs->robj = s2_sts.nrobj;
		p_hs->aobj = s2_sts.naobj;
		p_hs->dpgs = s2_sts.ndpg;
		p_hs->mpgs = s2_sts.nspg;
	}
};

static cHeap _g_object_;

void *page_alloc(_u32 npages, _ulong limit, void *p_udata) {
	return malloc(npages * PAGE_SIZE);
}

void page_free(void *ptr, _u32 npages, void *p_udata) {
	free(ptr);
}

_s2_hlock_t s2_lock(_s2_hlock_t hlock, void *p_udata) {
	_s2_hlock_t r = 0;
	cHeap *obj = (cHeap *)p_udata;
	if(!obj->m_disable_lock) {
		iMutex *pim = obj->get_mutex();
		if(pim)
			r = (_s2_hlock_t)pim->lock((HMUTEX)hlock);
	}
	return r;
}

void s2_unlock(_s2_hlock_t hlock, void *p_udata) {
	cHeap *obj = (cHeap *)p_udata;
	if(!obj->m_disable_lock) {
		iMutex *pim = obj->get_mutex();
		if(pim)
			pim->unlock((HMUTEX)hlock);
	}
}



