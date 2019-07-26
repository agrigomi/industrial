#ifndef __TVECTOR_H__
#define __TVECTOR_H__

#include "tArray.h"

#define INITIAL_CH_ARRAY	4

template <class _T>
class _LOCAL_ tVector { // Chunked array
private:
	tArray<_T*> 	m_array;
	_u32 		m_capacity;
	_u32 		m_size;
	_u32		m_initial;
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

	bool alloc_chunk(void) {
		bool r = false;
		iHeap *pi_heap = get_heap();

		if(pi_heap) {
			_T *p = (_T *)pi_heap->alloc(m_initial * sizeof(_T));

			if(p) {
				m_array += p;
				m_capacity += m_initial;
				r = true;
			}
		}

		return r;
	}

public:

	tVector(_u32 capacity, iHeap *pi_heap=NULL) {
		m_is_init = false;
		init(capacity, pi_heap);
	}

	tVector() {
		m_capacity = m_size = 0;
		m_initial = 0;
		mpi_heap = NULL;
		my_heap = m_is_init = false;
	}

	~tVector() {
		destroy();
	}

	void init(_u32 capacity, iHeap *pi_heap) {
		if(!m_is_init) {
			my_heap = false;
			m_initial = capacity;
			m_capacity = 0;
			m_size = 0;

			if(!(mpi_heap = pi_heap))
				get_heap();

			m_array.init(INITIAL_CH_ARRAY, mpi_heap);
			m_is_init = true;
		}
	}

	_T& get(_u32 i) {
		_T *r = NULL;

		if(m_is_init) {
			m_mutex.lock();
			if(i < m_size) {
				_u32 chunk = i / m_initial;
				_u32 idx = i % m_initial;
				_T *p = m_array[chunk];

				if(p)
					r = &p[idx];
			}
			m_mutex.unlock();
		}

		return *r;
	}

	_T& operator [](_u32 i) {
		return get(i);
	}

	void add(const _T &item) {
		if(m_is_init) {
			bool a = true;

			m_mutex.lock();
			if(m_size == m_capacity)
				a = alloc_chunk();

			if(a) {
				_u32 chunk = m_size / m_initial;
				_u32 idx = m_size % m_initial;
				_T *p = m_array[chunk];

				p[idx] = item;
				m_size++;
			}
			m_mutex.unlock();
		}
	}

	void operator +=(const _T &item) {
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
			_u32 sz = m_array.size();

			if(pi_heap) {
				for(_u32 i = 0; i  < sz; i++) {
					_T *ptr = m_array[i];

					if(ptr)
						mpi_heap->free(ptr, m_initial * sizeof(_T));
				}
			}

			m_array.clean();
			m_capacity = m_size = 0;
		}
		m_mutex.unlock();
	}

	void destroy(void) {
		m_mutex.lock();
		if(m_is_init && m_capacity) {
			_u32 sz = m_array.size();
			iHeap *pi_heap = get_heap();

			if(pi_heap) {
				for(_u32 i = 0; i  < sz; i++) {
					_T *ptr = m_array[i];

					if(ptr)
						pi_heap->free(ptr, m_initial * sizeof(_T));
				}
			}

			m_array.destroy();
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
