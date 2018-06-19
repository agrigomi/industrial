#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iNet.h"
#include "iMemory.h"
#include "iCmd.h"

IMPLEMENT_BASE_ARRAY("command_example", 100);

static iStdIO *gpi_stdio = 0;

void log_listener(_u8 lmt, _str_t msg) {
	_char_t pref = '-';

	if(gpi_stdio) {
		switch(lmt) {
			case LMT_TEXT: pref = 'T'; break;
			case LMT_INFO: pref = 'I'; break;
			case LMT_ERROR: pref = 'E'; break;
			case LMT_WARNING: pref = 'W';break;
		}
		gpi_stdio->fwrite("[%c] %s\n", pref, msg);
	}
}

static _cstr_t extensions[] = {
	"extcmd.so",
	"extnet.so",
	"extnetcmd.so",
	0
};

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);

	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
		gpi_stdio = (iStdIO *)pi_repo->object_by_iname(I_STD_IO, RF_ORIGINAL);

		if(pi_log)
			pi_log->add_listener(log_listener);
		else {
			printf("Unable to create application log !\n");
			return 1;
		}

		// arguments
		iArgs *pi_arg = (iArgs *)pi_repo->object_by_iname(I_ARGS, RF_ORIGINAL);

		if(pi_arg) {
			// retrieve extensions directory from command line
			_str_t ext_dir = pi_arg->value("ext-dir");

			if(!ext_dir) {
				// not found in command line, try from environment
				ext_dir = getenv("LD_LIBRARY_PATH");
				pi_log->fwrite(LMT_WARNING, "cmdex: require '--ext-dir' aggument in command line."
						" Use LD_LIBRARY_PATH=%s", ext_dir);
			}

			// set extensions directory
			pi_repo->extension_dir((ext_dir) ? ext_dir : "./");

			// no longer needed of arguments
			pi_repo->object_release(pi_arg);
		}

		// load extensions
		_u32 n = 0;

		while(extensions[n]) {
			_err_t err = pi_repo->extension_load(extensions[n]);

			if(err != ERR_NONE)
				pi_log->fwrite(LMT_ERROR, "Unable to load extension '%s'; err=%d", extensions[n], err);
			n++;
		}

		iCmdHost *pi_cmd_host = (iCmdHost *)pi_repo->object_by_iname(I_CMD_HOST, RF_ORIGINAL);

		if(pi_cmd_host && gpi_stdio) {
			_char_t buffer[1024]="";

			for(;;) {
				gpi_stdio->write((_str_t)"cmdex: ", 7);
				if(gpi_stdio->reads(buffer, sizeof(buffer))) {
					pi_cmd_host->exec(buffer, gpi_stdio);
				}
			}
		}
	}

	return r;
}
