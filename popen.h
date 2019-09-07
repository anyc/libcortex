#ifndef _CRTX_CURL_H
#define _CRTX_CURL_H

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
	
	char *cmd;
};

struct crtx_listener_base *crtx_new_popen_listener(void *options);

void crtx_popen_init();
void crtx_popen_finish();

#endif
