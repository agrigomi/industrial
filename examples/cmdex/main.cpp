#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iNet.h"
#include "iMemory.h"
#include "iCmd.h"

IMPLEMENT_BASE_ARRAY("command_example", 100);

static iStdIO *gpi_stdio = 0;

static _cstr_t extensions[] = {
	"extsync.so",
	"extfs.so",
	"extht.so",
	"extcmd.so",
	0
};

static int g_signal = 0;

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);

	if(r == ERR_NONE) {
		handle(SIGSEGV, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGSEGV\n");
			dump_stack();
			exit(1);
		});
		handle(SIGABRT, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGABRT\n");
			dump_stack();
			exit(1);
		});
		handle(SIGINT, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGINT\n");
		});
		handle(SIGQUIT, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGQUIT\n");
		});
		handle(SIGTSTP, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGTSTP\n");
		});
		handle(SIGPIPE, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGPIPE\n");
		});
		handle(SIGABRT, [](int signum, siginfo_t *info, void *arg) {
			g_signal = signum;
			printf("SIGABRT\n");
			dump_stack();
		});

		iRepository *pi_repo = get_repository();
		iLog *pi_log = dynamic_cast<iLog*>(pi_repo->object_by_iname(I_LOG, RF_ORIGINAL));
		gpi_stdio = dynamic_cast<iStdIO*>(pi_repo->object_by_iname(I_STD_IO, RF_ORIGINAL));

		if(pi_log)
			pi_log->add_listener([](_u8 lmt, _cstr_t msg) {
				_char_t pref = '-';

				if(gpi_stdio) {
					switch(lmt) {
						case LMT_TEXT: pref = 'T'; break;
						case LMT_INFO: pref = 'I'; break;
						case LMT_ERROR: pref = 'E'; break;
						case LMT_WARNING: pref = 'W';break;
					}
					gpi_stdio->ferror("[%c] %s\n", pref, msg);
				}
			});
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

		iCmdHost *pi_cmd_host = dynamic_cast<iCmdHost*>(pi_repo->object_by_iname(I_CMD_HOST, RF_ORIGINAL));

		if(pi_cmd_host && gpi_stdio) {
			_char_t buffer[1024]="";

			for(;;) {
				gpi_stdio->write((_str_t)"cmdex: ", 7);
				if((n = gpi_stdio->reads(buffer, sizeof(buffer)))) {
					if(n > 1) {
						if(memcmp(buffer, "quit\n", 5) == 0)
							break;
						pi_cmd_host->exec(buffer, gpi_stdio);
					}
				} else {
					if(!g_signal) // stdin closed
						break;
				}

				g_signal = 0;
			}

			pi_repo->object_release(pi_cmd_host);

		}

		n = 0;
		while(extensions[n]) {
			pi_repo->extension_unload(extensions[n]);
			n++;
		}

		uninit();
	}

	return r;
}
