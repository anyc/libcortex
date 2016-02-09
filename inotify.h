
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
};

struct crtx_listener_base *crtx_new_inotify_listener(void *options);

uint32_t crtx_inotify_string2mask(char *mask_string);
char crtx_inotify_mask2string(uint32_t mask, char *string, size_t *string_size);
void crtx_inotify_init();
void crtx_inotify_finish();

#endif