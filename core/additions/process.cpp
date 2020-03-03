#include "respawn.h"
#include "iProcess.h"
#include "iMemory.h"
#include "iRepository.h"

class cProcess: public iProcess {
private:
	iLlist	*mpi_list;

public:
	BASE(cProcess, "cProcess", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				break;
			case OCTL_UNINIT:
				break;
		}

		return r;
	}

	HPROCESS exec(_cstr_t path, _cstr_t argv[], _cstr_t env[]=NULL) {
		HPROCESS r = NULL;

		//...

		return r;
	}

	bool close(HPROCESS, _s32 signal=SIGINT) {
		bool r = false;



		return r;
	}

	_s32 read(HPROCESS, _u8 *ptr, _u32 sz) {
		_s32 r = -1;

		//...

		return r;
	}

	_s32 write(HPROCESS, _u8 *ptr, _u32 sz) {
		_s32 r = -1;

		//...

		return r;
	}

	_s32 status(HPROCESS) {
		_s32 r = -1;

		//...

		return r;
	}

	bool wait(HPROCESS) {
		bool r = false;

		//...

		return r;
	}
};

static cProcess _g_process_;
