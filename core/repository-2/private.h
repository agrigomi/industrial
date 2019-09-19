#ifndef __REPOSITORY_PRIVATE_H__
#define __REPOSITORY_PRIVATE_H__

#include <mutex>
#include "dtype.h"
#include "map_alg.h"
#include "sha1.h"

typedef struct 	mutex _mutex_t;
typedef _u64	_mutex_handle_t;
typedef struct hash_map _map_t;

struct mutex {
private:
	std::mutex	m_mutex;
	_u32		m_lcount; // lock count
	_u32		m_hcount; // handle count

	_u64 get_handle(void);

public:
	mutex();
	~mutex();
	_mutex_handle_t lock(_mutex_handle_t h=0);
	_mutex_handle_t try_lock(_mutex_handle_t h);
	void unlock(_mutex_handle_t h);
};

// Zone allocator
bool zinit(void);
bool is_zinit(void);
void *zalloc(_u32 size);
void zfree(void *ptr, _u32 size);
void zdestroy(void);
bool zverify(void *ptr, _u32 size);

template <typename T>
class zAllocator {
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef const void* const_void_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	zAllocator(){
		zinit();
	}
	~zAllocator(){}

	template <class U> struct rebind { typedef zAllocator<U> other; };
	template <class U> zAllocator(const zAllocator<U>&){}
	pointer address(reference x) const {return &x;}
	const_pointer address(const_reference x) const {
		return &x;
	}
	size_type max_size() const throw() {
		return size_t(-1) / sizeof(value_type);
	}
	pointer allocate(size_type n, const_void_pointer hint = 0) {
		return static_cast<pointer>(zalloc(n * sizeof(T)));
	}
	void deallocate(pointer p, size_type n) {
		zfree(p, n * sizeof(T));
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
};

// Map
struct hash_map {
private:
	_map_context_t 	m_context;
	_mutex_t	m_mutex;
	SHA1Context	m_sha1;
public:
	hash_map();
	~hash_map();

	_mutex_handle_t lock(_mutex_handle_t hlock=0);
	void unlock(_mutex_handle_t hlock);

	void *add(void *key, _u32 sz_key, void *data, _u32 sz_data, _mutex_handle_t hlock=0);
	void *set(void *key, _u32 sz_key, void *data, _u32 sz_data, _mutex_handle_t hlock=0);
	void *get(void *key, _u32 sz_key, _u32 *sz_data, _mutex_handle_t hlock=0);
	void  del(void *key, _u32 sz_key, _mutex_handle_t hlock=0);
	void  clr(void);
	void  enm(_s32 (*cb)(void *, _u32, void *), void *udata, _mutex_handle_t hlock=0);
};

#endif

