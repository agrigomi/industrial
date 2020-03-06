#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/select.h>
#include "respawn.h"


static int proc_open_pipe(_proc_t *pcxt) {
	int r = -1;

	pcxt->PREAD_FD = pcxt->PWRITE_FD = -1;
	pcxt->CREAD_FD = pcxt->CWRITE_FD = -1;

	if((r = pipe(pcxt->fd_in)) == 0) {
		if((r = pipe(pcxt->fd_out)) == -1)
			proc_close_pipe(pcxt);
	} else
		proc_close_pipe(pcxt);

	return r;
}

void proc_close_pipe(_proc_t *pcxt) {
	if(pcxt->PREAD_FD != -1) {
		close(pcxt->PREAD_FD);
		pcxt->PREAD_FD = -1;
	}
	if(pcxt->PWRITE_FD != -1) {
		close(pcxt->PWRITE_FD);
		pcxt->PWRITE_FD = -1;
	}
	if(pcxt->CREAD_FD != -1) {
		close(pcxt->CREAD_FD);
		pcxt->CREAD_FD = -1;
	}
	if(pcxt->CWRITE_FD != -1) {
		close(pcxt->CWRITE_FD);
		pcxt->CWRITE_FD = -1;
	}
}

static int do_fork(_proc_t *pcxt) {
	int r = -1;

	pcxt->status = -1;
	if((r = proc_open_pipe(pcxt)) == 0) {
		if((pcxt->cpid = fork()) != -1) {
			if(pcxt->cpid == 0) {
				/* for child process */
				dup2(pcxt->CREAD_FD, STDIN_FILENO);
				dup2(pcxt->CWRITE_FD, STDOUT_FILENO);
				proc_close_pipe(pcxt);
			} else if(pcxt->cpid > 0) {
				/* for parent process */
				close(pcxt->CREAD_FD);
				pcxt->CREAD_FD = -1;
				close(pcxt->CWRITE_FD);
				pcxt->CWRITE_FD = -1;
			}
		} else {
			proc_close_pipe(pcxt);
			r = -1;
		}
	}

	return r;
}

int proc_exec_ve(_proc_t *pcxt, /* process context */
		const char *path, /* path to executable */
		const char *const argv[], /* array of arguments */
		const char *const envp[] /* array of environment variables */
		) {
	int r = -1;

	if((r = do_fork(pcxt)) == 0) {
		if(pcxt->cpid == 0)
			/* child process */
			exit(execve(path, (char **)argv, (char **)envp));
	}

	return r;
}

int proc_exec_v(_proc_t *pcxt, /* process context */
		const char *path, /* path to executable */
		const char *const argv[] /* array of arguments */
		) {
	int r = -1;

	if((r = do_fork(pcxt)) == 0) {
		if(pcxt->cpid == 0)
			/* child process */
			exit(execv(path, (char **)argv));
	}

	return r;
}

int proc_exec_cb(_proc_t *pcxt, /* process context */
		int (*pcb)(void *), /* pointer to callback */
		void *udata /* user data */
		) {
	int r = -1;

	if((r = do_fork(pcxt)) == 0) {
		if(pcxt->cpid == 0)
			/* child process */
			exit(pcb(udata));
	}

	return r;
}

/* read from stdout of a child process */
int proc_read(_proc_t *pcxt, void *buf, int sz) {
	return read(pcxt->PREAD_FD, buf, sz);
}

/* read from stdout of a child process with timeout in seconds */
int proc_read_ts(_proc_t *pcxt, void *buf, int sz, int ts) {
	int r = -1;
	fd_set set;
	struct timeval timeout;

	FD_ZERO(&set); /* clear the set */
	FD_SET(pcxt->PREAD_FD, &set); /* add our file descriptor to the set */

	timeout.tv_sec = ts;
	timeout.tv_usec = 0;

	if((r = select(pcxt->PREAD_FD + 1, &set, NULL, NULL, &timeout)) > 0)
		r = read(pcxt->PREAD_FD, buf, sz);

	return r;
}

/* write to stdin of a child process */
int proc_write(_proc_t *pcxt, void *buf, int sz) {
	return write(pcxt->PWRITE_FD, buf, sz);
}

/* Return status of child process, if available.
   Othrewise -1 will be returned. */
int proc_status(_proc_t *pcxt) {
	int r = pcxt->status;

	if(r == -1) {
		if(waitpid(pcxt->cpid, &pcxt->status, WNOHANG) > 0)
			r = WEXITSTATUS(pcxt->status);
	} else
		r = WEXITSTATUS(r);

	return r;
}

/* wait for process to change state */
int proc_wait(_proc_t *pcxt) {
	return waitpid(pcxt->cpid, &pcxt->status, 0);
}

int proc_kill(_proc_t *pcxt) {
	return kill(pcxt->cpid, SIGKILL);
}

int proc_break(_proc_t *pcxt) {
	return kill(pcxt->cpid, SIGINT);
}

int proc_signal(_proc_t *pcxt, int signal) {
	return kill(pcxt->cpid, signal);
}
