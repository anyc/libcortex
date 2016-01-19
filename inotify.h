
#ifndef _CRTX_INOTIFY_H
#define _CRTX_INOTIFY_H

#include <sys/inotify.h>

#define INOTIFY_MSG_ETYPE "inotify/event"
extern char *inotify_msg_etype[];

struct crtx_inotify_listener {
	struct crtx_listener_base parent;
	
	int wd;
	uint32_t mask;
	char *path;
	
	char stop;
};

struct crtx_listener_base *crtx_new_inotify_listener(void *options);

void crtx_inotify_init();
void crtx_inotify_finish();

#endif