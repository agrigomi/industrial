#ifndef __I_PROCESS_H__
#define __I_PROCESS_H__

#include <signal.h>
#include "iBase.h"

#define I_PROCESS	"iProcess"

#define HPROCESS	void*

class iProcess: public iBase {
public:
	INTERFACE(iProcess, I_PROCESS);

	// Execute child process and return handle, or NULL for error.
	virtual HPROCESS exec(_cstr_t path, _cstr_t argv[], _cstr_t env[]=NULL)=0;

	// close child process by handle, and return exit status.
	virtual _s32 close(HPROCESS, _s32 signal=SIGINT)=0;

	// Read stdout of a child process with timeout in seconds.
	// Return number of received bytes, or -1 for error.
	virtual _s32 read(HPROCESS, _u8 *ptr, _u32 sz, _u32 timeout=0)=0;

	// Write to stdin of a child process. Return number of written bytes or -1 for error.
	virtual _s32 write(HPROCESS, _u8 *ptr, _u32 sz)=0;

	// Return exit status of a child process, or -1 if still running.
	virtual _s32 status(HPROCESS)=0;

	// Wait for process to change state.
	virtual bool wait(HPROCESS)=0;
};

#endif
