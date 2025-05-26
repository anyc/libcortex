/*
 * This example sets up a fanotify monitoring of a path and everytime another
 * process tries to open this file, this example will ask the user for permission
 * if the other process is allowed to access the file.
 *
 * Mario Kicherer (dev@kicherer.org) 2016
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
	char *buf;
	char title[32];
	struct crtx_dict *data, *actions_dict;
	size_t title_len, buf_len;
	struct crtx_fanotify_listener *lstnr;
	
	
	metadata = (struct fanotify_event_metadata *) event->data.pointer;
	lstnr = (struct crtx_fanotify_listener*) userdata;
	
	if ( (metadata->mask & FAN_OPEN_PERM) || (metadata->mask & FAN_OPEN) ) {
		// replace %s with the file path and store the string in buf
		crtx_fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		// write the process id into the title
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			// create options that are shown to a user
			// format: action_id, action message, ...
			actions_dict = crtx_create_dict("ssss",
				"", "grant", 5, CRTX_DIF_DONT_FREE_DATA,
				"", "Yes", 3, CRTX_DIF_DONT_FREE_DATA,
				"", "deny", 4, CRTX_DIF_DONT_FREE_DATA,
				"", "No", 2, CRTX_DIF_DONT_FREE_DATA
				);
		} else {
			// fanotify does not expect a response
			actions_dict = 0;
		}
		
		// create the payload of the event
		title_len = strlen(title);
		buf_len = strlen(buf);
		data = crtx_create_dict("ssD",
			"title", crtx_stracpy(title, &title_len), title_len, 0,
			"message", crtx_stracpy(buf, &buf_len), buf_len, 0,
			"actions", actions_dict, 0, 0
			);
		
		crtx_print_dict(data);
		
		crtx_dict_unref(data);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			struct fanotify_response access;
			ssize_t srv;
			char *line;
			size_t len;
			
			
			access.fd = metadata->fd;
			
			printf("Type \"grant\" to grant access:\n");
			line = 0;
			srv = getline(&line, &len, stdin);
			
			if (srv > 0) {
				if (!strncmp(line, "grant", 5) || !strncmp(line, "Yes", 5))
					access.response = FAN_ALLOW;
				else
					access.response = FAN_DENY;
				
				ssize_t srv = write(lstnr->fanotify_fd, &access, sizeof(access));
				if (srv != sizeof(access))
					fprintf(stderr, "write() failed\n");
			}
			
			free(line);
		}
		
		close(metadata->fd);
	}
	
	free(buf);
	
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
	
	// request permission before opening a file
	lstnr.init_flags = FAN_CLASS_PRE_CONTENT;
	lstnr.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD;
	
	// just notify if a file has been opened
	// 	lstnr.init_flags = FAN_CLASS_NOTIF;
	// 	lstnr.mask = FAN_OPEN | FAN_EVENT_ON_CHILD;
	
	lstnr.mark_flags = FAN_MARK_ADD;
	// lstnr.mark_flags |= FAN_MARK_MOUNT; // mark complete mount point
	
	lstnr.event_f_flags = O_RDONLY;
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
