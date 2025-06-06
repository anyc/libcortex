/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>

#include <QSocketNotifier>

extern "C" {
#include "intern.h"
#include "core.h"
#include "evloop.h"
#include "evloop_qt.h"
#include "timer.h"
}

static int qt_flags2crtx_event_flags(enum QSocketNotifier::Type qt_flags);

class MyQSocketNotifier : public QSocketNotifier {
	using QSocketNotifier::QSocketNotifier;
	
	public:
		struct crtx_evloop_callback *el_cb;
	
	public slots:
		void socketEvent(int socket) {
			this->el_cb->triggered_flags = qt_flags2crtx_event_flags(this->type());
			
			crtx_evloop_callback(this->el_cb);
		}
};

static enum MyQSocketNotifier::Type crtx_event_flags2qt_flags(int crtx_event_flags) {
	enum MyQSocketNotifier::Type ret;
	
	if (crtx_event_flags & EVLOOP_TIMEOUT)
		crtx_event_flags = crtx_event_flags & (~EVLOOP_TIMEOUT);
	
	if (CRTX_POPCOUNT32(crtx_event_flags) != 1) {
		CRTX_ERROR("evloop_qt: invalid amount of event flags %u != 1 (%d)\n", CRTX_POPCOUNT32(crtx_event_flags), crtx_event_flags);
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
	
	ret = 0;
	if (qt_flags == MyQSocketNotifier::Read)
		ret = EVLOOP_READ;
	else if (qt_flags == MyQSocketNotifier::Write)
		ret = EVLOOP_WRITE;
	else if (qt_flags == MyQSocketNotifier::Exception)
		ret = EVLOOP_SPECIAL;
	else
		CRTX_ERROR("evloop_qt: unknown Qt socketnotifier type: %d\n", qt_flags);
	
	return ret;
}

static char timeout_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_evloop_callback *el_cb;
	int r;
	
	
	el_cb = (struct crtx_evloop_callback*) userdata;
	
	r = crtx_create_event(&event);
	if (r) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(r));
	} else {
		crtx_event_set_raw_data(event, 'p', userdata, sizeof(struct crtx_evloop_callback), CRTX_DIF_DONT_FREE_DATA);
		
		el_cb->event_handler(event, el_cb->event_handler_data, 0);
		
		crtx_free_event(event);
	}
	
	return 0;
}

static int crtx_evloop_qt_mod_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
	MyQSocketNotifier *notifier;
	struct crtx_evloop_callback *el_cb;
	QMetaObject::Connection connection;
	
	for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
// 		if (!el_cb->active && el_cb->el_data) {
// 			notifier = (MyQSocketNotifier*) el_cb->el_data;
// 			delete notifier;
// 			
// 			el_cb->el_data = 0;
// 			
// 			continue;
// 		}
		
// 		if (el_cb->el_data)
// 			continue;
		
// 		printf("el-qt mod %d %d\n", evloop_fd->fd, el_cb->active);
// 		crtx_event_flags2str(stdout, el_cb->crtx_event_flags);
// 		printf("\n");
		
		if (el_cb->el_data) {
			notifier = (MyQSocketNotifier*) el_cb->el_data;
			
			if (notifier->isEnabled() != el_cb->active)
				notifier->setEnabled(el_cb->active);
			
			if (el_cb->active) {
				if (el_cb->crtx_event_flags & EVLOOP_TIMEOUT) {
					struct crtx_timer_listener *tlist;
					int r;
					
					
					tlist = (struct crtx_timer_listener*) malloc(sizeof(struct crtx_timer_listener));
					
					memset(tlist, 0, sizeof(struct crtx_timer_listener));
					
					tlist->newtimer.it_value.tv_sec = el_cb->timeout.tv_sec;
					tlist->newtimer.it_value.tv_nsec = el_cb->timeout.tv_nsec;
					
					CRTX_VDBG("setup evloop timeout: %ld %ld\n", el_cb->timeout.tv_sec,el_cb->timeout.tv_nsec );
					
					tlist->clockid = CRTX_DEFAULT_CLOCK;
					// 			tlist->settime_flags = TFD_TIMER_ABSTIME;
					
					tlist->direct_callback = timeout_event_handler;
					tlist->direct_userdata = el_cb;
					
					r = crtx_setup_listener("timer", tlist);
					if (r) {
						CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
						return r;
					}
					
					crtx_start_listener(&tlist->base);
				}
			}
			
			continue;
		}
		
		notifier = new MyQSocketNotifier(evloop_fd->fd, crtx_event_flags2qt_flags(el_cb->crtx_event_flags));
		
		connection = notifier->connect(notifier, &MyQSocketNotifier::activated, notifier, &MyQSocketNotifier::socketEvent);
		if (! (bool) connection) {
			CRTX_ERROR("connect() for MyQSocketNotifier failed\n");
			return 1;
		}
		
		notifier->el_cb = el_cb;
		el_cb->el_data = notifier;
		
		notifier->setEnabled(true);
	}
	
	return 0;
}

static int evloop_start(struct crtx_event_loop *evloop) {
	// Qt's event loop should be already running
	return 0;
}

static int evloop_stop(struct crtx_event_loop *evloop) {
	return 0;
}

static int evloop_create(struct crtx_event_loop *evloop) {
	return 0;
}

static int evloop_release(struct crtx_event_loop *evloop) {
	return 0;
}

static struct crtx_event_loop qt_loop = {
	.ll = { 0 },
	.id = "qt",
	.ctrl_pipe = { 0 },
	.ctrl_pipe_evloop_handler = { {0} },
	.default_el_cb = { {0} },
	.listener = 0,
	.fds = 0,
	
	.create = &evloop_create,
	.release = &evloop_release,
	.start = &evloop_start,
	.stop = &evloop_stop,
	0,
	
	.mod_fd = &crtx_evloop_qt_mod_fd,
};

extern "C" {
	
void crtx_evloop_qt_init(struct crtx_event_loop *evloop) {
	qt_loop.ll.data = &qt_loop;
	crtx_ll_append((struct crtx_ll**) &crtx_event_loops, &qt_loop.ll);
}

void crtx_evloop_qt_finish(struct crtx_event_loop *evloop) {
}

} // extern "C"




#ifdef CRTX_TEST

#include <QCoreApplication>

extern "C" {
#include "timer.h"
}

struct crtx_timer_listener tlist;
int counter = 0;
QCoreApplication *a;

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("received timer event: %u\n", event->data.uint32);
	
	counter++;
	if (counter > 2)
		a->exit(0);
	
	return 1;
}

void setup_timer() {
	int r;
	
	
	memset(&tlist, 0, sizeof(struct crtx_timer_listener));
	
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 5;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME;
	tlist.settime_flags = 0;
	
	r = crtx_setup_listener("timer", &tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		exit(1);
	}
	
	crtx_create_task(tlist.base.graph, 0, "timertest", timertest_handler, 0);
	
	crtx_start_listener(&tlist.base);
}

int main(int argc, char *argv[]) {
	int ret;
	
	crtx_init();
	
	crtx_set_main_event_loop("qt");
	
	a = new QCoreApplication(argc, argv);
	
	crtx_handle_std_signals();
	
	setup_timer();
	
	ret = a->exec();
	
	crtx_finish();
	
	return ret;
}

#endif
