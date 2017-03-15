
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <libvirt/virterror.h>

#include "core.h"
#include "libvirt.h"
#include "timer.h"
#include "epoll.h"

// See https://github.com/libvirt/libvirt/blob/master/examples/object-events/event-test.c

struct libvirt_wrapper {
	int id;
	
	struct crtx_event_loop_payload el_payload;
	
	int timeout;
	struct crtx_timer_listener timer_listener;
	
	virEventHandleCallback event_cb;
	virEventTimeoutCallback time_cb;
	
	void * opaque;
	
	virFreeCallback ff;
};

static struct crtx_ll * event_list = 0;
static struct crtx_ll * timeout_list = 0;


int virEventPollToNativeEvents(int events) {
	int ret = 0;
	if (events & VIR_EVENT_HANDLE_READABLE)
		ret |= POLLIN;
	if (events & VIR_EVENT_HANDLE_WRITABLE)
		ret |= POLLOUT;
	if (events & VIR_EVENT_HANDLE_ERROR)
		ret |= POLLERR;
	if (events & VIR_EVENT_HANDLE_HANGUP)
		ret |= POLLHUP;
	return ret;
}

int virEventPollFromNativeEvents(int events) {
	int ret = 0;
	if (events & POLLIN)
		ret |= VIR_EVENT_HANDLE_READABLE;
	if (events & POLLOUT)
		ret |= VIR_EVENT_HANDLE_WRITABLE;
	if (events & POLLERR)
		ret |= VIR_EVENT_HANDLE_ERROR;
	if (events & POLLNVAL) /* Treat NVAL as error, since libvirt doesn't distinguish */
		ret |= VIR_EVENT_HANDLE_ERROR;
	if (events & POLLHUP)
		ret |= VIR_EVENT_HANDLE_HANGUP;
	return ret;
}

static void el_callback(struct crtx_event_loop_payload *el_payload) {
	struct libvirt_wrapper *wrap;
	struct epoll_event *epoll_event;
	
	
	wrap = (struct libvirt_wrapper*) el_payload->data;
	
	epoll_event = (struct epoll_event *) el_payload->el_data;
	
// 	printf("event %d %d\n", wrap->id, wrap->el_payload.fd);
	
	wrap->event_cb(wrap->id, wrap->el_payload.fd, virEventPollFromNativeEvents(epoll_event->events), wrap->opaque);
}


static void el_error_cb(struct crtx_event_loop_payload *el_payload, void *data) {
	struct libvirt_wrapper *wrap;
	
	wrap = (struct libvirt_wrapper*) el_payload->data;
	printf("error\n");
	wrap->ff(wrap->opaque);
}

static int crtx_virEventAddHandleFunc(int fd, int event, virEventHandleCallback cb, void * opaque, virFreeCallback ff) {
	struct libvirt_wrapper *wrap;
	struct crtx_ll *it;
	
	wrap = (struct libvirt_wrapper*) calloc(1, sizeof(struct libvirt_wrapper));
	
	wrap->el_payload.fd = fd;
	wrap->el_payload.event_flags = virEventPollToNativeEvents(event);
	wrap->el_payload.simple_callback = &el_callback;
	wrap->el_payload.error_cb = &el_error_cb;
	wrap->el_payload.data = wrap;
	wrap->event_cb = cb;
	wrap->opaque = opaque;
	wrap->ff = ff;
	
	wrap->id = 0;
	if (event_list) {
		for (it=event_list;it->next;it=it->next) {
			if (wrap->id < ((struct libvirt_wrapper*) it->data)->id)
				break;
			if (wrap->id == ((struct libvirt_wrapper*) it->data)->id)
				wrap->id++;
		}
		
		if (wrap->id == ((struct libvirt_wrapper*) it->data)->id)
			wrap->id++;
	}
	
	printf("new id %d fd %d flag %d\n", wrap->id, fd, wrap->el_payload.event_flags);
	
	crtx_ll_append_new(&event_list, wrap);
	
	crtx_root->event_loop.add_fd(&crtx_root->event_loop.listener->parent, &wrap->el_payload);
	
	return wrap->id;
}

static void crtx_virEventUpdateHandleFunc(int watch, int event) {
	struct crtx_ll *it;
	struct libvirt_wrapper *wrap;
	
	for (it=event_list; it; it=it->next) {
		if ( ((struct libvirt_wrapper*) it->data)->id == watch)
			break;
	}
	
	if (!it) {
		ERROR("libvirt: no watch with id %d found\n", watch);
		return;
	}
	
	wrap = ((struct libvirt_wrapper*) it->data);
	wrap->el_payload.event_flags = virEventPollToNativeEvents(event);
	
	printf("update %d flags %d\n", wrap->id, wrap->el_payload.event_flags);
	
	crtx_root->event_loop.mod_fd(&crtx_root->event_loop.listener->parent, &wrap->el_payload);
}

static int crtx_virEventRemoveHandleFunc(int watch) {
	struct crtx_ll *it;
	struct libvirt_wrapper *wrap;
	
	for (it=event_list; it; it=it->next) {
		if ( ((struct libvirt_wrapper*) it->data)->id == watch)
			break;
	}
	
	if (!it) {
		ERROR("libvirt: no watch with id %d found\n", watch);
		return -1;
	}
	
	wrap = ((struct libvirt_wrapper*) it->data);
	printf("rem %d\n", wrap->id);
	crtx_root->event_loop.del_fd(&crtx_root->event_loop.listener->parent, &wrap->el_payload);
	
	crtx_ll_unlink(&event_list, it);
	
	free(it);
	free(wrap);
	
	
	
	return 0;
}

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct libvirt_wrapper *wrap;
	
	wrap = (struct libvirt_wrapper*) userdata;
// 	printf("ev cb\n");
	wrap->time_cb(wrap->id, wrap->opaque);
	
	return 1;
}

static int crtx_virEventAddTimeoutFunc(int timeout, virEventTimeoutCallback cb, void * opaque, virFreeCallback ff) {
	struct libvirt_wrapper *wrap;
	struct crtx_ll *it;
	
	wrap = (struct libvirt_wrapper*) calloc(1, sizeof(struct libvirt_wrapper));
	
	wrap->timeout = timeout;
	wrap->time_cb = cb;
	wrap->opaque = opaque;
	wrap->ff = ff;
	
	wrap->id = 0;
	if (timeout_list) {
		for (it=timeout_list;it->next;it=it->next) {
			if (wrap->id < ((struct libvirt_wrapper*) it->data)->id)
				break;
			if (wrap->id == ((struct libvirt_wrapper*) it->data)->id)
				wrap->id++;
		}
		
		if (wrap->id == ((struct libvirt_wrapper*) it->data)->id)
			wrap->id++;
	}
	
	printf("add timeout %d\n", timeout);
	printf("new id %d\n", wrap->id);
	
	crtx_ll_append_new(&timeout_list, wrap);
	
	
// 	struct crtx_timer_listener tlist;
// 	struct itimerspec newtimer;
	struct crtx_listener_base *blist;
	
	if (timeout > 0) {
		time_t secs = timeout / 1000;
		long nsecs = timeout % 1000 * 1000000;
		
		wrap->timer_listener.newtimer.it_value.tv_sec = secs;
		wrap->timer_listener.newtimer.it_value.tv_nsec = nsecs;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = secs;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = nsecs;
	} else
	if (timeout == 0) { // interrupt immediately
		wrap->timer_listener.newtimer.it_value.tv_sec = 0;
		wrap->timer_listener.newtimer.it_value.tv_nsec = 1;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = 0;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = 1;
	} else {
		wrap->timer_listener.newtimer.it_value.tv_sec = 0;
		wrap->timer_listener.newtimer.it_value.tv_nsec = 0;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = 0;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = 0;
	}
	
	wrap->timer_listener.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	wrap->timer_listener.settime_flags = 0;
// 	wrap->timer_listener.newtimer = &newtimer;
	
	blist = create_listener("timer", &wrap->timer_listener);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "libvirt_timer", timertest_handler, wrap);
	
// 	if (timeout > -1)
	crtx_start_listener(blist);
	
// 	crtx_root->event_loop.add_fd(&crtx_root->event_loop.listener->parent, &wrap->el_payload);
	
	return wrap->id;
}

static void crtx_virEventUpdateTimeoutFunc(int timer, int timeout) {
	struct crtx_ll *it;
	struct libvirt_wrapper *wrap;
// 	struct itimerspec newtimer;
	
	for (it=timeout_list; it; it=it->next) {
		if ( ((struct libvirt_wrapper*) it->data)->id == timer)
			break;
	}
	
	if (!it) {
		ERROR("libvirt: no timer with id %d found\n", timer);
		return;
	}
	
	wrap = ((struct libvirt_wrapper*) it->data);
	
	
	wrap->timeout = timeout;
	
	if (timeout > 0) {
		time_t secs = timeout / 1000;
		long nsecs = timeout % 1000 * 1000000;
		
		wrap->timer_listener.newtimer.it_value.tv_sec = secs;
		wrap->timer_listener.newtimer.it_value.tv_nsec = nsecs;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = secs;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = nsecs;
	} else
	if (timeout == 0) { // interrupt immediately
		wrap->timer_listener.newtimer.it_value.tv_sec = 0;
		wrap->timer_listener.newtimer.it_value.tv_nsec = 1;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = 0;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = 1;
	} else {
		wrap->timer_listener.newtimer.it_value.tv_sec = 0;
		wrap->timer_listener.newtimer.it_value.tv_nsec = 0;
		
		wrap->timer_listener.newtimer.it_interval.tv_sec = 0;
		wrap->timer_listener.newtimer.it_interval.tv_nsec = 0;
	}
	
	wrap->timer_listener.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	wrap->timer_listener.settime_flags = 0;
// 	wrap->timer_listener.newtimer = &newtimer;
	
	
// 		crtx_start_listener(blist);
	
	crtx_update_listener(&wrap->timer_listener.parent);
	
	printf("mod timeout %d\n", timeout);
	
// 	crtx_virEventAddTimeoutFunc(int timeout, virEventTimeoutCallback cb, void * opaque, virFreeCallback ff)
}

static int crtx_virEventRemoveTimeoutFunc(int timer) {
	struct crtx_ll *it;
	struct libvirt_wrapper *wrap;
	
	for (it=timeout_list; it; it=it->next) {
		if ( ((struct libvirt_wrapper*) it->data)->id == timer)
			break;
	}
	
	if (!it) {
		ERROR("libvirt: no timer with id %d found\n", timer);
		return -1;
	}
	
	wrap = ((struct libvirt_wrapper*) it->data);
	
// 	crtx_root->event_loop.del_fd(&crtx_root->event_loop.listener->parent, &wrap->el_payload);
	free_listener(&wrap->timer_listener.parent);
	
	crtx_ll_unlink(&timeout_list, it);
	
	free(it);
	free(wrap);
	
	printf("del timeout\n");
	
	return 0;
}

static const char *eventToString(int event) {
	const char *ret = "";
	switch ((virDomainEventType) event) {
		case VIR_DOMAIN_EVENT_DEFINED:
			ret ="Defined";
			break;
		case VIR_DOMAIN_EVENT_UNDEFINED:
			ret ="Undefined";
			break;
		case VIR_DOMAIN_EVENT_STARTED:
			ret ="Started";
			break;
		case VIR_DOMAIN_EVENT_SUSPENDED:
			ret ="Suspended";
			break;
		case VIR_DOMAIN_EVENT_RESUMED:
			ret ="Resumed";
			break;
		case VIR_DOMAIN_EVENT_STOPPED:
			ret ="Stopped";
			break;
		case VIR_DOMAIN_EVENT_SHUTDOWN:
			ret = "Shutdown";
			break;
		case VIR_DOMAIN_EVENT_PMSUSPENDED:
			ret = "PM suspended";
			break;
		case VIR_DOMAIN_EVENT_CRASHED:
			ret = "Crashed";
			break;
	}
	return ret;
}

static const char *eventDetailToString(int event, int detail) {
	const char *ret = "";
	switch ((virDomainEventType) event) {
		case VIR_DOMAIN_EVENT_DEFINED:
			if (detail == VIR_DOMAIN_EVENT_DEFINED_ADDED)
				ret = "Added";
			else if (detail == VIR_DOMAIN_EVENT_DEFINED_UPDATED)
				ret = "Updated";
			break;
		case VIR_DOMAIN_EVENT_UNDEFINED:
			if (detail == VIR_DOMAIN_EVENT_UNDEFINED_REMOVED)
				ret = "Removed";
			break;
		case VIR_DOMAIN_EVENT_STARTED:
			switch ((virDomainEventStartedDetailType) detail) {
			case VIR_DOMAIN_EVENT_STARTED_BOOTED:
				ret = "Booted";
				break;
			case VIR_DOMAIN_EVENT_STARTED_MIGRATED:
				ret = "Migrated";
				break;
			case VIR_DOMAIN_EVENT_STARTED_RESTORED:
				ret = "Restored";
				break;
			case VIR_DOMAIN_EVENT_STARTED_FROM_SNAPSHOT:
				ret = "Snapshot";
				break;
			case VIR_DOMAIN_EVENT_STARTED_WAKEUP:
				ret = "Event wakeup";
				break;
			}
			break;
		case VIR_DOMAIN_EVENT_SUSPENDED:
			switch ((virDomainEventSuspendedDetailType) detail) {
			case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
				ret = "Paused";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
				ret = "Migrated";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
				ret = "I/O Error";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
				ret = "Watchdog";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
				ret = "Restored";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT:
				ret = "Snapshot";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
				ret = "API error";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY:
				ret = "Suspended postcopy";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY_FAILED:
				ret = "Suspended postcopy failed";
				break;
			}
			break;
		case VIR_DOMAIN_EVENT_RESUMED:
			switch ((virDomainEventResumedDetailType) detail) {
			case VIR_DOMAIN_EVENT_RESUMED_UNPAUSED:
				ret = "Unpaused";
				break;
			case VIR_DOMAIN_EVENT_RESUMED_MIGRATED:
				ret = "Migrated";
				break;
			case VIR_DOMAIN_EVENT_RESUMED_FROM_SNAPSHOT:
				ret = "Snapshot";
				break;
			case VIR_DOMAIN_EVENT_RESUMED_POSTCOPY:
				ret = "Postcopy";
				break;
			}
			break;
		case VIR_DOMAIN_EVENT_STOPPED:
			switch ((virDomainEventStoppedDetailType) detail) {
			case VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN:
				ret = "Shutdown";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_DESTROYED:
				ret = "Destroyed";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_CRASHED:
				ret = "Crashed";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_MIGRATED:
				ret = "Migrated";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_SAVED:
				ret = "Saved";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_FAILED:
				ret = "Failed";
				break;
			case VIR_DOMAIN_EVENT_STOPPED_FROM_SNAPSHOT:
				ret = "Snapshot";
				break;
			}
			break;
		case VIR_DOMAIN_EVENT_SHUTDOWN:
			switch ((virDomainEventShutdownDetailType) detail) {
			case VIR_DOMAIN_EVENT_SHUTDOWN_FINISHED:
				ret = "Finished";
				break;
			}
			break;
		case VIR_DOMAIN_EVENT_PMSUSPENDED:
			switch ((virDomainEventPMSuspendedDetailType) detail) {
				case VIR_DOMAIN_EVENT_PMSUSPENDED_MEMORY:
					ret = "Memory";
					break;
				case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
					ret = "Disk";
					break;
// 				case VIR_DOMAIN_EVENT_PMSUSPENDED_LAST:
// 					ret = "Last";
// 					break;
			}
			break;
		case VIR_DOMAIN_EVENT_CRASHED:
			switch ((virDomainEventCrashedDetailType) detail) {
				case VIR_DOMAIN_EVENT_CRASHED_PANICKED:
					ret = "Panic";
					break;
// 				case VIR_DOMAIN_EVENT_CRASHED_LAST:
// 					ret = "Last";
// 					break;
			}
			break;
	}
	return ret;
}

static int myDomainEventCallback2(virConnectPtr conn,
								  virDomainPtr dom,
								  int event,
								  int detail,
								  void *opaque)
{
	printf("%s EVENT: Domain %s(%d) %s %s\n", __func__, virDomainGetName(dom),
		   virDomainGetID(dom), eventToString(event),
		   eventDetailToString(event, detail));
	return 0;
}

static void connectClose(virConnectPtr conn, int reason, void *opaque) {
	switch ((virConnectCloseReason) reason) {
		case VIR_CONNECT_CLOSE_REASON_ERROR:
			fprintf(stderr, "Connection closed due to I/O error\n");
			return;
			
		case VIR_CONNECT_CLOSE_REASON_EOF:
			fprintf(stderr, "Connection closed due to end of file\n");
			return;
			
		case VIR_CONNECT_CLOSE_REASON_KEEPALIVE:
			fprintf(stderr, "Connection closed due to keepalive timeout\n");
			return;
			
		case VIR_CONNECT_CLOSE_REASON_CLIENT:
			fprintf(stderr, "Connection closed due to client request\n");
			return;
			
// 		case VIR_CONNECT_CLOSE_REASON_LAST:
// 			break;
	};
	
	fprintf(stderr, "Connection closed due to unknown reason\n");
}

static void shutdown_listener(struct crtx_listener_base *base) {
	struct crtx_libvirt_listener *lvlist;
	
	lvlist = (struct crtx_libvirt_listener *) base;
	
	virConnectClose(lvlist->conn);
}

static char start_listener(struct crtx_listener_base *base) {
	struct crtx_libvirt_listener *lvlist;
	int ret;
	
	lvlist = (struct crtx_libvirt_listener *) base;
	
	printf("start %s\n", lvlist->hypervisor);
	
	lvlist->conn = virConnectOpen(lvlist->hypervisor);
	if (lvlist->conn == 0) {
		fprintf(stderr, "Failed to connect to hypervisor\n");
		return 0;
	}
	
	ret = virConnectRegisterCloseCallback(lvlist->conn, connectClose, NULL, NULL);
	if (ret < 0) {
		ERROR("virConnectRegisterCloseCallback failed: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		return 0;
	}
	
	ret = virConnectDomainEventRegisterAny(lvlist->conn,
												NULL,
												VIR_DOMAIN_EVENT_ID_LIFECYCLE,
												VIR_DOMAIN_EVENT_CALLBACK(myDomainEventCallback2),
												0, 0);
// 												strdup("callback 2"), 0);
	if (ret < 0) {
		ERROR("virConnectDomainEventRegisterAny failed: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		return 0;
	}
	
	ret = virConnectSetKeepAlive(lvlist->conn, 5, 3);
	if (ret < 0) {
		ERROR("Failed to start keepalive protocol: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		return 0;
	}
	
// 	while (1) {
// 		if (virEventRunDefaultImpl() < 0) {
// 			fprintf(stderr, "Failed to run event loop: %s\n",
// 					virGetLastErrorMessage());
// 		}
// 	}
	
	return 1;
}

struct crtx_listener_base *crtx_new_libvirt_listener(void *options) {
	struct crtx_libvirt_listener *lvlist;
	int ret;
	
	lvlist = (struct crtx_libvirt_listener *) options;
	
	lvlist->parent.start_listener = start_listener;
	lvlist->parent.shutdown = shutdown_listener;
	
	ret = virInitialize();
	if (ret < 0) {
		ERROR("failed to initialize libvirt");
		return 0;
	}
	
	virEventRegisterImpl(crtx_virEventAddHandleFunc,
						crtx_virEventUpdateHandleFunc,
						crtx_virEventRemoveHandleFunc,
						crtx_virEventAddTimeoutFunc,
						crtx_virEventUpdateTimeoutFunc,
						crtx_virEventRemoveTimeoutFunc);
	
// 	ret = virEventRegisterDefaultImpl();
// 	if (ret < 0) {
// 		ERROR("failed to initialize libvirt");
// 		return 0;
// 	}
	
	return &lvlist->parent;
}

void crtx_libvirt_init() {
}

void crtx_libvirt_finish() {
}


#ifdef CRTX_TEST

int libvirt_main(int argc, char **argv) {
	struct crtx_libvirt_listener lvlist;
	struct crtx_listener_base *lbase;
	char ret;
	
	
	memset(&lvlist, 0, sizeof(struct crtx_libvirt_listener));
	
	if (argc == 1)
		lvlist.hypervisor = "qemu:///system";
	else
		lvlist.hypervisor = argv[1];
	
	lbase = create_listener("libvirt", &lvlist);
	if (!lbase) {
		ERROR("create_listener(libvirt) failed\n");
		exit(1);
	}
	
// 	crtx_create_task(lbase->graph, 0, "udev_test", udev_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting libvirt listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(libvirt_main);

#endif
