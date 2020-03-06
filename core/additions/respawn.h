#ifndef __RESPAWN_H__
#define __RESPAWN_H__

#include <sys/types.h>

#define PREAD_FD	fd_in[0]
#define CWRITE_FD	fd_in[1]
#define CREAD_FD	fd_out[0]
#define PWRITE_FD	fd_out[1]

typedef struct {
	/* pipes */
	int	fd_in[2]; /* 0 - parent read / 1 - child write */
	int	fd_out[2]; /* 0 - child read / 1 - parent write */
	/* process PID */
	pid_t	cpid;
	/* process exit status */
	int	status;
}_proc_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Execute a file with arguments and environment.
   return 0 for sucess -1 for error */
int proc_exec_ve(_proc_t *, /* process context */
		const char *path, /* path to executable */
		const char *const argv[], /* array of arguments */
		const char *const envp[] /* array of environment variables */
		);

/* Execute a file with arguments.
   return 0 for sucess -1 for error */
int proc_exec_v(_proc_t *, /* process context */
		const char *path, /* path to executable */
		const char *const argv[] /* array of arguments */
		);

/* Redirect pipes to stdin/stdout of a chile process and
   invoke callback.
   Return 0 for auccess, -1 for error */
int proc_exec_cb(_proc_t *, /* process context */
		int (*)(void *), /* pointer to callback */
		void * /* user data */
		);

/* Read from stdout of a child process.
   return -1 for error, or number of received bytes */
int proc_read(_proc_t *, void *buf, int sz);

/* Read from stdout of a child process with timeout in seconds.
   return -1 for error, or number of received bytes */
int proc_read_ts(_proc_t *, void *buf, int sz, int ts);

/* Write to stdin of a child process.
   return -1 for error, or number of written bytes */
int proc_write(_proc_t *, void *buf, int sz);

/* Return status of child process, if available.
   Othrewise -1 will be returned. */
int proc_status(_proc_t *);

/* wait for process to change state */
int proc_wait(_proc_t *);

/* Do not forget to call this function !!! */
void proc_close_pipe(_proc_t *);

/* kill child process.
   return 0 for success, -1 for error */
int proc_kill(_proc_t *);

/* Send SIGINT to child process.
   return 0 for sucess, -1 for error */
int proc_break(_proc_t *);

/* Send signall to child process
   return 0 for success, -1 for error */
int proc_signal(_proc_t *, int signal);

#ifdef __cplusplus
}
#endif

#endif
