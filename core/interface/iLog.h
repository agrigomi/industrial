#ifndef __I_LOG_H__
#define __I_LOG_H__

#include "iBase.h"
#include "iSync.h"

#define I_LOG	"iLog"

/* log message type */
#define LMT_NONE	0
#define LMT_TEXT	0
#define LMT_INFO	1
#define LMT_WARNING	2
#define LMT_ERROR	3

typedef void _log_listener_t(_u8 lmt, _str_t msg);

class iLog: public iBase {
public:
	INTERFACE(iLog, I_LOG);
	virtual void init(_u32 capacity)=0;
	virtual void add_listener(_log_listener_t *)=0;
	virtual void remove_listener(_log_listener_t *)=0;
	virtual void write(_u8 lmt, _cstr_t msg)=0;
	virtual void fwrite(_u8 lmt, _cstr_t fmt, ...)=0;
	virtual _str_t first(HMUTEX=0)=0;
	virtual _str_t next(HMUTEX=0)=0;
	virtual HMUTEX lock(HMUTEX=0)=0;
	virtual void unlock(HMUTEX)=0;
};

#endif
