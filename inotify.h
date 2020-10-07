#ifndef _CRTX_INOTIFY_H
#define _CRTX_INOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <sys/inotify.h>

// #define INOTIFY_MSG_ETYPE "inotify/event"
// extern char *inotify_msg_etype[];

struct crtx_inotify_listener {
	struct crtx_listener_base base;
	
	int wd;
	uint32_t mask;
	char *path;
};

uint32_t crtx_inotify_string2mask(char *mask_string);
char crtx_inotify_mask2string(uint32_t mask, char *string, size_t *string_size);

struct crtx_listener_base *crtx_setup_inotify_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(inotify)

void crtx_inotify_init();
void crtx_inotify_finish();

#ifdef __cplusplus
}
#endif

#endif
