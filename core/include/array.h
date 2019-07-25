#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <string.h>
#include "iMemory.h"
#include "iRepository.h"


template <class T>
class tArray {
private:
	T 	*mp_array;
	_u32 	m_capacity;
	_u32	m_initial;
	_u32 	m_size;
	iHeap 	*mpi_heap;
	bool	my_heap;

	bool realloc(void) {
		bool r = false;
		T *_old = mp_array;
		_u32 sz_new = (m_capacity + m_initial) * sizeof(T);
		_u32 sz_old = m_capacity * sizeof(T);

		if(mpi_heap) {
			T *_new = (T *)mpi_heap->alloc(sz_new);

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
	tArray(int capacity, iHeap *pi_heap=NULL) {
		m_capacity = 0;
		mp_array = NULL;
		m_size = 0;
		m_initial = capacity;
		my_heap = false;

		if(!(mpi_heap = pi_heap)) {
			mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL);
			my_heap = true;
		}
	}

	~tArray() {
		destroy();
	}

	T& operator [](_u32 i) {
		T *r = NULL;

		if(i < m_size)
			r = &mp_array[i];

		return *r;
	}

	void add(const T &item) {
		bool a = true;

		if(m_size == m_capacity)
			a = realloc();

		if(a) {
			mp_array[m_size] = item;
			m_size++;
		}
	}

	void operator +=(const T &item) {
		add(item);
	}

	_u32 size(void) {
		return m_size;
	}

	void destroy(void) {
		if(mp_array && m_capacity && mpi_heap) {
			mpi_heap->free((void *)mp_array, m_capacity * sizeof(T));
			mp_array = NULL;
			m_capacity = m_size = m_initial = 0;
			if(my_heap)
				_gpi_repo_->object_release(mpi_heap);
		}
	}
};

#endif
