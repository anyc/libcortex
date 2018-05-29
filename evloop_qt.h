#ifndef CRTX_EVLOOP_QT_H
#define CRTX_EVLOOP_QT_H

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */



struct crtx_evloop_qt_listener {
	struct crtx_listener_base parent;
	
	void *eloop;
};

// void crtx_evloop_qt_add_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_mod_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_del_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph);
void crtx_evloop_qt_start(struct crtx_event_loop *evloop);
void crtx_evloop_qt_stop(struct crtx_event_loop *evloop);

#endif
