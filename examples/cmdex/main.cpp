#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iNet.h"
#include "iMemory.h"
#include "iCmd.h"

IMPLEMENT_BASE_ARRAY("command_example", 100);

static iStdIO *gpi_stdio = 0;
#define BT_BUF_SIZE 100

void signal_handler(int signum, siginfo_t *info, void *) {
	int j, nptrs;
	void *buffer[BT_BUF_SIZE];
	char **strings;

	nptrs = backtrace(buffer, BT_BUF_SIZE);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	would produce similar output to the following: */

	if((strings = backtrace_symbols(buffer, nptrs))) {
		for (j = 0; j < nptrs; j++)
			printf("%s\n", strings[j]);

		free(strings);
	}

	// info->si_addr holds the dereferenced pointer address
	if (info->si_addr == NULL) {
		// This will be thrown at the point in the code
		// where the exception was caused.
		printf("signal %d\n", signum);
	} else {
		// Now restore default behaviour for this signal,
		// and send signal to self.
		signal(signum, SIG_DFL);
		kill(getpid(), signum);
	}
}

void handle(int sig) {
	struct sigaction act; // Signal structure

	act.sa_sigaction = signal_handler; // Set the action to our function.
	sigemptyset(&act.sa_mask); // Initialise signal set.
	act.sa_flags = SA_SIGINFO|SA_NODEFER; // Our handler takes 3 par.
	sigaction(sig, &act, NULL);
}

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
	"extfs.so",
	"extnet.so",
	"extnetcmd.so",
	"exthttp.so",
	0
};

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);

	if(r == ERR_NONE) {
		handle(SIGSEGV); // Set signal action to our handler.
		//handle(SIGPIPE);

		iRepository *pi_repo = get_repository();
		iLog *pi_log = dynamic_cast<iLog*>(pi_repo->object_by_iname(I_LOG, RF_ORIGINAL));
		gpi_stdio = dynamic_cast<iStdIO*>(pi_repo->object_by_iname(I_STD_IO, RF_ORIGINAL));

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

		iCmdHost *pi_cmd_host = dynamic_cast<iCmdHost*>(pi_repo->object_by_iname(I_CMD_HOST, RF_ORIGINAL));

		if(pi_cmd_host && gpi_stdio) {
			_char_t buffer[1024]="";
			_u32 n = 0;
			for(;;) {
				gpi_stdio->write((_str_t)"cmdex: ", 7);
				if((n = gpi_stdio->reads(buffer, sizeof(buffer)))) {
					if(n > 1)
						pi_cmd_host->exec(buffer, gpi_stdio);
				}
			}
		}
	}

	return r;
}
