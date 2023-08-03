/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <libvirt/virterror.h>

#include "intern.h"
#include "core.h"
#include "libvirt.h"
#include "timer.h"
#include "epoll.h"

// See https://github.com/libvirt/libvirt/blob/master/examples/object-events/event-test.c

// LIBVIRT_DEBUG=1 

static int elw_virEventRemoveHandleFunc(int watch);

struct libvirt_eventloop_wrapper {
	int id;
	
	struct crtx_evloop_fd evloop_fd;
	struct crtx_evloop_callback default_el_cb;
	
	int timeout;
	struct crtx_timer_listener timer_listener;
	
	virEventHandleCallback event_cb;
	virEventTimeoutCallback time_cb;
	
	void * opaque;
	
	virFreeCallback ff;
	
	char delete;
	struct crtx_ll *llentry;
};

static struct crtx_ll * event_list = 0;
static struct crtx_ll * timeout_list = 0;

/* poll conversion
static int elw_virEventPollToNativeEvents(int events) {
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

static int elw_virEventPollFromNativeEvents(int events) {
	int ret = 0;
	if (events & POLLIN)
		ret |= VIR_EVENT_HANDLE_READABLE;
	if (events & POLLOUT)
		ret |= VIR_EVENT_HANDLE_WRITABLE;
	if (events & POLLERR)
		ret |= VIR_EVENT_HANDLE_ERROR;
	if (events & POLLNVAL) // Treat NVAL as error, since libvirt doesn't distinguish
		ret |= VIR_EVENT_HANDLE_ERROR;
	if (events & POLLHUP)
		ret |= VIR_EVENT_HANDLE_HANGUP;
	return ret;
}
*/

// epoll conversion
static int elw_virEventPollToNativeEvents(int events) {
	int ret = 0;
	if (events & VIR_EVENT_HANDLE_READABLE)
		ret |= CRTX_EVLOOP_READ;
	if (events & VIR_EVENT_HANDLE_WRITABLE)
		ret |= CRTX_EVLOOP_WRITE;
	if (events & VIR_EVENT_HANDLE_ERROR)
		ret |= EPOLLERR;
	if (events & VIR_EVENT_HANDLE_HANGUP)
		ret |= EPOLLHUP;
	return ret;
}

static int elw_virEventPollFromNativeEvents(int events) {
	int ret = 0;
	if (events & CRTX_EVLOOP_READ)
		ret |= VIR_EVENT_HANDLE_READABLE;
	if (events & CRTX_EVLOOP_WRITE)
		ret |= VIR_EVENT_HANDLE_WRITABLE;
	if (events & EPOLLERR)
		ret |= VIR_EVENT_HANDLE_ERROR;
	if (events & EPOLLHUP)
		ret |= VIR_EVENT_HANDLE_HANGUP;
	if (events & EPOLLRDHUP)
		ret |= VIR_EVENT_HANDLE_HANGUP;
	return ret;
}

static void elw_fd_cleanup(struct libvirt_eventloop_wrapper* wrap) {
	if (wrap->ff)
		wrap->ff(wrap->opaque);
	
// 	crtx_root->event_loop.del_fd(&crtx_root->event_loop.listener->base, &wrap->evloop_fd);
// 	crtx_root->event_loop.del_fd(&crtx_root->event_loop, &wrap->evloop_fd);
	crtx_evloop_remove_cb(wrap->evloop_fd.callbacks);
	
	crtx_ll_unlink(&event_list, wrap->llentry);
	
	free(wrap->llentry);
	free(wrap);
}

// static void elw_fd_callback(struct crtx_evloop_fd *evloop_fd) {
static char elw_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct libvirt_eventloop_wrapper *wrap;
// 	struct epoll_event *epoll_event;
	
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
// 	epoll_event = (struct epoll_event *) el_cb->fd_entry->el_data;
	
	wrap = (struct libvirt_eventloop_wrapper*) userdata;
	
// 	printf("lv fd cb %d %p %d\n", wrap->id, wrap->event_cb, epoll_event->events);
// 	if (epoll_event->events & EPOLLHUP) {
// 		CRTX_DBG("libvirt received EPOLLHUP for %d, closing...\n", wrap->id);
// 		elw_virEventRemoveHandleFunc(wrap->id);
// 		
// 		return;
// 	}
	
	wrap->event_cb(wrap->id, wrap->evloop_fd.fd, elw_virEventPollFromNativeEvents(el_cb->triggered_flags), wrap->opaque);
	
	if (wrap->delete)
		elw_fd_cleanup(wrap);
	
	return 0;
}

static void elw_fd_error_cb(struct crtx_evloop_callback *el_cb, void *data) {
	struct libvirt_eventloop_wrapper *wrap;
// 	struct epoll_event *epoll_event;
	
	
	wrap = (struct libvirt_eventloop_wrapper*) el_cb->event_handler_data;
// 	epoll_event = (struct epoll_event *) evloop_fd->el_data;
	
	CRTX_DBG("libvirt event loop error callback for fd %d\n", el_cb->fd_entry->fd);
	
	if (wrap->delete)
		elw_fd_cleanup(wrap);
	
	wrap->event_cb(wrap->id, wrap->evloop_fd.fd, elw_virEventPollFromNativeEvents(el_cb->triggered_flags), wrap->opaque);
	
	wrap->delete = 1;
}

static int elw_virEventAddHandleFunc(int fd, int event, virEventHandleCallback cb, void * opaque, virFreeCallback ff) {
	struct libvirt_eventloop_wrapper *wrap;
	struct crtx_ll *it;
	
	wrap = (struct libvirt_eventloop_wrapper*) calloc(1, sizeof(struct libvirt_eventloop_wrapper));
	
// 	wrap->evloop_fd.fd = fd;
// 	wrap->evloop_fd.crtx_event_flags = elw_virEventPollToNativeEvents(event);
// 	wrap->evloop_fd.simple_callback = &elw_fd_callback;
// 	wrap->evloop_fd.error_cb = &elw_fd_error_cb;
// 	wrap->evloop_fd.data = wrap;
	crtx_evloop_create_fd_entry(&wrap->evloop_fd, &wrap->default_el_cb,
						fd,
						elw_virEventPollToNativeEvents(event),
						0,
						&elw_fd_event_handler,
						wrap,
						&elw_fd_error_cb, 0
					);
	
	wrap->event_cb = cb;
	wrap->opaque = opaque;
	wrap->ff = ff;
	
	wrap->id = 0;
	if (event_list) {
		for (it=event_list;it->next;it=it->next) {
			if (wrap->id < ((struct libvirt_eventloop_wrapper*) it->data)->id)
				break;
			if (wrap->id == ((struct libvirt_eventloop_wrapper*) it->data)->id)
				wrap->id++;
		}
		
		if (wrap->id == ((struct libvirt_eventloop_wrapper*) it->data)->id)
			wrap->id++;
	}
	
	CRTX_DBG("libvirt: new fd %d (id %d flag %d)\n", fd, wrap->id, wrap->default_el_cb.crtx_event_flags);
	
	wrap->llentry = crtx_ll_append_new(&event_list, wrap);
	
// 	crtx_root->event_loop.add_fd(&crtx_root->event_loop, &wrap->evloop_fd);
	crtx_evloop_enable_cb(&wrap->default_el_cb);
	
	return wrap->id;
}

static void elw_virEventUpdateHandleFunc(int watch, int event) {
	struct crtx_ll *it;
	struct libvirt_eventloop_wrapper *wrap;
	int flags;
	
	
	for (it=event_list; it; it=it->next) {
		if ( ((struct libvirt_eventloop_wrapper*) it->data)->id == watch)
			break;
	}
	
	if (!it) {
		CRTX_ERROR("libvirt: no watch with id %d found\n", watch);
		return;
	}
	
	wrap = ((struct libvirt_eventloop_wrapper*) it->data);
	flags = elw_virEventPollToNativeEvents(event);
	
	CRTX_DBG("libvirt: update id %d flags %d (prev: %d)\n", wrap->id, flags, wrap->default_el_cb.crtx_event_flags);
	
	if (flags != wrap->default_el_cb.crtx_event_flags) {
		wrap->default_el_cb.crtx_event_flags = flags;
	// 	crtx_root->event_loop.mod_fd(&crtx_root->event_loop, &wrap->evloop_fd);
		crtx_evloop_enable_cb(&wrap->default_el_cb);
	}
}

static int elw_virEventRemoveHandleFunc(int watch) {
	struct crtx_ll *it;
	struct libvirt_eventloop_wrapper *wrap;
	
	for (it=event_list; it; it=it->next) {
		if ( ((struct libvirt_eventloop_wrapper*) it->data)->id == watch)
			break;
	}
	
	if (!it) {
		CRTX_ERROR("libvirt: no watch with id %d found\n", watch);
		return -1;
	}
	
	wrap = ((struct libvirt_eventloop_wrapper*) it->data);
	
	// this deadlocks. crtx calls an fd handler in libvirt which calls this function, maybe both reference the same
	// libvirt object
// 	wrap->ff(wrap->opaque);
	wrap->delete = 1;
	
	// 	printf("rem id %d\n", wrap->id);
	CRTX_DBG("libvirt: remove fd %d (id %d)\n", wrap->evloop_fd.fd, wrap->id);
	
// 	crtx_root->event_loop.del_fd(&crtx_root->event_loop.listener->base, &wrap->evloop_fd);
	
// 	crtx_ll_unlink(&event_list, it);
// 	
// 	free(it);
// 	free(wrap);
	
	
	
	return 0;
}


static void elw_timer_cleanup(struct libvirt_eventloop_wrapper *wrap) {
	if (wrap->ff)
		wrap->ff(wrap->opaque);
	
// 	crtx_shutdown_listener(&wrap->timer_listener.base);
	
	crtx_ll_unlink(&timeout_list, wrap->llentry);
	free(wrap->llentry);
	
	free(wrap);
}

static char elw_timer_event_cb(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct libvirt_eventloop_wrapper *wrap;
	
	wrap = (struct libvirt_eventloop_wrapper*) userdata;
// 	printf("timer event id %d (%p)\n", wrap->id, wrap->opaque);
	wrap->time_cb(wrap->id, wrap->opaque);
	
	if (wrap->delete)
		elw_timer_cleanup(wrap);
	
	return 1;
}

// void elw_free_timer_lstnr(struct crtx_listener_base *base, void *userdata) {
// 	free(userdata);
// }

static int elw_virEventAddTimeoutFunc(int timeout, virEventTimeoutCallback cb, void * opaque, virFreeCallback ff) {
	struct libvirt_eventloop_wrapper *wrap;
	struct crtx_ll *it;
	int r;
	
	
	wrap = (struct libvirt_eventloop_wrapper*) calloc(1, sizeof(struct libvirt_eventloop_wrapper));
	
	wrap->timeout = timeout;
	wrap->time_cb = cb;
	wrap->opaque = opaque;
	wrap->ff = ff;
	
	wrap->id = 0;
	if (timeout_list) {
		for (it=timeout_list;it->next;it=it->next) {
			if (wrap->id < ((struct libvirt_eventloop_wrapper*) it->data)->id)
				break;
			if (wrap->id == ((struct libvirt_eventloop_wrapper*) it->data)->id)
				wrap->id++;
		}
		
		if (wrap->id == ((struct libvirt_eventloop_wrapper*) it->data)->id)
			wrap->id++;
	}
	
	CRTX_DBG("libvirt: new timer %d (id %d)\n", timeout, wrap->id);
	
	wrap->llentry = crtx_ll_append_new(&timeout_list, wrap);
	
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
	
// 	wrap->timer_listener.base.free = &elw_free_timer_lstnr;
// 	wrap->timer_listener.base.free_userdata = wrap;
	
	r = crtx_setup_listener("timer", &wrap->timer_listener);
	if (r) {
		CRTX_ERROR("crtx_setup_listener(timer) failed: %s\n", strerror(-r));
		return -1;
	}
	
	crtx_create_task(wrap->timer_listener.base.graph, 0, "libvirt_timer", elw_timer_event_cb, wrap);
	
	crtx_start_listener(&wrap->timer_listener.base);
	
	return wrap->id;
}

static void elw_virEventUpdateTimeoutFunc(int timer, int timeout) {
	struct crtx_ll *it;
	struct libvirt_eventloop_wrapper *wrap;
// 	struct itimerspec newtimer;
	
	for (it=timeout_list; it; it=it->next) {
		if ( ((struct libvirt_eventloop_wrapper*) it->data)->id == timer)
			break;
	}
// 	printf("update timer %d %d\n", timer, timeout);
	if (!it) {
		CRTX_ERROR("libvirt: no timer with id %d found\n", timer);
		return;
	}
	
	wrap = ((struct libvirt_eventloop_wrapper*) it->data);
	
	
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
	
	crtx_update_listener(&wrap->timer_listener.base);
	
// 	CRTX_DBG("libvirt: mod timeout %d\n", timeout);
}

static int elw_virEventRemoveTimeoutFunc(int timer) {
	struct crtx_ll *it;
	struct libvirt_eventloop_wrapper *wrap;
	
	for (it=timeout_list; it; it=it->next) {
		if ( ((struct libvirt_eventloop_wrapper*) it->data)->id == timer)
			break;
	}
	
	if (!it) {
		CRTX_ERROR("libvirt: no timer with id %d found\n", timer);
		return -1;
	}
	
	wrap = ((struct libvirt_eventloop_wrapper*) it->data);
	
	CRTX_DBG("libvirt: remove timer id %d\n", wrap->id);
	
// 	wrap->ff(wrap->opaque);
	wrap->delete = 1;
	
// 	crtx_root->event_loop.del_fd(&crtx_root->event_loop.listener->base, &wrap->evloop_fd);

	crtx_shutdown_listener(&wrap->timer_listener.base);
	
// 	crtx_ll_unlink(&timeout_list, it);
// 	free(it);
	
// 	CRTX_DBG("libvirt: del timeout\n");
	
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
#if LIBVIR_CHECK_VERSION(2,5,0)
			case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY:
				ret = "Suspended postcopy";
				break;
			case VIR_DOMAIN_EVENT_SUSPENDED_POSTCOPY_FAILED:
				ret = "Suspended postcopy failed";
				break;
#endif
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
#if LIBVIR_CHECK_VERSION(2,5,0)
			case VIR_DOMAIN_EVENT_RESUMED_POSTCOPY:
				ret = "Postcopy";
				break;
#endif
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
#if LIBVIR_CHECK_VERSION(3,2,0)
			case VIR_DOMAIN_EVENT_SHUTDOWN_GUEST:
				ret = "guest";
				break;
			case VIR_DOMAIN_EVENT_SHUTDOWN_HOST:
				ret = "host";
				break;
#endif
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
#if LIBVIR_CHECK_VERSION(6,1,0)
				case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
					ret = "Disk";
					break;
#endif
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
#if LIBVIR_CHECK_VERSION(6,1,0)
				case VIR_DOMAIN_EVENT_CRASHED_CRASHLOADED:
					ret = "Crashloaded";
					break;
#endif
// 				case VIR_DOMAIN_EVENT_CRASHED_LAST:
// 					ret = "Last";
// 					break;
			}
			break;
	}
	return ret;
}

static int domain_evt_cb(virConnectPtr conn,
								  virDomainPtr dom,
								  int event,
								  int detail,
								  void *opaque)
{
// 	printf("%s EVENT: Domain %s(%d) %s %s\n", __func__, virDomainGetName(dom),
// 		   virDomainGetID(dom), eventToString(event),
// 		   eventDetailToString(event, detail));
	
	struct crtx_event *crtx_event;
	struct crtx_libvirt_listener *lvlist;
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	size_t slen;
	const char *vm_name;
	int r;
	
	
	lvlist = (struct crtx_libvirt_listener *) opaque;
	
	r = crtx_create_event(&crtx_event); //, 0, 0);
	if (r) {
		CRTX_ERROR("crtx_create_event() failed: %s\n", strerror(-r));
		return r;
	}
	
	crtx_event->description = "domain_event";
	
	dict = crtx_init_dict(0, 0, 0);
	
	vm_name = virDomainGetName(dom);
	
	if (vm_name) {
		slen = 0;
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "name", crtx_stracpy(vm_name, &slen), slen, 0);
	}
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "id", virDomainGetID(dom), sizeof(unsigned int), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "type", eventToString(event), strlen(eventToString(event)), CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "detail", eventDetailToString(event, detail), strlen(eventDetailToString(event, detail)), CRTX_DIF_DONT_FREE_DATA);
	
// 	crtx_event->data.type = 'D';
// 	crtx_event->data.dict = dict;
	crtx_event_set_dict_data(crtx_event, dict, 0);
	
	crtx_print_dict(dict);
	
	crtx_add_event(lvlist->base.graph, crtx_event);
	
	return 0;
}

static void conn_evt_cb(virConnectPtr conn, int reason, void *opaque) {
	struct crtx_libvirt_listener *lvlist;
// 	int ret;
	char *sreason;
// 	struct crtx_event *event;
// 	struct crtx_dict *dict;
// 	struct crtx_dict_item *di;
	
	
	lvlist = (struct crtx_libvirt_listener *) opaque;
	
	switch ((virConnectCloseReason) reason) {
		case VIR_CONNECT_CLOSE_REASON_ERROR:
// 			fprintf(stderr, "Connection closed due to I/O error\n");
// 			return;
			sreason = "error";
			break;
		
		case VIR_CONNECT_CLOSE_REASON_EOF:
// 			fprintf(stderr, "Connection closed due to end of file\n");
// 			return;
			sreason = "eof";
			break;
		
		case VIR_CONNECT_CLOSE_REASON_KEEPALIVE:
// 			fprintf(stderr, "Connection closed due to keepalive timeout\n");
// 			return;
			sreason = "timeout";
			break;
			
		case VIR_CONNECT_CLOSE_REASON_CLIENT:
// 			fprintf(stderr, "Connection closed due to client request\n");
// 			return;
			sreason = "client";
			break;
			
// 		case VIR_CONNECT_CLOSE_REASON_LAST:
// 			break;
	};
	
	CRTX_DBG("libvirt connection closed: %s\n", sreason);
	
	lvlist = (struct crtx_libvirt_listener *) opaque;
	
	// we close the connection here as unregistering the callback in stop_listener will deadlock.
	// although we close the connection, we still leak memory allocated by virConnectOpen->virNetSocketNewConnectUNIX->...
	virConnectClose(lvlist->conn);
	
	lvlist->conn = 0;
	
	// signal crtx that we stopped
	crtx_stop_listener(&lvlist->base);
}

static char stop_listener(struct crtx_listener_base *base) {
	struct crtx_libvirt_listener *lvlist;
	
	lvlist = (struct crtx_libvirt_listener *) base;
	
	if (lvlist->conn) {
		virConnectDomainEventDeregisterAny(lvlist->conn, lvlist->domain_event_id);
		
		virConnectUnregisterCloseCallback(lvlist->conn, conn_evt_cb);
		
		virConnectClose(lvlist->conn);
		
		lvlist->conn = 0;
	}
	
	return 0;
}

virConnectPtr crtx_libvirt_get_conn(struct crtx_listener_base *base) {
	struct crtx_libvirt_listener *lvlist;
	
	lvlist = (struct crtx_libvirt_listener *) base;
	
	return lvlist->conn;
}

static char start_listener(struct crtx_listener_base *base) {
	struct crtx_libvirt_listener *lvlist;
	int ret;
	
	lvlist = (struct crtx_libvirt_listener *) base;
	
	lvlist->conn = virConnectOpen(lvlist->hypervisor);
	if (lvlist->conn == 0) {
		CRTX_ERROR("Failed to connect to hypervisor\n");
		return -1;
	}
	
	ret = virConnectRegisterCloseCallback(lvlist->conn, conn_evt_cb, lvlist, NULL);
	if (ret < 0) {
		CRTX_ERROR("virConnectRegisterCloseCallback failed: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		lvlist->conn = 0;
		return ret;
	}
	
	ret = virConnectDomainEventRegisterAny(lvlist->conn,
												NULL,
												VIR_DOMAIN_EVENT_ID_LIFECYCLE,
												VIR_DOMAIN_EVENT_CALLBACK(domain_evt_cb),
												lvlist, 0);
	if (ret < 0) {
		CRTX_ERROR("virConnectDomainEventRegisterAny failed: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		lvlist->conn = 0;
		return ret;
	} else {
		lvlist->domain_event_id = ret;
	}
	
	ret = virConnectSetKeepAlive(lvlist->conn, 5, 3);
	if (ret < 0) {
		CRTX_ERROR("Failed to start keepalive protocol: %s\n", virGetLastErrorMessage());
		virConnectClose(lvlist->conn);
		lvlist->conn = 0;
		return ret;
	}
	
// 	while (1) {
// 		if (virEventRunDefaultImpl() < 0) {
// 			fprintf(stderr, "Failed to run event loop: %s\n",
// 					virGetLastErrorMessage());
// 		}
// 	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_libvirt_listener(void *options) {
	struct crtx_libvirt_listener *lvlist;
	int ret;
	
	lvlist = (struct crtx_libvirt_listener *) options;
	
	lvlist->base.start_listener = start_listener;
	lvlist->base.stop_listener = stop_listener;
	lvlist->base.mode = CRTX_NO_PROCESSING_MODE;
	
	ret = virInitialize();
	if (ret < 0) {
		CRTX_ERROR("failed to initialize libvirt");
		return 0;
	}
	
	/*
	 * libvirt will use these callbacks to register a file handle
	 */
	virEventRegisterImpl(elw_virEventAddHandleFunc,
						elw_virEventUpdateHandleFunc,
						elw_virEventRemoveHandleFunc,
						elw_virEventAddTimeoutFunc,
						elw_virEventUpdateTimeoutFunc,
						elw_virEventRemoveTimeoutFunc);
	
// 	ret = virEventRegisterDefaultImpl();
// 	if (ret < 0) {
// 		CRTX_ERROR("failed to initialize libvirt");
// 		return 0;
// 	}
	
// 	new_eventgraph(&lvlist->base.graph, "libvirt", 0);
	
	return &lvlist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(libvirt)

void crtx_libvirt_init() {
}

void crtx_libvirt_finish() {
}


#ifdef CRTX_TEST

struct crtx_libvirt_listener lvlist;
struct crtx_timer_retry_listener *retry_lstnr;
// struct crtx_timer_listener tlist;
// struct crtx_listener_base *tblist;

static char libvirt_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	printf("virt %d timer %d\n", lvlist.base.state, tlist.base.state);
	if (!strcmp(event->description, "listener_state")) {
// 		printf("ev %d\n", event->data.uint32);
// 		if (event->data.uint32 == CRTX_LSTNR_STARTED && tlist.base.state == CRTX_LSTNR_STARTED) {
		if (event->data.uint32 == CRTX_LSTNR_STARTED) {
// 			printf("stop timer\n");
			crtx_stop_listener(&retry_lstnr->timer_lstnr.base);
		}
// 		if (event->data.uint32 == CRTX_LSTNR_STOPPED && tlist.base.state == CRTX_LSTNR_STOPPED) {
		if (event->data.uint32 == CRTX_LSTNR_STOPPED) {
// 			printf("start timer\n");
			crtx_start_listener(&retry_lstnr->timer_lstnr.base);
		}
	} else
	if (!strcmp(event->description, "domain_event")) {
		crtx_print_dict(event->data.dict);
	}
	
	return 0;
}

// static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	if (!strcmp(event->type, CRTX_EVT_TIMER)) {
// // 		printf("state %d\n", lvlist.base.state);
// 		if (lvlist.base.state == CRTX_LSTNR_STOPPED) {
// // 			printf("start libvirt\n");
// 			crtx_start_listener(&lvlist.base);
// 		}
// 	}
// 	
// 	return 0;
// }

int libvirt_main(int argc, char **argv) {
	char ret;
	
	
	memset(&lvlist, 0, sizeof(struct crtx_libvirt_listener));
	
	if (argc == 1)
		lvlist.hypervisor = "qemu:///system";
	else
		lvlist.hypervisor = argv[1];
	
	ret = crtx_setup_listener("libvirt", &lvlist);
	if (ret) {
		CRTX_ERROR("create_listener(libvirt) failed\n");
		exit(1);
	}
	
	lvlist.base.state_graph = lvlist.base.graph;
	
	// 	new_eventgraph(&lvlist->base.graph, "libvirt", 0);
	
	crtx_create_task(lvlist.base.graph, 0, "libvirt_test", libvirt_test_handler, 0);
	
	
	
// 	memset(&tlist, 0, sizeof(struct crtx_timer_listener));
// 	
// 	tlist.newtimer.it_value.tv_sec = 1;
// 	tlist.newtimer.it_value.tv_nsec = 0;
// 	tlist.newtimer.it_interval.tv_sec = 1;
// 	tlist.newtimer.it_interval.tv_nsec = 0;
// 	tlist.clockid = CLOCK_REALTIME;
// 	
// 	tblist = create_listener("timer", &tlist);
// 	if (!tblist) {
// 		CRTX_ERROR("create_listener(timer) failed\n");
// 		exit(1);
// 	}
// 	
// 	crtx_create_task(tblist->graph, 0, "timertest", timertest_handler, 0);
	
// 	crtx_start_listener(tblist);
	
	retry_lstnr = crtx_timer_retry_listener(&lvlist.base, 5);
	
	ret = crtx_start_listener(&lvlist.base);
	if (ret) {
		CRTX_ERROR("starting libvirt listener failed\n");
// 		return 1;
	}
	
	crtx_loop();
	
// 	free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(libvirt_main);

#endif
