#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <atomic>

struct spinlock {
private:
	std::atomic_flag status = ATOMIC_FLAG_INIT;

public:
	void lock(void) {
		while (status.test_and_set(std::memory_order_acquire)) { ; }
	}
	void unlock(void) {
		status.clear(std::memory_order_release);
	}
	bool try_lock(void) {
		return !status.test_and_set(std::memory_order_acquire);
	}
};

typedef struct spinlock _spinlock_t;

#endif
