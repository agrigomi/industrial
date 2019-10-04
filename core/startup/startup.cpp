#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include "startup.h"
#include "iMemory.h"
#include "iLog.h"
#include "iArgs.h"
#include "iTaskMaker.h"

#ifdef _CORE_
_EXPORT_ iRepository *_gpi_repo_ = 0;
_EXPORT_ iRepository *get_repository(void) {
#else
_LOCAL_ iRepository *_gpi_repo_ = 0;
_LOCAL_ iRepository *get_repository(void) {
#endif
	return _gpi_repo_;
}

#ifdef _CORE_
void _EXPORT_ register_object(iBase *pi_base) {
#else
void _LOCAL_ register_object(iBase *pi_base) {
#endif
	if(pi_base) {
		_base_entry_t e={pi_base, 0, 0};
		add_base_entry(&e);
	}
}

#ifdef _CORE_

#define BT_BUF_SIZE 100

void _EXPORT_ dump_stack(void) {
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
}

static void signal_action(int signum, siginfo_t *info, void *) {
	dump_stack();

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

void _EXPORT_ handle(int sig, _sig_action_t *pf_action) {
	struct sigaction act; // Signal structure

	act.sa_sigaction = (pf_action) ? pf_action : signal_action; // Set the action to our function.
	sigemptyset(&act.sa_mask); // Initialise signal set.
	act.sa_flags = SA_SIGINFO|SA_NODEFER; // Our handler takes 3 par.
	sigaction(sig, &act, NULL);
}

typedef struct {
	_cstr_t iname;
	_base_entry_t  *p_entry;
}_early_init_t;

_early_init_t ei[]= {
	{I_REPOSITORY,	0}, //0
	{I_ARGS,	0}, //1
	{I_LOG,		0}, //2
	{0,		0}
};

void _EXPORT_ uninit(void) {
	if(_gpi_repo_) {
		_gpi_repo_->destroy();
		// uninit original heap
		if(ei[1].p_entry)
			ei[1].p_entry->pi_base->object_ctl(OCTL_UNINIT, _gpi_repo_);
	}
}

_err_t _EXPORT_ init(int argc, char *argv[]) {
	_err_t r = ERR_UNKNOWN;
	_u32 count, limit;
	_base_entry_t *array = get_base_array(&count, &limit);
	_u32 i = 0;

	while(array && i < count) {
		_object_info_t oinfo;
		if(array[i].pi_base) {
			array[i].pi_base->object_info(&oinfo);
			for(_u32 j = 0; ei[j].iname; j++) {
				if(strcmp(oinfo.iname, ei[j].iname) == 0) {
					ei[j].p_entry = &array[i];
					break;
				}
			}
		}
		i++;
	}

	if(ei[0].p_entry && (_gpi_repo_ = (iRepository*)ei[0].p_entry->pi_base)) {
		// the repo is here !
		// init repository object
		if(!_gpi_repo_->object_ctl(OCTL_INIT, 0))
			goto _init_done_;
		ei[0].p_entry->ref_cnt++;
		ei[0].p_entry->state |= ST_INITIALIZED;
		_gpi_repo_->init_array(array, count);
		// init args
		iArgs *pi_args = 0;
		if(ei[1].p_entry && (pi_args = (iArgs*)ei[1].p_entry->pi_base)) {
			// args is here
			if(pi_args->object_ctl(OCTL_INIT, _gpi_repo_)) {
				ei[1].p_entry->state |= ST_INITIALIZED;
				ei[1].p_entry->ref_cnt++;
				pi_args->init(argc, argv);
			}
		}
		// init log
		iLog *pi_log = 0;
		if(ei[2].p_entry &&
				!(ei[2].p_entry->state & ST_INITIALIZED) &&
				(pi_log = (iLog*)ei[2].p_entry->pi_base)) {
			// log is here
			if(pi_log->object_ctl(OCTL_INIT, _gpi_repo_)) {
				ei[2].p_entry->state |= ST_INITIALIZED;
				ei[2].p_entry->ref_cnt++;
			}
		}
	}
_init_done_:
#else // EXTENSION
_err_t _EXPORT_ init(iRepository *pi_repo) {
	_gpi_repo_ = pi_repo;
	_err_t r = ERR_NONE;
#endif
	return r;
}
