#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <string.h>
#include "startup.h"
#include "iMemory.h"
#include "iRepository.h"

#define INITIAL_ARRAY_SIZE 16

template <class T>
class _LOCAL_ tArray {
private:
	T 	*mp_array;
	_u32 	m_capacity;
	_u32	m_initial;
	_u32 	m_size;
	iHeap 	*mpi_heap;
	bool	my_heap;
	bool	m_is_init;

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
	tArray(_u32 capacity, iHeap *pi_heap=NULL) {
		m_is_init = false;
		init(capacity, pi_heap);
	}

	tArray() {
		m_capacity = m_size = 0;
		m_initial = INITIAL_ARRAY_SIZE;
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

			if(!(mpi_heap = pi_heap)) {
				mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL);
				my_heap = true;
			}

			m_is_init = true;
		}
	}

	T& get(_u32 i) {
		T *r = NULL;

		if(i < m_size)
			r = &mp_array[i];

		return *r;
	}

	T& operator [](_u32 i) {
		return get(i);
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

	void clean(void) {
		if(mp_array && m_capacity && mpi_heap) {
			mpi_heap->free((void *)mp_array, m_capacity * sizeof(T));
			mp_array = NULL;
			m_capacity = m_size = 0;
		}
	}

	void destroy(void) {
		if(mp_array && m_capacity && mpi_heap) {
			mpi_heap->free((void *)mp_array, m_capacity * sizeof(T));
			mp_array = NULL;
			m_capacity = m_size = m_initial = 0;
			if(my_heap)
				_gpi_repo_->object_release(mpi_heap);
			mpi_heap = NULL;
			m_is_init = my_heap = false;
		}
	}
};

#define INITIAL_CH_ARRAY	16

template <class _T>
class tChArray { //Chunked array
private:
	tArray<_T*> m_array;
	_u32 	m_capacity;
	_u32 	m_size;
	_u32	m_initial;
	iHeap 	*mpi_heap;
	bool	my_heap;
	bool	m_is_init;
public:

	tChArray(_u32 capacity, iHeap *pi_heap=NULL) {
		m_is_init = false;
		init(capacity, pi_heap);
	}

	tChArray() {
		m_capacity = m_size = m_initial = 0;
		mpi_heap = NULL;
		my_heap = m_is_init = false;
	}

	~tChArray() {
		destroy();
	}

	void init(_u32 capacity, iHeap *pi_heap) {
		if(!m_is_init) {
			my_heap = false;
			m_initial = capacity;
			m_capacity = 0;
			m_size = 0;

			if(!(mpi_heap = pi_heap)) {
				mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL);
				my_heap = true;
			}

			m_array.init(INITIAL_CH_ARRAY, mpi_heap);
			m_is_init = true;
		}
	}

	void destroy(void) {
		if(mpi_heap  && m_is_init) {
			_u32 sz = m_array.size();

			for(_u32 i = 0; i  < sz; i++) {
				_T *ptr = m_array[i];
				if(ptr)
					mpi_heap->free(ptr, m_initial * sizeof(_T));
			}

			m_array.destroy();
			m_capacity = m_size = m_initial = 0;

			if(my_heap)
				_gpi_repo_->object_release(mpi_heap);

			mpi_heap = NULL;
			m_is_init = my_heap = false;
		}
	}
};

#endif
