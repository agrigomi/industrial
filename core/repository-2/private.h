#ifndef __REPOSITORY_PRIVATE_H__
#define __REPOSITORY_PRIVATE_H__

#include <mutex>
#include "dtype.h"

typedef struct 	mutex _mutex_t;
typedef _u64	_mutex_handle_t;

struct mutex {
private:
	std::mutex	m_mutex;
	_u32		m_lcount; // lock count
	_u32		m_hcount; // handle count

	_u64 get_handle(void);

public:
	_mutex_handle_t lock(_mutex_handle_t h);
	_mutex_handle_t try_lock(_mutex_handle_t h);
	void unlock(_mutex_handle_t h);
};

// Zone allocator
bool zinit(void);
void *zalloc(_u32 size);
void zfree(void *ptr, _u32 size);
void zdestroy(void);
bool zverify(void *ptr, _u32 size);

#endif

