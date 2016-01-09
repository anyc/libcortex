
#ifndef _CRTX_FANOTIFY_H
#define _CRTX_FANOTIFY_H

#include <fcntl.h>
#include <sys/fanotify.h>

#define FANOTIFY_MSG_ETYPE "fanotify/event"
extern char *fanotify_msg_etype[];

struct crtx_fanotify_listener {
	struct crtx_listener_base parent;
	
	int fanotify_fd;
	unsigned int init_flags;
	unsigned int event_f_flags;
	unsigned int mark_flags;
	uint64_t mask;
	int dirfd;
	char *path;
	
	pthread_t thread;
};

void crtx_fanotify_get_path(int fd, char **path, size_t *length, char *format);

struct crtx_listener_base *crtx_new_fanotify_listener(void *options);

void crtx_fanotify_init();
void crtx_fanotify_finish();

#endif
