/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef DEBUG
#include <sys/ioctl.h>
#endif

#include "intern.h"
#include "core.h"
#include "epoll.h"
#include "evloop.h"


static int crtx_epoll_add_fd_intern(struct crtx_epoll_listener *epl, int fd, struct epoll_event *event) {
	int ret;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl add failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

static int crtx_epoll_mod_fd_intern(struct crtx_epoll_listener *epl, int fd, struct epoll_event *event) {
	int ret;
	
	if (epl->stop)
		return 0;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_MOD, fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl mod failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

static int crtx_epoll_del_fd_intern(struct crtx_epoll_listener *epl, int fd) {
	int ret;
	
	if (epl->stop)
		return 0;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_DEL, fd, 0);
	if (ret < 0) {
		ERROR("epoll_ctl del failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

static int crtx_event_flags2epoll_flags(int crtx_event_flags) {
	int ret;
	
	
	ret = 0;
	if (crtx_event_flags & EVLOOP_READ)
		ret |= EPOLLIN;
	if (crtx_event_flags & EVLOOP_WRITE)
		ret |= EPOLLOUT;
	if (crtx_event_flags & EVLOOP_SPECIAL)
		ret |= EPOLLPRI;
	if (crtx_event_flags & EVLOOP_EDGE_TRIGGERED)
		ret |= EPOLLET;
	
	return ret;
}

static int epoll_flags2crtx_event_flags(int epoll_flags) {
	int ret;
	
	
	ret = 0;
	if (epoll_flags & EPOLLIN)
		ret |= EVLOOP_READ;
	if (epoll_flags & EPOLLOUT)
		ret |= EVLOOP_WRITE;
	if (epoll_flags & EPOLLPRI)
		ret |= EVLOOP_SPECIAL;
	if (epoll_flags & EPOLLET)
		ret |= EVLOOP_EDGE_TRIGGERED;
	
	return ret;
}

#if 1
static char *epoll_flags2str(unsigned int *flags) {
// #define RETFLAG(flag) if (*flags & flag) { *flags = (*flags) & (~flag); return #flag; }
#define RETFLAG(flag) if (*flags & flag) VDBG(#flag " ");
	
RETFLAG(EPOLLIN);
RETFLAG(EPOLLOUT);
RETFLAG(EPOLLRDHUP);
RETFLAG(EPOLLPRI);
RETFLAG(EPOLLERR);
RETFLAG(EPOLLHUP);
RETFLAG(EPOLLET);

// VDBG(" %u\n", *flags);
return "";
}
#endif


static int crtx_epoll_manage_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
	struct epoll_event *epoll_event;
	struct crtx_epoll_listener *epl;
	struct crtx_evloop_callback *el_cb;
	int ret;
	
	
	epl = (struct crtx_epoll_listener *) evloop->listener;
	
	if (!evloop_fd->el_data) {
		epoll_event = (struct epoll_event*) calloc(1, sizeof(struct epoll_event));
		evloop_fd->el_data = epoll_event;
		
		epoll_event->data.ptr = evloop_fd;
	} else {
		epoll_event = evloop_fd->el_data;
	}
	
	epoll_event->events = 0;
	for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
		if (!el_cb->active)
			continue;
		
		epoll_event->events |= crtx_event_flags2epoll_flags(el_cb->crtx_event_flags);
	}
	
	if (epoll_event->events) {
		if (evloop_fd->fd_added) {
			VDBG("epoll mod %d %d\n", evloop_fd->fd, epoll_flags2str(&epoll_event->events));
			
			crtx_epoll_mod_fd_intern(epl, evloop_fd->fd, epoll_event);
		} else {
			VDBG("epoll add %d %d\n", evloop_fd->fd, epoll_flags2str(&epoll_event->events));
			
			ret = crtx_epoll_add_fd_intern(epl, evloop_fd->fd, epoll_event);
			
			evloop_fd->fd_added = (ret == 0);
			
			crtx_ll_append((struct crtx_ll**) &evloop->fds, &evloop_fd->ll);
		}
	} else {
		VDBG("epoll del %d\n", evloop_fd->fd);
		
		crtx_ll_unlink((struct crtx_ll**) &evloop->fds, (struct crtx_ll*) &evloop_fd->ll);
		
		crtx_epoll_del_fd_intern(epl, evloop_fd->fd);
		
		free(evloop_fd->el_data);
		evloop_fd->el_data = 0;
	}
	
	return 0;
}

int crtx_epoll_mod_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
	int ret;
	
	ret = crtx_epoll_manage_fd(evloop, evloop_fd);
	
	return ret;
}

static int evloop_start(struct crtx_event_loop *evloop) {
	struct crtx_epoll_listener *epl;
	size_t i;
	int n_rdy_events;
	struct epoll_event *rec_events;
	struct crtx_evloop_fd *evloop_fd;
	struct crtx_evloop_callback *el_cb;
	struct timespec now_ts;
	uint64_t now;
	uint64_t timeout;
	int epoll_timeout_ms;
	char timeout_set;
	
	
	epl = (struct crtx_epoll_listener*) evloop->listener;
	
	rec_events = (struct epoll_event*) malloc(sizeof(struct epoll_event)*epl->max_n_events);
	
	while (!epl->stop) {
		clock_gettime(CRTX_DEFAULT_CLOCK, &now_ts);
		now = CRTX_timespec2uint64(&now_ts);
		
		if (epl->timeout >= 0)
			timeout = epl->timeout;
		else
			timeout = UINT64_MAX;
		
		timeout_set = 0;
		for (evloop_fd = evloop->fds; evloop_fd; evloop_fd = (struct crtx_evloop_fd *) evloop_fd->ll.next) {
			if (timeout == 0)
				break;
			
			for (el_cb = evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
				if (el_cb->timeout.tv_sec > 0 || el_cb->timeout.tv_nsec > 0) {
					if (CRTX_timespec2uint64(&el_cb->timeout) <= now) {
						timeout_set = 1;
						timeout = 0;
						break;
					}
					
					if (CRTX_timespec2uint64(&el_cb->timeout) - now < timeout) {
						timeout_set = 1;
						timeout = CRTX_timespec2uint64(&el_cb->timeout) - now;
					}
				}
			}
		}
		
		if (timeout_set) {
			if (timeout > INT32_MAX) {
				epoll_timeout_ms = INT32_MAX;
			} else {
				epoll_timeout_ms = timeout / 1000000;
			}
		} else {
			epoll_timeout_ms = epl->timeout;
		}
		
		VDBG("epoll waiting (TO: %d)...\n", epoll_timeout_ms);
		n_rdy_events = epoll_wait(epl->epoll_fd, rec_events, epl->max_n_events, epoll_timeout_ms);
		if (n_rdy_events < 0 && errno == EINTR)
			continue;
		
		if (n_rdy_events < 0) {
			ERROR("epoll_wait failed: %s\n", strerror(errno));
			break;
		}
		
		VDBG("epoll returned with %d events\n", n_rdy_events);
		
		for (i=0; i < n_rdy_events; i++) {
			evloop_fd = (struct crtx_evloop_fd* ) rec_events[i].data.ptr;
			
			#ifdef DEBUG
			VDBG("epoll #%d %d ", i, evloop_fd->fd);
			#define PRINTFLAG(flag) if (rec_events[i].events & flag) VDBG(#flag " ");
			
			PRINTFLAG(EPOLLIN);
			PRINTFLAG(EPOLLOUT);
			PRINTFLAG(EPOLLRDHUP);
			PRINTFLAG(EPOLLPRI);
			PRINTFLAG(EPOLLERR);
			PRINTFLAG(EPOLLHUP);
			VDBG("\n");
			
			int bytesAvailable;
			ioctl(evloop_fd->fd, FIONREAD, &bytesAvailable);
			VDBG("bytes %u\n", bytesAvailable);
			#endif
			
			if (rec_events[i].events & EPOLLERR || rec_events[i].events & EPOLLRDHUP || rec_events[i].events & EPOLLHUP) {
				ERROR("epoll returned EPOLLERR for fd %d\n", evloop_fd->fd);
				
				for (el_cb = evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
					if (el_cb->error_cb)
						el_cb->error_cb(el_cb, el_cb->error_cb_data);
					else
						DBG("no error_cb for fd %d\n", el_cb->fd_entry->fd);
				}
				
				continue;
			} else {
				for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
					if (!el_cb->active)
						continue;
					
					el_cb->triggered_flags = epoll_flags2crtx_event_flags(rec_events[i].events);
					
					crtx_evloop_callback(el_cb);
				}
			}
		}
		
		
		clock_gettime(CRTX_DEFAULT_CLOCK, &now_ts);
		now = CRTX_timespec2uint64(&now_ts);
		
		for (evloop_fd = evloop->fds; evloop_fd; evloop_fd = (struct crtx_evloop_fd *) evloop_fd->ll.next) {
			for (el_cb = evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
				if (el_cb->timeout.tv_sec > 0 || el_cb->timeout.tv_nsec > 0) {
					if (CRTX_timespec2uint64(&el_cb->timeout) < now) {
						printf("epoll timeout #%d %d %u\n", i, evloop_fd->fd, CRTX_timespec2uint64(&el_cb->timeout));
						
						if (!el_cb->active)
							continue;
						
						el_cb->triggered_flags = EVLOOP_TIMEOUT;
						
						crtx_evloop_callback(el_cb);
					}
				}
			}
		}
	}
	
	DBG("epoll stops\n");
	
	free(rec_events);
	
	return 0;
}

void *crtx_epoll_main(void *data) {
	evloop_start( (struct crtx_event_loop*) data );
	
	return 0;
}

static int evloop_stop(struct crtx_event_loop *evloop) {
	struct crtx_event_loop_control_pipe ecp;
	struct crtx_epoll_listener *epl;
	ssize_t ret;
	
// 	printf("evloop stop %d %d\n", evloop->ctrl_pipe[0], evloop->ctrl_pipe[1]);
	
	epl = (struct crtx_epoll_listener *) evloop->listener;
	epl->stop = 1;
	
	ecp.graph = 0;
	
	ret = write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	if (ret < sizeof(struct crtx_event_loop_control_pipe)) {
		ERROR("evloop_stop() write returned %zd %s\n", ret, strerror(errno));
	}
	
	return 0;
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_event_loop *evloop;
	
	evloop = (struct crtx_event_loop *) data;
	
	evloop_stop(evloop);
}

static void shutdown_epoll_listener(struct crtx_listener_base *lbase) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) lbase;
	
	crtx_evloop_release(epl->evloop);
	
	stop_thread(0, epl->evloop);
	
// 	printf("close epoll fd\n");
	
	close(epl->epoll_fd);
}

struct crtx_listener_base *crtx_new_epoll_listener(void *options) {
	struct crtx_epoll_listener *epl;
	
	
	epl = (struct crtx_epoll_listener*) options;
	
	epl->epoll_fd = epoll_create1(0);
	if (epl->epoll_fd < 0) {
		ERROR("epoll_create1 failed: %s\n", strerror(errno));
		return 0;
	}
	
	epl->base.shutdown = &shutdown_epoll_listener;
	
	epl->base.thread_job.fct = &crtx_epoll_main;
	epl->base.thread_job.fct_data = epl->evloop;
	epl->base.thread_job.do_stop = &stop_thread;
	
	// either the main thread will execute crtx_epoll_main or we spawn a new thread
	epl->base.mode = CRTX_PREFER_THREAD;
	
	
	if (epl->max_n_events < 1)
		epl->max_n_events = 1;
	
	// 0 is our default, -1 is no timeout for epoll
	epl->timeout -= 1;
	
	return &epl->base;
}

static int evloop_create(struct crtx_event_loop *evloop) {
	int ret;
	struct crtx_epoll_listener *epl;
	
	if (!evloop->listener)
		evloop->listener = (struct crtx_listener_base*) calloc(1, sizeof(struct crtx_epoll_listener));
	epl = (struct crtx_epoll_listener *) evloop->listener;
	
	epl->evloop = evloop;
	
	ret = crtx_create_listener("epoll", evloop->listener);
	if (ret) {
		ERROR("create_listener(epoll) failed\n");
	}
	
	return ret;
}

static int evloop_release(struct crtx_event_loop *evloop) {
	crtx_free_listener(evloop->listener);
	
	return 0;
}

struct crtx_event_loop epoll_loop = {
	.ll = { 0 },
	.id = "epoll",
	.ctrl_pipe = { 0 },
	.ctrl_pipe_evloop_handler = { {0} },
	.default_el_cb = { {0} },
	.listener = 0,
	.fds = 0,
	
	.create = &evloop_create,
	.release = &evloop_release,
	.start = &evloop_start,
	.stop = &evloop_stop,
	
	.mod_fd = &crtx_epoll_mod_fd,
};

void crtx_epoll_init() {
	epoll_loop.ll.data = &epoll_loop;
	crtx_ll_append((struct crtx_ll**) &crtx_event_loops, &epoll_loop.ll);
}

void crtx_epoll_finish() {
}


#ifdef CRTX_TEST
#include <fcntl.h>
#include "timer.h"

struct crtx_timer_listener tlist;
struct itimerspec newtimer;
struct crtx_listener_base *blist;
int testpipe[2];
int count = 0;

static char epoll_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char buf[1024];
	ssize_t n_read;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	n_read = read(el_cb->fd_entry->fd, buf, sizeof(buf)-1);
	if (n_read < 0) {
		ERROR("epoll test read failed: %s\n", strerror(errno));
		return 1;
	}
	buf[n_read] = 0;
	
	printf("read: \"%s\" (%zd)\n", buf, n_read);
	
	return 1;
}

static char timer_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("timerhandler\n");
	
	if (count > 2) {
		crtx_init_shutdown();
	} else {
		write(testpipe[1], "success", 7);
		count++;
	}
	
	return 1;
}

void start_timer() {
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 3;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	
	blist = create_listener("timer", &tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "timer", timer_handler, 0);
	
	crtx_start_listener(blist);
}

int epoll_main(int argc, char **argv) {
	char ret;
	struct crtx_evloop_fd evloop_fd;
	struct crtx_evloop_callback default_el_cb;
	
	if (argc > 1) {
		crtx_root->force_mode = CRTX_PREFER_ELOOP;
	} else {
		crtx_root->force_mode = CRTX_PREFER_THREAD;
	}
	
	crtx_get_main_event_loop();
	
	ret = pipe(testpipe); // 0 read, 1 write
	if (ret < 0) {
		ERROR("creating test pipe failed: %s\n", strerror(errno));
		return 1;
	}
	
	memset(&evloop_fd, 0, sizeof(struct crtx_evloop_fd));
	memset(&default_el_cb, 0, sizeof(struct crtx_evloop_callback));
	
	crtx_evloop_create_fd_entry(&evloop_fd, &default_el_cb,
						testpipe[0],
						EVLOOP_READ,
						0,
						&epoll_test_handler,
						0,
						0, 0
					);
	
	crtx_evloop_enable_cb(&crtx_root->event_loop, &default_el_cb);
	
	if (argc > 1) {
		start_timer();
		
		crtx_loop();
	} else {
		ret = crtx_start_listener(crtx_root->event_loop.listener);
		if (ret) {
			ERROR("starting epoll listener failed\n");
			return 1;
		}
		
		sleep(1);
		write(testpipe[1], "success", 7);
		
		sleep(1);
		
		printf("stop\n");
	}
	
	close(testpipe[0]);
	
	return 0;
}

CRTX_TEST_MAIN(epoll_main);
#endif
