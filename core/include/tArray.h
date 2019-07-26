#ifndef __TARRAY_H__
#define __TARRAY_H__

#include <string.h>
#include <mutex>
#include "startup.h"
#include "iMemory.h"
#include "iRepository.h"

template <class T>
class _LOCAL_ tArray {
private:
	T 		*mp_array;
	_u32 		m_capacity;
	_u32		m_initial;
	_u32 		m_size;
	iHeap 		*mpi_heap;
	bool		my_heap;
	bool		m_is_init;
	std::mutex	m_mutex;

	iHeap *get_heap(void) {
		iHeap *r = mpi_heap;

		if(!r) {
			r = mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL);
			my_heap = true;
		}

		return r;
	}

	bool realloc(void) {
		bool r = false;
		T *_old = mp_array;
		_u32 sz_new = (m_capacity + m_initial) * sizeof(T);
		_u32 sz_old = m_capacity * sizeof(T);
		iHeap *pi_heap = get_heap();

		if(pi_heap) {
			T *_new = (T *)pi_heap->alloc(sz_new);

			if(_new) {
				memset((void *)_new, 0, sz_new);
				if(_old) {
					memcpy((void *)_new, (void *)_old, sz_old);
					mpi_heap->free((void *)_old, sz_old);
				}

				mp_array = _new;
				m_capacity += m_initial;
				r = true;
			}
		}

		return r;
	}

public:
	tArray(_u32 capacity, iHeap *pi_heap=NULL) {
		m_is_init = false;
		init(capacity, pi_heap);
	}

	tArray() {
		m_capacity = m_size = 0;
		m_initial = 0;
		mpi_heap = NULL;
		mp_array = NULL;
		my_heap = m_is_init = false;
	}

	~tArray() {
		destroy();
	}

	void init(_u32 capacity, iHeap *pi_heap) {
		if(!m_is_init) {
			m_capacity = 0;
			mp_array = NULL;
			m_size = 0;
			m_initial = capacity;
			my_heap = false;

			if(!(mpi_heap = pi_heap))
				get_heap();

			m_is_init = true;
		}
	}

	T& get(_u32 i) {
		T *r = NULL;

		if(m_is_init) {
			m_mutex.lock();
			if(i < m_size)
				r = &mp_array[i];
			m_mutex.unlock();
		}

		return *r;
	}

	T& operator [](_u32 i) {
		return get(i);
	}

	void add(const T &item) {
		if(m_is_init) {
			bool a = true;

			m_mutex.lock();
			if(m_size == m_capacity)
				a = realloc();

			if(a) {
				mp_array[m_size] = item;
				m_size++;
			}
			m_mutex.unlock();
		}
	}

	void operator +=(const T &item) {
		add(item);
	}

	_u32 size(void) {
		_u32 r = 0;

		m_mutex.lock();
		r = m_size;
		m_mutex.unlock();
		return r;
	}

	void clean(void) {
		m_mutex.lock();
		if(m_is_init && m_capacity) {
			iHeap *pi_heap = get_heap();

			if(pi_heap)
				pi_heap->free((void *)mp_array, m_capacity * sizeof(T));

			mp_array = NULL;
			m_capacity = m_size = 0;
		}
		m_mutex.unlock();
	}

	void destroy(void) {
		m_mutex.lock();
		if(m_is_init && m_capacity) {
			iHeap *pi_heap = get_heap();

			if(pi_heap)
				pi_heap->free((void *)mp_array, m_capacity * sizeof(T));

			mp_array = NULL;
			m_capacity = m_size = 0;
			m_initial = 0;
			if(my_heap)
				_gpi_repo_->object_release(mpi_heap);
			mpi_heap = NULL;
			m_is_init = my_heap = false;
		}
		m_mutex.unlock();
	}
};

#endif
