#ifndef _CRTX_POPEN_H
#define _CRTX_POPEN_H

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
	
	struct crtx_writequeue stdin_wq_lstnr;
	struct crtx_pipe_listener stdin_lstnr;
	struct crtx_pipe_listener stdout_lstnr;
	struct crtx_pipe_listener stderr_lstnr;
	
	int stdin;
	int stdout;
	int stderr;
	
	void (*stdin_cb)(int fd, void *userdata);
	void (*stdout_cb)(int fd, void *userdata);
	void (*stderr_cb)(int fd, void *userdata);
	
	void *stdin_cb_data;
	void *stdout_cb_data;
	void *stderr_cb_data;
	
	char *filepath; // execute the executable with this file path
	char *filename; // execute an executable with this filename (e.g., in $PATH)
	char **argv;
	char **envp;
	char *chdir;
	
	void (*pre_exec)(struct crtx_popen_listener *plstnr, void *pre_exec_userdata);
	void *pre_exec_userdata;
};

struct crtx_listener_base *crtx_new_popen_listener(void *options);
void crtx_popen_clear_lstnr(struct crtx_popen_listener *lstnr);
CRTX_DECLARE_ALLOC_FUNCTION(popen)

void crtx_popen_init();
void crtx_popen_finish();

#endif
