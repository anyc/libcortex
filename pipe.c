
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "core.h"
#include "intern.h"
#include "pipe.h"

static char pipe_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_pipe_listener *lstnr;
	
	lstnr = (struct crtx_pipe_listener *) userdata;
	
	crtx_event_set_raw_data(event, 'i', lstnr->fds[CRTX_READ_END], sizeof(int), 0);
	return lstnr->fd_event_handler(event, lstnr->fd_event_handler_data, 0);
}

static void on_error_cb(struct crtx_evloop_callback *el_cb, void *data) {
	struct crtx_pipe_listener *lstnr;
	
	lstnr = (struct crtx_pipe_listener *) data;
	
	crtx_stop_listener(&lstnr->base);
}

static char start_listener(struct crtx_listener_base *lbase) {
	// check if fds are still open
	crtx_setup_pipe_listener(lbase);
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *lbase) {
	struct crtx_pipe_listener *lstnr;
	
	lstnr = (struct crtx_pipe_listener *) lbase;
	
	CRTX_DBG("closing %d %d\n", lstnr->fds[CRTX_READ_END], lstnr->fds[CRTX_WRITE_END]);
	
	if (lstnr->fds[CRTX_READ_END] >= 0)
		close(lstnr->fds[CRTX_READ_END]);
	if (lstnr->fds[CRTX_WRITE_END] >= 0)
		close(lstnr->fds[CRTX_WRITE_END]);
}

struct crtx_listener_base *crtx_setup_pipe_listener(void *options) {
	struct crtx_pipe_listener *lstnr;
	int r, reinit;
	
	
	lstnr = (struct crtx_pipe_listener *) options;
	
	reinit = 0;
	if (!crtx_is_fd_valid(lstnr->fds[0]) && !crtx_is_fd_valid(lstnr->fds[1])) {
		reinit = 1;
	} else {
		if (!crtx_is_fd_valid(lstnr->fds[0])) {
			close(lstnr->fds[1]);
			reinit = 1;
		} else
		if (!crtx_is_fd_valid(lstnr->fds[1])) {
			close(lstnr->fds[0]);
			reinit = 1;
		}
	}
	
	if (reinit) {
		r = pipe2(lstnr->fds, crtx_root->global_fd_flags); // 0 read, 1 write
		if (r < 0) {
			CRTX_ERROR("creating pipe failed: %s\n", strerror(errno));
			return 0;
		}
		
		crtx_evloop_init_listener(&lstnr->base,
			&lstnr->fds[CRTX_READ_END], 0,
			CRTX_EVLOOP_READ,
			0,
			&pipe_fd_event_handler, lstnr,
			&on_error_cb, lstnr
			);
		
		lstnr->base.start_listener = &start_listener;
		lstnr->base.shutdown = &shutdown_listener;
		
		CRTX_VDBG("pipe fds: %d %d\n", lstnr->fds[CRTX_READ_END], lstnr->fds[CRTX_WRITE_END]);
	}
	
	return &lstnr->base;
}

void crtx_pipe_clear_lstnr(struct crtx_pipe_listener *lstnr) {
	lstnr->fds[0] = -1;
	lstnr->fds[1] = -1;
}

CRTX_DEFINE_ALLOC_FUNCTION(pipe)

void crtx_pipe_init() {
}

void crtx_pipe_finish() {
}
