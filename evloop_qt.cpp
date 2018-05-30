/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QSocketNotifier>

extern "C" {
#include "intern.h"
#include "core.h"
#include "evloop.h"

#include "evloop_qt.h"
}

static int qt_flags2crtx_event_flags(enum QSocketNotifier::Type qt_flags);

class MyQSocketNotifier : public QSocketNotifier {
// 	Q_OBJECT
	
	using QSocketNotifier::QSocketNotifier;
	
	public:
		struct crtx_evloop_callback *el_cb;
// 		MyQSocketNotifier() {}
// 		MyQSocketNotifier();
// 		virtual ~MyQSocketNotifier() {}
	
	public slots:
		void socketEvent(int socket) {
// 			int readret;
// 			int buf[1024];
// 			qDebug()<<"rxEvent()";
// 			readret = read(socket/*rxfd*/, buf, 2*sizeof(double));
// 			qDebug()<<"Received:"<<buf[0]<<", "<<buf[1]<<" read() returns "<<readret;
			
			this->el_cb->triggered_flags = qt_flags2crtx_event_flags(this->type());
			
// 			printf("asd %d\n", this->el_cb->triggered_flags);
			
			crtx_evloop_callback(this->el_cb);
		}
};

struct eloop_data {
	MyQSocketNotifier *notifiers;
};

#ifndef CRTX_TEST

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

static int qt_flags2crtx_event_flags(enum QSocketNotifier::Type qt_flags) {
	int ret;
	
	if (POPCOUNT32(qt_flags) > 1) {
		ERROR("evloop_qt: %u\n", POPCOUNT32(qt_flags));
	}
	
	if (qt_flags == MyQSocketNotifier::Read)
		ret = EVLOOP_READ;
	else if (qt_flags == MyQSocketNotifier::Write)
		ret = EVLOOP_WRITE;
	else if (qt_flags == MyQSocketNotifier::Exception)
		ret = EVLOOP_SPECIAL;
	
	return ret;
}

extern "C" {

static int crtx_evloop_qt_mod_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
	MyQSocketNotifier *notifier;
	struct crtx_evloop_callback *el_cb;
	
	if (!evloop_fd->el_data) {
		evloop_fd->el_data = (struct eloop_data*) calloc(1, sizeof(struct eloop_data));
	}
	
	for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
		if (!el_cb->active)
			continue;
		
		notifier = new MyQSocketNotifier(evloop_fd->fd, crtx_event_flags2qt_flags(el_cb->crtx_event_flags));
	// 	connect(notifier, SIGNAL(activated(int)), SLOT(rxEvent(int)), Qt::BlockingQueuedConnection );
		
		notifier->connect(notifier, &MyQSocketNotifier::activated, notifier, &MyQSocketNotifier::socketEvent);
		
		notifier->el_cb = el_cb;
		
		notifier->setEnabled(true);
	}
	
// 	evloop_fd->el_data = notifier;
	
	return 0;
}

// static int crtx_evloop_qt_mod_fd(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload) {
// 	
// 	return 0;
// }
// 
// static int crtx_evloop_qt_del_fd(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload) {
// 	
// 	return 0;
// }

static int evloop_start(struct crtx_event_loop *evloop) {
	// 	struct crtx_event_loop_payload *el_payload;
	// 	struct epoll_event ctrl_event;
	
	return 0;
}

static int evloop_stop(struct crtx_event_loop *evloop) {
	
	return 0;
}

static int evloop_create(struct crtx_event_loop *evloop) {
// 	struct crtx_event_loop_payload *el_payload;
// 	struct epoll_event ctrl_event;
	
	return 0;
}

static int evloop_release(struct crtx_event_loop *evloop) {
	
	return 0;
}

static void crtx_evloop_qt_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	
}

struct crtx_event_loop qt_loop = {
	{ 0 },
	"qt",
	{ 0 },
	{ {0} },
	{ {0} },
	0,
	
	&evloop_create,
	&evloop_release,
	&evloop_start,
	&evloop_stop,
	
// 	&crtx_evloop_qt_add_fd,
	&crtx_evloop_qt_mod_fd,
// 	&crtx_evloop_qt_del_fd,
};

void crtx_evloop_qt_init(struct crtx_event_loop *evloop) {
	qt_loop.ll.data = &qt_loop;
	crtx_ll_append((struct crtx_ll**) &crtx_event_loops, &qt_loop.ll);
}

void crtx_evloop_qt_finish(struct crtx_event_loop *evloop) {
	
}

}


#else

#include <QCoreApplication>

extern "C" {
#include "timer.h"
}

struct crtx_timer_listener tlist;
struct crtx_listener_base *blist;

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("received timer event: %u\n", event->data.uint32);
	
	return 1;
}

void setup_timer() {
	memset(&tlist, 0, sizeof(struct crtx_timer_listener));
	
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 5;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME;
	tlist.settime_flags = 0;
	
	blist = create_listener("timer", &tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "timertest", timertest_handler, 0);
	
	crtx_start_listener(blist);
}

int main(int argc, char *argv[]) {
	int ret;
	
	crtx_init();
	
	crtx_set_main_event_loop("qt");
// 	crtx_evloop_qt_start();
	
	QCoreApplication a(argc, argv);
	
	crtx_handle_std_signals();
	
	setup_timer();
	
	ret = a.exec();
	
	crtx_finish();
	
	return ret;
}

#endif
