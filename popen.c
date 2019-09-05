/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "intern.h"
#include "core.h"
#include "popen.h"
#include "fork.h"

#ifndef CRTX_TEST

void reinit_cb(void *reinit_cb_data) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) reinit_cb_data;
	
	printf("exec %s\n", plstnr->cmd);
	
	dup2(plstnr->stdin_lstnr.fds[0], STDIN_FILENO);
// 	close(plstnr->stdin_lstnr.fds[0]);
// 	close(plstnr->stdin_lstnr.fds[1]);
	
	dup2(plstnr->stdout_lstnr.fds[1], STDOUT_FILENO);
// 	close(plstnr->stdout_lstnr.fds[0]);
// 	close(plstnr->stdout_lstnr.fds[1]);
	
	dup2(plstnr->stderr_lstnr.fds[1], STDERR_FILENO);
// 	close(plstnr->stderr_lstnr.fds[0]);
// 	close(plstnr->stderr_lstnr.fds[1]);
	
	rv = execlp(plstnr->cmd, plstnr->cmd, (char*) 0);
	if (rv) {
		ERROR("execlp() failed: %s\n", strerror(errno));
	}
	
	crtx_init_shutdown();
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	plstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	rv = crtx_start_listener(&plstnr->stdin_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	
	plstnr->stdin_wq_lstnr.write_fd = plstnr->stdin_lstnr.fds[1];
// 	plstnr->stdin_wq_lstnr.write = &wq_write;
	rv = crtx_start_listener(&plstnr->stdin_wq_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	
	rv = crtx_start_listener(&plstnr->stdout_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	
	rv = crtx_start_listener(&plstnr->stderr_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	
	rv = crtx_start_listener(&plstnr->fork_lstnr.base);
	if (rv) {
		ERROR("starting fork listener failed\n");
		return rv;
	}
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
// 	int rv;
	
// 	rv = crtx_stop_listener(&plstnr->fork_lstnr.base);
// 	if (rv) {
// 		ERROR("stopping fork listener failed\n");
// 		return rv;
// 	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_popen_listener(void *options) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) options;
	
	plstnr->base.start_listener = &start_listener;
	plstnr->base.stop_listener = &stop_listener;
	
	
	plstnr->fork_lstnr.reinit_cb = &reinit_cb;
	plstnr->fork_lstnr.reinit_cb_data = plstnr;
	
	rv = crtx_create_listener("fork", &plstnr->fork_lstnr);
	if (rv) {
		ERROR("create_listener(fork) failed\n");
		return 0;
	}
	
	rv = crtx_create_listener("writequeue", &plstnr->stdin_lstnr);
	if (rv) {
		ERROR("create_listener(writequeue) failed\n");
		return 0;
	}
	
	rv = crtx_create_listener("pipe", &plstnr->stdout_lstnr);
	if (rv) {
		ERROR("create_listener(pipe) failed\n");
		return 0;
	}
	
	rv = crtx_create_listener("pipe", &plstnr->stderr_lstnr);
	if (rv) {
		ERROR("create_listener(pipe) failed\n");
		return 0;
	}
	
// 	crtx_create_task(fork_lstnr.base.graph, 0, "fork_event_handler", fork_event_handler, 0);
	
	return &plstnr->base;
}

void crtx_popen_init() {
}

void crtx_popen_finish() {
}


#else /* CRTX_TEST */

#define BUF_SIZE 1024
char buffer[BUF_SIZE];

// static char signal_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	
// 	return 0;
// }

static int wq_write(struct crtx_writequeue *wqueue, void *userdata) {
	ssize_t n_bytes;
	
	while (1) {
		n_bytes = write(wqueue->write_fd, buffer, BUF_SIZE);
		if (n_bytes < 0) {
			if (errno == EAGAIN || errno == ENOBUFS) {
				printf("received EAGAIN\n");
				
				crtx_writequeue_start(wqueue);
				
				return EAGAIN;
			} else {
				printf("error %s\n", strerror(errno));
				exit(1);
			}
		} else {
		}
	}
}


int popen_main(int argc, char **argv) {
	struct crtx_popen_listener plist;
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	memset(&plist, 0, sizeof(struct crtx_popen_listener));
	
	plist.cmd = "whoami";
	
	plist.stdin_wq_lstnr.write = &wq_write;
	plist.stdin_wq_lstnr.write_userdata = 0;
	
	ret = crtx_create_listener("popen", &plist);
	if (ret) {
		ERROR("create_listener(popen) failed\n");
		return 0;
	}
	
// 	crtx_create_task(plist.base.graph, 0, "signal_test", &sigchild_event_handler, 0);
	
	ret = crtx_start_listener(&plist.base);
	if (ret) {
		ERROR("starting popen listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	return 0;
}

// CRTX_TEST_MAIN(popen_main);
int main(int argc, char **argv) {
	int i;
	crtx_init();
// 	crtx_handle_std_signals();
	i = popen_main(argc, argv);
	crtx_finish();
	return i;
}
#endif
