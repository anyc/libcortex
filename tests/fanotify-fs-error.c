/*
 * This example sets up a filesystem error monitoring using fanotify. Please
 * note that error reporting over fanotify requires explicit support by the
 * respective filesystem driver.
 *
 * Mario Kicherer (dev@kicherer.org) 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "core.h"
#include "fanotify.h"

struct crtx_fanotify_listener lstnr;

static int event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	struct fanotify_event_info_header *info;
	struct fanotify_event_info_error *err;
	int off;
	
	metadata = (struct fanotify_event_metadata *) event->data.pointer;
	
	if (metadata->fd != FAN_NOFD)
		return 0;
	
	if (metadata->mask == FAN_FS_ERROR) {
		off = sizeof(struct fanotify_event_metadata);
		while (off < metadata->event_len) {
			info = (struct fanotify_event_info_header *) ((char *) metadata + off);
			
			switch (info->info_type) {
				case FAN_EVENT_INFO_TYPE_ERROR:
					err = (struct fanotify_event_info_error *) info;
					printf("received FS error: %d count: %d\n", err->error, err->error_count);
					// for example, 117 means EFSCORRUPTED with ext4
					// see https://elixir.bootlin.com/linux/v6.15/source/fs/ext4/ext4.h#L3859
					break;
				default:
					break;
			}
			
			off += info->len;
		}
	}
	
	return 0;
}

int main(int argc, char **argv) {
	int rv;
	
	
	memset(&lstnr, 0, sizeof(struct crtx_fanotify_listener));
	
	if (argc > 1) {
		lstnr.path = argv[1];
	} else {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		return 1;
	}
	
	crtx_init();
	crtx_handle_std_signals();
	
	lstnr.init_flags = FAN_CLASS_NOTIF|FAN_REPORT_FID;
	lstnr.event_f_flags = O_RDONLY;
	
	lstnr.mark_flags = FAN_MARK_ADD|FAN_MARK_FILESYSTEM;
	lstnr.mask = FAN_FS_ERROR;
	lstnr.dirfd = AT_FDCWD;
	
	rv = crtx_setup_listener("fanotify", &lstnr);
	if (rv) {
		fprintf(stderr, "cannot create fanotify lstnr\n");
		return 0;
	}
	
	// add function that will process a fanotify event
	crtx_create_task(lstnr.base.graph, 0, "event_handler", &event_handler, &lstnr.base);
	
	crtx_start_listener(&lstnr.base);
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
