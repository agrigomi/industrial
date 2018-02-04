#include <string.h>
#include "iMemory.h"
#include "iRepository.h"
#include "rb_alg.h"

extern "C" {
	void *rb_mem_alloc(_u32 sz, void *udata);
	void rb_mem_free(void *ptr, _u32 sz, void *udata);
	void rb_mem_set(void *ptr, _u8 pattern, _u32 size, void *udata);
	void rb_mem_cpy(void *dst, void *src, _u32 size, void *udata);
	_u64 rb_lock(_u64 hlock, void *udata);
	void rb_unlock(_u64 hlock, void *udata);
};

class cRingBuffer: public iRingBuffer {
private:
	_rb_context_t m_cxt;
	iHeap *mpi_heap;
	iMutex *mpi_mutex;

	friend void *rb_mem_alloc(_u32 sz, void *udata);
	friend void rb_mem_free(void *ptr, _u32 sz, void *udata);
	friend _u64 rb_lock(_u64 hlock, void *udata);
	friend void rb_unlock(_u64 hlock, void *udata);

public:
	BASE(cRingBuffer, "cRingBuffer", RF_CLONE, 1,0,0);

	void init(_u32 capacity) {
		m_cxt.mem_alloc = rb_mem_alloc;
		m_cxt.mem_free = rb_mem_free;
		m_cxt.mem_set = rb_mem_set;
		m_cxt.mem_cpy = rb_mem_cpy;
		m_cxt.lock = rb_lock;
		m_cxt.unlock = rb_unlock;

		rb_init(&m_cxt, capacity, this);
	}

	void destroy(void) {
		rb_destroy(&m_cxt);
	}

	void push(void *msg, _u16 sz) {
		rb_push(&m_cxt, msg, sz);
	}

	void *pull(_u16 *psz) {
		return rb_pull(&m_cxt, psz);
	}

	void reset_pull(void) {
		rb_reset_pull(&m_cxt);
	}

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				mpi_heap = 0;
				mpi_mutex = 0;
				memset(&m_cxt, 0, sizeof(m_cxt));
				iRepository *repo = (iRepository*)arg;
				mpi_heap = (iHeap*)repo->object_by_iname(I_HEAP, RF_ORIGINAL);
				mpi_mutex = (iMutex*)repo->object_by_iname(I_MUTEX, RF_CLONE);
				if(mpi_heap && mpi_mutex)
					r = true;
				break;
			}
			case OCTL_UNINIT: {
				iRepository *repo = (iRepository*)arg;
				rb_destroy(&m_cxt);
				repo->object_release(mpi_heap);
				repo->object_release(mpi_mutex);
				break;
			}
		}

		return r;
	}
};

void *rb_mem_alloc(_u32 sz, void *udata) {
	void *r = 0;
	cRingBuffer *obj = (cRingBuffer*)udata;
	if(obj->mpi_heap)
		r = obj->mpi_heap->alloc(sz);
	return r;
}
void rb_mem_free(void *ptr, _u32 sz, void *udata) {
	cRingBuffer *obj = (cRingBuffer*)udata;
	if(obj->mpi_heap)
		obj->mpi_heap->free(ptr, sz);
}
void rb_mem_set(void *ptr, _u8 pattern, _u32 size, void *udata) {
	memset(ptr, pattern, size);
}
void rb_mem_cpy(void *dst, void *src, _u32 size, void *udata) {
	memcpy(dst, src, size);
}
_u64 rb_lock(_u64 hlock, void *udata) {
	_u64 r = 0;
	cRingBuffer *obj = (cRingBuffer*)udata;
	if(obj->mpi_mutex)
		r = obj->mpi_mutex->lock(hlock);
	return r;
}
void rb_unlock(_u64 hlock, void *udata) {
	cRingBuffer *obj = (cRingBuffer*)udata;
	if(obj->mpi_mutex)
		obj->mpi_mutex->unlock(hlock);
}

static cRingBuffer _g_object_;
