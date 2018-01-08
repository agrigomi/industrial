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

	friend _s2_hlock_t s2_lock(_s2_hlock_t hlock, void *p_udata);
	friend void s2_unlock(_s2_hlock_t, void *p_udata);

public:
	BASE(cHeap, "cHeap", RF_CLONE|RF_ORIGINAL, 1, 0, 0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT: {
				mpi_mutex = 0;
				iRepository *repo = (iRepository *)arg;
				// clone mutex
				if((mpi_mutex = (iMutex*)repo->object_by_interface(I_MUTEX, RF_CLONE))) {
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
					r = true;
				}
			}
			case OCTL_UNINIT: {
				iRepository *repo = (iRepository *)arg;
				repo->object_release(mpi_mutex);
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
	if(obj->mpi_mutex)
		r = (_s2_hlock_t)obj->mpi_mutex->lock((HMUTEX)hlock);
	return r;
}

void s2_unlock(_s2_hlock_t hlock, void *p_udata) {
	cHeap *obj = (cHeap *)p_udata;
	if(obj->mpi_mutex)
		obj->mpi_mutex->unlock((HMUTEX)hlock);
}

