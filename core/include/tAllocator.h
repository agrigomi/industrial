#ifndef __TALLOCATOR_H__
#define __TALLOCATOR_H__

#include <typeinfo>
#include <cstddef>
#include "iMemory.h"
#include "iRepository.h"

template <typename T>
class tAllocator {
private:
	iHeap	*mpi_heap;

	iHeap *get_heap(void) {
		if(!mpi_heap)
			mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));

		return mpi_heap;
	}
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef const void* const_void_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	tAllocator(){
		mpi_heap = NULL;
	}
	~tAllocator(){
		if(mpi_heap)
			_gpi_repo_->object_release(mpi_heap);
	}

	template <class U> struct rebind { typedef tAllocator<U> other; };
	template <class U> tAllocator(const tAllocator<U>&){}
	pointer address(reference x) const {return &x;}
	const_pointer address(const_reference x) const {
		return &x;
	}
	size_type max_size() const throw() {
		return size_t(-1) / sizeof(value_type);
	}
	pointer allocate(size_type n, const_void_pointer hint = 0) {
		pointer r = NULL;
		iHeap *pi_heap = get_heap();

		if(pi_heap)
			r = static_cast<pointer>(pi_heap->alloc(n * sizeof(T)));

		return r;
	}
	void deallocate(pointer p, size_type n) {
		iHeap *pi_heap = get_heap();

		if(pi_heap)
			pi_heap->free(p, n * sizeof(T));
	}
	void construct(pointer p, const T& val) {
		new (static_cast<void*>(p))T(val);
	}
	void construct(pointer p) {
		new (static_cast<void*>(p))T();
	}
	void destroy(pointer p) {
		p->~T();
	}
	template <typename T2>
	inline bool operator==(const tAllocator<T2>&) const noexcept {
		    return typeid(T) == typeid(T2);
	}

	template <typename T2>
	inline bool operator!=(const tAllocator<T2>&) const noexcept {
		     return typeid(T) != typeid(T2);
	}
};

#endif
