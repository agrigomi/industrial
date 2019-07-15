#ifndef __I_MEMORY_H__
#define __I_MEMORY_H__

#include "iBase.h"
#include "iSync.h"

#define I_HEAP		"iHeap"
#define I_LLIST		"iLlist"
#define I_RING_BUFFER 	"iRingBuffer"
#define I_MAP		"iMap"
#define I_BUFFER_MAP	"iBufferMap"
#define I_POOL		"iPool"

typedef struct {
	_u32 robj; // number of reserved objects
	_u32 aobj; // number of active objects
	_u32 dpgs; // number of data pages
	_u32 mpgs; // number of meta pages
}_heap_status_t;

class iHeap:public iBase {
public:
	INTERFACE(iHeap, I_HEAP);
	virtual void *alloc(_u32)=0;
	virtual void free(void *, _u32)=0;
	virtual bool verify(void *, _u32)=0;
	virtual void status(_heap_status_t *p_hs)=0;
};

class iRingBuffer: public iBase {
public:
	INTERFACE(iRingBuffer, I_RING_BUFFER);
	virtual void init(_u32 capacity)=0;
	virtual void destroy(void)=0;
	virtual void push(void *msg, _u16 sz)=0;
	virtual void *pull(_u16 *psz)=0;
	virtual void reset_pull(void)=0;
};

typedef void*	_map_enum_t;

typedef struct {
	_u32	capacity; // current map capacity
	_u32	count; // number of objects
	_u32	collisions; // number of colisions
}_map_status_t;

class iMap: public iBase {
public:
	INTERFACE(iMap, I_MAP);
	virtual HMUTEX lock(HMUTEX hlock=0)=0;
	virtual void unlock(HMUTEX hlock)=0;
	// init hash-table context
	virtual bool init(_u32 capacity, iHeap *pi_heap=0)=0;
	// add new record (if already exists, returns pointer to existing one)
	virtual void *add(const void *key, _u32 sz_key, const void *data, _u32 sz_data, HMUTEX hlock=0)=0;
	// Same as add, but overwrite existing record and returns pointer to new one
	virtual void *set(const void *key, _u32 sz_key, const void *data, _u32 sz_data, HMUTEX hlock=0)=0;
	// remove record
	virtual void del(const void *key, _u32 sz_key, HMUTEX hlock=0)=0;
	// returns the number of records
	virtual _u32 cnt(void)=0;
	// Return pointer to record data if exists, othrewise NULL
	virtual void *get(const void *key, _u32 sz_key, _u32 *sz_data, HMUTEX hlock=0)=0;
	// Clear entire hash-table
	virtual void clr(HMUTEX hlock=0)=0;
	virtual _map_enum_t enum_open(void)=0;
	virtual void enum_close(_map_enum_t en)=0;
	virtual void *enum_first(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0)=0;
	virtual void *enum_next(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0)=0;
	virtual void *enum_current(_map_enum_t en, _u32 *sz_data, HMUTEX hlock=0)=0;
	virtual void enum_del(_map_enum_t en, HMUTEX hlock=0)=0;
	virtual void status(_map_status_t *)=0;
};

class iPool: public iBase {
public:
	INTERFACE(iPool, I_POOL);
	virtual bool init(_u32 data_size, iHeap *pi_heap=0)=0;
	virtual void *alloc(void)=0;
	virtual void free(void *)=0;
	virtual void clear(void)=0;
};

#define LL_VECTOR	1
#define LL_RING		2

class iLlist:public iBase {
public:
	INTERFACE(iLlist, I_LLIST);
	virtual bool init(_u8 mode, _u8 ncol, iHeap *pi_heap=0)=0;
	virtual void uninit(void)=0;

	virtual HMUTEX lock(HMUTEX hlock=0)=0;
	virtual void unlock(HMUTEX hlock)=0;

	// create new empty record
	virtual void *add(_u32 size, HMUTEX hlock=0)=0;

	// get record by index
	virtual void *get(_u32 index, _u32 *p_size, HMUTEX hlock=0)=0;

	// add record at end of list
	virtual void *add(void *rec, _u32 size, HMUTEX hlock=0)=0;

	// insert record
	virtual void *ins(_u32 index, void *rec, _u32 size, HMUTEX hlock=0)=0;

	// remove record by index
	virtual void rem(_u32 index, HMUTEX hlock=0)=0;

	// delete current record
	virtual void del(HMUTEX hlock=0)=0;

	// delete all records
	virtual void clr(HMUTEX hlock=0)=0;

	// return number of records
	virtual _u32 cnt(HMUTEX hlock=0)=0;

	// select column
	virtual void col(_u8 col, HMUTEX hlock=0)=0;

	// set current record by content
	virtual bool sel(void *rec, HMUTEX hlock=0)=0;

	// move record to other column
	virtual bool mov(void *rec, _u8 col, HMUTEX hlock=0)=0;

	// get next record
	virtual void *next(_u32 *p_size, HMUTEX hlock=0)=0;

	// get current record
	virtual void *current(_u32 *p_size, HMUTEX hlock=0)=0;

	// get first record
	virtual void *first(_u32 *p_size, HMUTEX hlock=0)=0;

	// get last record
	virtual void *last(_u32 *p_size, HMUTEX hlock=0)=0;

	// get prev. record
	virtual void *prev(_u32 *p_size, HMUTEX hlock=0)=0;

	// queue mode specific (last-->first-->seccond ...)
	virtual void roll(HMUTEX hlock=0)=0;
};

// buffer I/O operations
#define BIO_INIT	1
#define BIO_READ	2
#define BIO_WRITE	3
#define BIO_UNINIT	4

#define MAX_UDATA64_INDEX	3

typedef void*	HBUFFER;
typedef _u32 _buffer_io_t(_u8 op, void *buffer, _u32 size, void *udata);

typedef struct {
	_u32	b_size;	// buffer size
	_u32	b_all;	// total number of buffers
	_u32	b_free; // number of free buffers
	_u32	b_busy; // number of busy buffers
	_u32	b_dirty; // number of dirty buffers
}_bmap_status_t;

class iBufferMap: public iBase {
public:
	INTERFACE(iBufferMap, I_BUFFER_MAP);
	virtual void init(_u32 buffer_size, _buffer_io_t *pcb_bio, iHeap *pi_heap=0)=0;
	virtual void uninit(void)=0;
	virtual HBUFFER alloc(void *udata=0)=0;
	virtual void free(HBUFFER)=0;
	virtual void *ptr(HBUFFER)=0;
	virtual void dirty(HBUFFER)=0;
	virtual void *get_udata(HBUFFER)=0;
	virtual _u32 size(void)=0;
	virtual void set_udata(HBUFFER, void *)=0;
	virtual void set_udata64(HBUFFER, _u64 udata64, _u8 index)=0;
	virtual _u64 get_udata64(HBUFFER, _u8 index)=0;
	virtual void reset(HBUFFER=0)=0;
	virtual void flush(HBUFFER=0)=0;
	virtual void status(_bmap_status_t *)=0;
};

#endif
