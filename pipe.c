
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
// 	struct crtx_event *nevent;
	struct crtx_pipe_listener *lstnr;
// 	struct crtx_evloop_callback *el_cb;
	
	lstnr = (struct crtx_pipe_listener *) userdata;
	
// 	el_cb = event->data.pointer;
// 	printf("fd %d %d\n", el_cb->fd_entry->fd, lstnr->fds[0]);
	
// 	nevent = crtx_create_event(lstnr->event_type);
	crtx_event_set_raw_data(event, 'i', lstnr->fds[CRTX_READ_END], sizeof(int), 0);
	return lstnr->fd_event_handler(event, lstnr->fd_event_handler_data, 0);
	
// 	crtx_add_event(lstnr->base.graph, nevent);
	
// 	return 0;
}

static void on_error_cb(struct crtx_evloop_callback *el_cb, void *data) {
	struct crtx_pipe_listener *lstnr;
	
	lstnr = (struct crtx_pipe_listener *) data;
	
	crtx_stop_listener(&lstnr->base);
}

static void shutdown_listener(struct crtx_listener_base *lbase) {
	struct crtx_pipe_listener *lstnr;
	
	lstnr = (struct crtx_pipe_listener *) lbase;
	
	DBG("closing %d %d\n", lstnr->fds[CRTX_READ_END], lstnr->fds[CRTX_WRITE_END]);
	
	if (lstnr->fds[CRTX_READ_END] >= 0)
		close(lstnr->fds[CRTX_READ_END]);
	if (lstnr->fds[CRTX_WRITE_END] >= 0)
		close(lstnr->fds[CRTX_WRITE_END]);
}

struct crtx_listener_base *crtx_new_pipe_listener(void *options) {
	struct crtx_pipe_listener *lstnr;
	int r;
	
	
	lstnr = (struct crtx_pipe_listener *) options;
	
	r = pipe2(lstnr->fds, crtx_root->global_fd_flags); // 0 read, 1 write
	if (r < 0) {
		ERROR("creating pipe failed: %s\n", strerror(errno));
		return 0;
	}
	
	VDBG("pipe fds: %d %d\n", lstnr->fds[CRTX_READ_END], lstnr->fds[CRTX_WRITE_END]);
	
	crtx_evloop_init_listener(&lstnr->base,
							  lstnr->fds[CRTX_READ_END],
						EVLOOP_READ,
						0,
						&pipe_fd_event_handler, lstnr,
// 						lstnr->fd_event_handler, lstnr->fd_event_handler_data,
						&on_error_cb, lstnr
					);
	
	lstnr->base.shutdown = &shutdown_listener;
	
	return &lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(pipe)

void crtx_pipe_init() {
}

void crtx_pipe_finish() {
}
