/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "evloop.h"

#include <sys/epoll.h>

struct crtx_event_loop* crtx_get_event_loop() {
	if (!crtx_root->event_loop.listener) {
		struct crtx_listener_base *lbase;
		int ret;
		
		crtx_root->event_loop.listener = (struct crtx_epoll_listener*) calloc(1, sizeof(struct crtx_epoll_listener));
		
		crtx_root->event_loop.add_fd = &crtx_epoll_add_fd;
		crtx_root->event_loop.mod_fd = &crtx_epoll_mod_fd;
		crtx_root->event_loop.del_fd = &crtx_epoll_del_fd;
		
		lbase = create_listener("epoll", crtx_root->event_loop.listener);
		if (!lbase) {
			ERROR("create_listener(epoll) failed\n");
			exit(1);
		}
		
		if (crtx_root->detached_event_loop) {
			ret = crtx_start_listener(lbase);
			if (ret) {
				ERROR("starting epoll listener failed\n");
				return 0;
			}
		}
	} 
	
	return &crtx_root->event_loop;
}
