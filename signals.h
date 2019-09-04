#ifndef _CRTX_SIGNALS_H
#define _CRTX_SIGNALS_H

#include <sys/types.h>

#include "core.h"
#include "pipe.h"

#define CRTX_SIGNAL_DEFAULT 0
#define CRTX_SIGNAL_SIGACTION 1
#define CRTX_SIGNAL_SIGNALFD 2
#define CRTX_SIGNAL_SELFPIPE 3

struct signal_map {
	int id;
	char *name;
	char *etype;
	struct crtx_listener_base *lstnr;
};

struct crtx_signal_listener {
	struct crtx_listener_base base;
	
	int *signals; // { SIGINT, ..., 0 }
	char type;
	
	int fd;
	
	void (*signal_handler)(int sig_num);
	
	struct crtx_pipe_listener pipe_lstnr;
	struct crtx_signal_listener *sub_lstnr;
};

int crtx_signals_handle_std_signals(struct crtx_signal_listener **signal_lstnr);

struct crtx_listener_base *crtx_new_signals_listener(void *options);
void crtx_signals_init();
void crtx_signals_finish();

typedef void (*sigchld_cb)(pid_t pid, int status, void *userdata);
int crtx_signals_add_child_handler(sigchld_cb cb, void *userdata);

struct signal_map *crtx_get_signal_info(int signum);

#endif
