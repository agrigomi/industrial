#include "private.h"

static iFS *gpi_fs = NULL;
static iLog *gpi_log = NULL;

void sync_init(iFS *pi_fs, iLog *pi_log) {
	gpi_fs = pi_fs;
	gpi_log = pi_log;
}

void do_sync(void) {
	//...
}
