
#ifndef _CRTX_INOTIFY_H
#define _CRTX_INOTIFY_H

#include <sys/inotify.h>

#define INOTIFY_MSG_ETYPE "inotify/event"
extern char *inotify_msg_etype[];

struct inotify_listener {
	struct listener parent;
	
	int wd;
	uint32_t mask;
	char *path;
	
	pthread_t thread;
};

struct listener *new_inotify_listener(void *options);

void crtx_inotify_init();
void crtx_inotify_finish();

#endif