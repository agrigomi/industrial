#include "startup.h"

_err_t main(int argc, char *argv[]) {
	_err_t r = init(0);
	if(r == ERR_NONE) {
		//...
	}
	return r;
}
