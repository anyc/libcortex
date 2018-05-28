/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "evloop.h"

// #include <sys/epoll.h>
// #ifdef CRTX_EVLOOP_X
// #else
// #include "epoll.h"
// #endif

struct crtx_event_loop *crtx_event_loops = 0;

struct crtx_event_loop* crtx_get_event_loop() {
	if (!crtx_root->event_loop.listener) {
		struct crtx_listener_base *lbase;
		int ret;
		struct crtx_ll *ll;
		char *chosen_evloop;
		
		chosen_evloop = getenv("CRTX_EVLOOP");
		
		if (!chosen_evloop) {
			chosen_evloop = DEFAULT_EVLOOP
		}
		
		for (ll=crtx_event_loops; ll; ll = ll->next) {
			if (!strcmp(ll->id, chosen_evloop)) {
				memcpy(&crtx_root->event_loop, ll->data, sizeof(struct crtx_event_loop));
				
				crtx_root->event_loop.create(&crtx_root->event_loop);
			}
		}
		
// 		#ifdef CRTX_EVLOOP_X
// 		#else
// 		crtx_root->event_loop.listener = (struct crtx_epoll_listener*) calloc(1, sizeof(struct crtx_epoll_listener));
// 		#endif
// 		
// 		crtx_root->event_loop.add_fd = &crtx_epoll_add_fd;
// 		crtx_root->event_loop.mod_fd = &crtx_epoll_mod_fd;
// 		crtx_root->event_loop.del_fd = &crtx_epoll_del_fd;
// 		
// 		lbase = create_listener("epoll", crtx_root->event_loop.listener);
// 		if (!lbase) {
// 			ERROR("create_listener(epoll) failed\n");
// 			exit(1);
// 		}
		
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

void crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	crtx_epoll_queue_graph(evloop->listener, graph);
}

void crtx_evloop_start(struct crtx_event_loop *evloop) {
	crtx_epoll_main(evloop->listener);
}

void crtx_evloop_stop(struct crtx_event_loop *evloop) {
	crtx_epoll_stop(evloop->listener);
}
