/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QtCore/QSocketNotifier>

extern "C" {
#include "intern.h"
#include "core.h"
#include "evloop.h"

#include "evloop_qt.h"
}

class MyQSocketNotifier : public QSocketNotifier {
	Q_OBJECT
	
	using QSocketNotifier::QSocketNotifier;
		
	public slots:
		void socketEvent(int socket) {
// 			int readret;
// 			int buf[1024];
// 			qDebug()<<"rxEvent()";
// 			readret = read(socket/*rxfd*/, buf, 2*sizeof(double));
// 			qDebug()<<"Received:"<<buf[0]<<", "<<buf[1]<<" read() returns "<<readret;
			printf("asdasd %d\n", socket);
		}
};

static enum MyQSocketNotifier::Type crtx_event_flags2qt_flags(int crtx_event_flags) {
	enum MyQSocketNotifier::Type ret;
	
	if (POPCOUNT32(crtx_event_flags) > 1) {
		ERROR("evloop_qt: %u\n", POPCOUNT32(crtx_event_flags));
	}
	
	if (crtx_event_flags & EVLOOP_READ)
		ret = MyQSocketNotifier::Read;
	else if (crtx_event_flags & EVLOOP_WRITE)
		ret = MyQSocketNotifier::Write;
	else if (crtx_event_flags & EVLOOP_SPECIAL)
		ret = MyQSocketNotifier::Exception;
	
	return ret;
}

extern "C" {

void crtx_evloop_qt_add_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	MyQSocketNotifier *notifier;
	
	notifier = new MyQSocketNotifier(el_payload->fd, crtx_event_flags2qt_flags(el_payload->crtx_event_flags));
// 	connect(notifier, SIGNAL(activated(int)), SLOT(rxEvent(int)), Qt::BlockingQueuedConnection );
	
	notifier->connect(notifier, &MyQSocketNotifier::activated, notifier, &MyQSocketNotifier::socketEvent);
	
	notifier->setEnabled(true);
	
	el_payload->el_data = notifier;
}

void crtx_evloop_qt_mod_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	
}

void crtx_evloop_qt_del_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	
}

void crtx_evloop_qt_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	
}

void crtx_evloop_qt_start(struct crtx_event_loop *evloop) {
// 	evloop->eloop = new EvloopQt();
}

void crtx_evloop_qt_stop(struct crtx_event_loop *evloop) {
	
}

}
