
#ifndef _CRTX_FANOTIFY_H
#define _CRTX_FANOTIFY_H

#include <fcntl.h>
#include <sys/fanotify.h>

#define FANOTIFY_MSG_ETYPE "fanotify/event"
extern char *fanotify_msg_etype[];

struct fanotify_listener {
	struct listener parent;
	
	int fanotify_fd;
	unsigned int init_flags;
	unsigned int event_f_flags;
	unsigned int mark_flags;
	uint64_t mask;
	int dirfd;
	char *path;
	
	pthread_t thread;
};

void fanotify_get_path(int fd, char **path, size_t *length, char *format);

struct listener *new_fanotify_listener(void *options);
void free_fanotify_listener(void *data);

void crtx_fanotify_init();
void crtx_fanotify_finish();

#endif