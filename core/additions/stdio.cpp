#include <string.h>
#include <stdio.h>
#include "iIO.h"

class cStdIO: public iStdIO {
private:
public:
	BASE(cStdIO, "cStdIO", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
			case OCTL_UNINIT:
				r = true;
		}

		return r;
	}

	_u32 read(void *data, _u32 size) {
		_u32 r = 0;

		if(fread(data, size, 1, stdin) == 1)
			r = size;

		return r;
	}

	_u32 write(void *data, _u32 size) {
		_u32 r = 0;

		if(::fwrite(data, size, 1, stdout) == 1)
			r = size;

		return r;
	}

	_u32 fwrite(_cstr_t fmt, ...) {
		_u32 r = 0;
		va_list va;

		va_start(va, fmt);
		r = vfprintf(stdout, fmt, va);
		va_end(va);

		return r;
	}

	_u32 reads(_str_t str, _u32 size) {
		memset(str, 0, size);
		fgets(str, size, stdin);
		return strlen(str);
	}
};

static cStdIO _g_stdio_;
