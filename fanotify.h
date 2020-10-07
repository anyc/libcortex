#ifndef _CRTX_FANOTIFY_H
#define _CRTX_FANOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <fcntl.h>
#include <sys/fanotify.h>

// #define FANOTIFY_MSG_ETYPE "fanotify/event"
// extern char *fanotify_msg_etype[];

struct crtx_fanotify_listener {
	struct crtx_listener_base base;
	
	int fanotify_fd;
	unsigned int init_flags;
	unsigned int event_f_flags;
	unsigned int mark_flags;
	uint64_t mask;
	int dirfd;
	char *path;
	
	char stop;
};

void crtx_fanotify_get_path(int fd, char **path, size_t *length, char *format);

struct crtx_listener_base *crtx_setup_fanotify_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(fanotify)

void crtx_fanotify_init();
void crtx_fanotify_finish();

#ifdef __cplusplus
}
#endif

#endif
