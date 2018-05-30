#ifndef CRTX_EVLOOP_QT_H
#define CRTX_EVLOOP_QT_H

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

// #ifdef __cplusplus
// 
// #include "intern.h"
// #include "core.h"
// #include "evloop.h"
// 
// #include "evloop_qt.h"
// 
// 
// class MyQSocketNotifier : public QSocketNotifier {
// // 	Q_OBJECT
// 	
// 	using QSocketNotifier::QSocketNotifier;
// 	
// public:
// 	struct crtx_evloop_callback *el_cb;
// 	// 		MyQSocketNotifier() {}
// // 	MyQSocketNotifier();
// // 	virtual ~MyQSocketNotifier() {}
// 	
// public slots:
// 	void socketEvent(int socket) {
// 		// 			int readret;
// 		// 			int buf[1024];
// 		// 			qDebug()<<"rxEvent()";
// 		// 			readret = read(socket/*rxfd*/, buf, 2*sizeof(double));
// 		// 			qDebug()<<"Received:"<<buf[0]<<", "<<buf[1]<<" read() returns "<<readret;
// 		printf("asdasd %d\n", socket);
// 	}
// };
// 
// #endif

struct crtx_evloop_qt_listener {
	struct crtx_listener_base base;
	
	void *eloop;
};

// void crtx_evloop_qt_add_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_mod_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_del_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_evloop_qt_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph);
void crtx_evloop_qt_start(struct crtx_event_loop *evloop);
void crtx_evloop_qt_stop(struct crtx_event_loop *evloop);

#endif
