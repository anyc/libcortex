#ifndef _CRTX_POPEN_H
#define _CRTX_POPEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "core.h"
#include "fork.h"
#include "pipe.h"
#include "writequeue.h"

struct crtx_popen_listener {
	struct crtx_listener_base base;
	
	struct crtx_fork_listener fork_lstnr;
	
	struct crtx_writequeue_listener stdin_wq_lstnr;
	struct crtx_pipe_listener stdin_lstnr;
	struct crtx_pipe_listener stdout_lstnr;
	struct crtx_pipe_listener stderr_lstnr;
	
	int fstdin;		// override stdin for the new process (optional)
	int fstdout;	// override stdout for the new process (optional)
	int fstderr;	// override stderr for the new process (optional)
	
	int rc;			// return code of the child process
	
	void (*stdout_cb)(int fd, void *userdata);	// if set, will be called if child process writes to stdout
	void (*stderr_cb)(int fd, void *userdata);	// if set, will be called if child process writes to stderr
	
	void *stdout_cb_data;	// user data that will be passed to the respective callback above
	void *stderr_cb_data;	// user data that will be passed to the respective callback above
	
	const char *filepath; // execute the executable with this file path
	const char *filename; // execute an executable with this filename (e.g., in $PATH)
	char **argv;	// start executable with parameters (must include executable itself)
	char **envp;	// start executable with a custom environment (optional)
	const char *chdir; // change current directory before starting child process (optional)
	
	// execute callback just before execv'ing the given command
	void (*pre_exec)(struct crtx_popen_listener *plstnr, void *pre_exec_userdata);
	void *pre_exec_userdata;
};

struct crtx_listener_base *crtx_setup_popen_listener(void *options);
void crtx_popen_clear_lstnr(struct crtx_popen_listener *lstnr);
void crtx_popen_close_stdin(struct crtx_popen_listener *lstnr);
CRTX_DECLARE_ALLOC_FUNCTION(popen)

void crtx_popen_init();
void crtx_popen_finish();

#ifdef __cplusplus
}
#endif

#endif
