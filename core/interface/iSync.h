#ifndef __I_SYNC_H__
#define __I_SYNC_H__

#include "iBase.h"

#define I_MUTEX	"iMutex"
#define I_EVENT	"iEvent"

typedef _u64	HMUTEX;
typedef _u64 	_evt_t;

class iMutex: public iBase {
public:
	INTERFACE(iMutex, I_MUTEX);
	virtual HMUTEX try_lock(HMUTEX=0)=0;
	virtual HMUTEX lock(HMUTEX=0)=0;
	virtual void unlock(HMUTEX)=0;
};

class iEvent: public iBase {
public:
	INTERFACE(iEvent, I_EVENT);
	virtual _evt_t wait(_evt_t mask, _u32 sleep=0)=0;
	virtual _evt_t check(_evt_t mask)=0;
	virtual void set(_evt_t mask)=0;
};
#endif

