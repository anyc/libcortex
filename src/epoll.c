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
#include <sys/stat.h>
#include <fcntl.h>

#ifdef DEBUG
#include <sys/ioctl.h>
#endif

#include "intern.h"
#include "core.h"
#include "epoll.h"
#include "evloop.h"

#ifndef CRTX_TEST

static int crtx_epoll_add_fd_intern(struct crtx_epoll_listener *epl, int fd, struct epoll_event *event) {
	int ret;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, fd, event);
	if (ret < 0) {
		CRTX_ERROR("epoll_ctl add failed for fd %d (add to %d): %s\n", fd, epl->epoll_fd, strerror(errno));
		
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
		CRTX_ERROR("epoll_ctl mod failed for fd %d: %s\n", fd, strerror(errno));
		
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
		CRTX_ERROR("epoll_ctl del failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

int crtx_event_flags2epoll_flags(int crtx_event_flags) {
	int ret;
	
	
	ret = 0;
	if (crtx_event_flags & CRTX_EVLOOP_READ)
		ret |= EPOLLIN;
	if (crtx_event_flags & CRTX_EVLOOP_WRITE)
		ret |= EPOLLOUT;
	if (crtx_event_flags & CRTX_EVLOOP_SPECIAL)
		ret |= EPOLLPRI;
	if (crtx_event_flags & CRTX_EVLOOP_EDGE_TRIGGERED)
		ret |= EPOLLET;
	
	return ret;
}

int crtx_epoll_flags2crtx_event_flags(int epoll_flags) {
	int ret;
	
	
	ret = 0;
	if (epoll_flags & EPOLLIN)
		ret |= CRTX_EVLOOP_READ;
	if (epoll_flags & EPOLLOUT)
		ret |= CRTX_EVLOOP_WRITE;
	if (epoll_flags & EPOLLPRI)
		ret |= CRTX_EVLOOP_SPECIAL;
	if (epoll_flags & EPOLLET)
		ret |= CRTX_EVLOOP_EDGE_TRIGGERED;
	if (epoll_flags & EPOLLERR)
		ret |= CRTX_EVLOOP_ERROR;
	if (epoll_flags & EPOLLHUP)
		ret |= CRTX_EVLOOP_HUP;
	if (epoll_flags & EPOLLRDHUP)
		ret |= CRTX_EVLOOP_RDHUP;
	
	return ret;
}

#if 1
static char *epoll_flags2str(unsigned int flags) {
	// #define RETFLAG(flag) if (*flags & flag) { *flags = (*flags) & (~flag); return #flag; }
	#define RETFLAG(flag) if (flags & flag) CRTX_VDBG(#flag " ");
		
	RETFLAG(EPOLLIN);
	RETFLAG(EPOLLOUT);
	RETFLAG(EPOLLRDHUP);
	RETFLAG(EPOLLPRI);
	RETFLAG(EPOLLERR);
	RETFLAG(EPOLLHUP);
	RETFLAG(EPOLLET);

	// CRTX_VDBG(" %u\n", *flags);
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
			CRTX_VDBG("epoll mod %d %s (%p)\n", *evloop_fd->fd, epoll_flags2str(epoll_event->events), evloop);
			
			crtx_epoll_mod_fd_intern(epl, *evloop_fd->fd, epoll_event);
		} else {
			CRTX_VDBG("epoll add %d %s (%p)\n", *evloop_fd->fd, epoll_flags2str(epoll_event->events), evloop);
			
			ret = crtx_epoll_add_fd_intern(epl, *evloop_fd->fd, epoll_event);
			if (ret < 0) {
				CRTX_ERROR("epoll add fd failed for: %s %d\n", evloop_fd->listener?evloop_fd->listener->name:"", *evloop_fd->fd);
			}
			
			evloop_fd->fd_added = (ret == 0);
			
			crtx_ll_append((struct crtx_ll**) &evloop->fds, &evloop_fd->ll);
		}
	} else {
		CRTX_VDBG("epoll del %d (%d %p)\n", *evloop_fd->fd, evloop->after_fork_close, evloop);
		
		crtx_ll_unlink((struct crtx_ll**) &evloop->fds, (struct crtx_ll*) &evloop_fd->ll);
		
		// after a fork, we close all file handles but we don't want to remove
		// them from an epoll instance as it would remove the file handle for
		// the parent process as well.
		if (!evloop->after_fork_close && *evloop_fd->fd >= 0)
			crtx_epoll_del_fd_intern(epl, *evloop_fd->fd);
		
		free(evloop_fd->el_data);
		evloop_fd->el_data = 0;
		
		evloop_fd->fd_added = 0;
	}
	
	return 0;
}

int crtx_epoll_mod_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
	int ret;
	
	ret = crtx_epoll_manage_fd(evloop, evloop_fd);
	
	return ret;
}

static int evloop_start_intern(struct crtx_event_loop *evloop, char onetime) {
	struct crtx_epoll_listener *epl;
	size_t i;
	struct crtx_evloop_fd *evloop_fd;
	struct crtx_evloop_callback *el_cb;
	struct timespec now_ts, then_ts, max_ts;
	uint64_t now;
	uint64_t timeout;
	int epoll_timeout_ms;
	char timeout_set, loop, events_processed;
	
	
	epl = (struct crtx_epoll_listener*) evloop->listener;
	
	/*
	 * In this loop, we first process events that may have been left unprocessed
	 * from a previous call of epoll_wait(). This can happen if calls of this
	 * function are nested, i.e. if an event handler enters this event loop
	 * again. Afterwards, we call timeout handlers, if necessary, and then
	 * finally call epoll_wait().
	 * 
	 * If $onetime == 1, we process events only one time. If events or timeouts
	 * are left from a previous epoll_wait() call, we only process them and then
	 * return. If no events or timeouts are left, we call epoll_wait(), process
	 * the returned events and timeout and then return.
	 */
	
	// onetime will decrease $loop after every loop
	if (onetime)
		loop = 2;
	else
		loop = 1;
	
	if (evloop->after_fork_close || evloop->phase_out || epl->stop)
		return 1;
	
	while (loop) {
		/*
		 * process (outstanding) events
		 */
		
		if (epl->cur_event_idx < epl->n_rdy_events)
			events_processed = 1;
		else
			events_processed = 0;
		
		while (epl->cur_event_idx < epl->n_rdy_events) {
			i = epl->cur_event_idx;
			
			epl->cur_event_idx += 1;
			
			evloop_fd = (struct crtx_evloop_fd* ) epl->events[i].data.ptr;
			
			#ifdef DEBUG
			CRTX_VDBG("epoll #%zu %d ", i, *evloop_fd->fd);
			#define PRINTFLAG(flag) if (epl->events[i].events & flag) CRTX_VDBG(#flag " ");
			
			PRINTFLAG(EPOLLIN);
			PRINTFLAG(EPOLLOUT);
			PRINTFLAG(EPOLLRDHUP);
			PRINTFLAG(EPOLLPRI);
			PRINTFLAG(EPOLLERR);
			PRINTFLAG(EPOLLHUP);
			CRTX_VDBG("\n");
			
			int rv;
			int bytesAvailable = 0;
			rv = ioctl(*evloop_fd->fd, FIONREAD, &bytesAvailable);
			if (!rv) {
				CRTX_VDBG("available data: %u bytes\n", bytesAvailable);
			} else {
				// this is not really necessary but it avoids confusion if somebody monitors errno
				errno = 0;
			}
			
			#endif
			
			if (epl->events[i].events & EPOLLERR || epl->events[i].events & EPOLLRDHUP || epl->events[i].events & EPOLLHUP) {
				if (epl->events[i].events & EPOLLERR)
					CRTX_DBG("epoll returned EPOLLERR for fd %d\n", *evloop_fd->fd);
				if (epl->events[i].events & EPOLLHUP)
					CRTX_DBG("epoll returned EPOLLHUP for fd %d\n", *evloop_fd->fd);
				if (epl->events[i].events & EPOLLRDHUP)
					CRTX_DBG("epoll returned EPOLLRDHUP for fd %d\n", *evloop_fd->fd);
				
				for (el_cb = evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
					if (el_cb->error_cb) {
						el_cb->triggered_flags = crtx_epoll_flags2crtx_event_flags(epl->events[i].events);
						
						el_cb->error_cb(el_cb, el_cb->error_cb_data);
					} else {
						CRTX_DBG("no error_cb for fd %d\n", *el_cb->fd_entry->fd);
					}
				}
			} else {
				for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
					if (!el_cb->active)
						continue;
					
					el_cb->triggered_flags = crtx_epoll_flags2crtx_event_flags(epl->events[i].events);
					
					crtx_evloop_callback(el_cb);
					
					if (evloop->after_fork_close)
						break;
				}
			}
			
			if (evloop->after_fork_close)
				break;
		}
		
		if (evloop->after_fork_close)
			break;
		
		/*
		 * process timeout callbacks
		 */
		
		clock_gettime(CRTX_DEFAULT_CLOCK, &now_ts);
		
		for (evloop_fd = evloop->fds; evloop_fd; evloop_fd = (struct crtx_evloop_fd *) evloop_fd->ll.next) {
			for (el_cb = evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
				if (el_cb->timeout.tv_sec > 0 || el_cb->timeout.tv_nsec > 0) {
					if (CRTX_CMP_TIMESPEC(&el_cb->timeout, &now_ts) < 0) {
						CRTX_VDBG("epoll timeout %d\n", *evloop_fd->fd);
						
						el_cb->timeout_enabled = 0;
						el_cb->timeout.tv_sec = 0;
						el_cb->timeout.tv_nsec = 0;
						
						if (!el_cb->active)
							continue;
						
						el_cb->triggered_flags = CRTX_EVLOOP_TIMEOUT;
						
						crtx_evloop_callback(el_cb);
						
						events_processed = 1;
					}
				}
				
				if (evloop->after_fork_close)
					break;
			}
			
			if (evloop->after_fork_close)
				break;
		}
		
		if (onetime && events_processed)
			break;
		if (onetime && loop == 1)
			break;
		if (evloop->after_fork_close)
			break;
		
		/*
		 * calculate the next timeout for the epoll_wait() call
		 */
		
		// used as theoretical maximum timestamp
		max_ts.tv_sec = 1000000000ULL;
		max_ts.tv_nsec = 1000000000ULL - 1;
		
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
				if (el_cb->timeout_enabled) {
					if (CRTX_CMP_TIMESPEC(&el_cb->timeout, &now_ts) <= 0) {
						CRTX_VDBG("timeout set by %s already in the past\n", evloop_fd->listener?evloop_fd->listener->id:"");
						timeout_set = 1;
						timeout = 0;
						break;
					} else {
						// theoretically, CRTX_timespec2uint64() could create a number that is too large for uint64
						
						memcpy(&then_ts, &now_ts, sizeof(struct timespec));
						then_ts.tv_sec += timeout / 1000000000ULL;
						then_ts.tv_nsec += timeout % 1000000000ULL;
						if (then_ts.tv_nsec > 1000000000ULL) {
							then_ts.tv_nsec -= 1000000000ULL;
							then_ts.tv_sec += 1;
						}
						
						if (CRTX_CMP_TIMESPEC(&el_cb->timeout, &then_ts) < 0) {
							if (CRTX_CMP_TIMESPEC(&max_ts, &el_cb->timeout) < 0) {
								timeout_set = 1;
								timeout = 1000000000ULL * 1000000000ULL;
							} else {
								timeout_set = 1;
								timeout = CRTX_timespec2uint64(&el_cb->timeout) - now;
							}
							
							CRTX_VDBG("timeout set by \"%s\": %" PRIu64 " ns\n", evloop_fd->listener?evloop_fd->listener->id:"", timeout);
						}
					}
				}
			}
		}
		
		if (timeout_set) {
			if (timeout / 1000000 > INT32_MAX) {
				epoll_timeout_ms = INT32_MAX;
			} else {
				epoll_timeout_ms = timeout / 1000000;
			}
			
			#ifndef CRTX_EPOLL_BUSY_WAIT_SUB_MSEC_TIMEOUT
			/*
			 * If $timeout is smaller than 1ms and epoll_timeout_ms == 0, the
			 * code would call epoll_wait() in a busy loop until the real timeout
			 * is reached. If smaller timeouts are really needed for low latency,
			 * a separate timer should be used.
			 */
			if (epoll_timeout_ms == 0) {
				epoll_timeout_ms = 1;
			}
			#endif
		} else {
			epoll_timeout_ms = epl->timeout;
		}
		
		if (evloop->phase_out)
			epoll_timeout_ms = 0;
		
		if (epl->stop)
			break;
		
		/*
		 * wait on events
		 */
		
		CRTX_VDBG("epoll waiting (timeout: %d ms)...\n", epoll_timeout_ms);
		
		epl->cur_event_idx = 0;
		while (1) {
			epl->n_rdy_events = epoll_wait(epl->epoll_fd, epl->events, epl->max_n_events, epoll_timeout_ms);
			
			if (epl->n_rdy_events < 0 && errno == EINTR)
				continue;
			else
				break;
		}
		
		if (epl->n_rdy_events < 0) {
			CRTX_ERROR("epoll_wait failed: %s (fd %d, events %p, max %zu, %d ms)\n", strerror(errno), epl->epoll_fd, epl->events, epl->max_n_events, epoll_timeout_ms);
			break;
		}
		
		CRTX_VDBG("epoll returned with %d events\n", epl->n_rdy_events);
		
		if (epl->n_rdy_events == 0 && evloop->phase_out)
			break;
		
		if (epl->stop)
			break;
		
		if (onetime)
			loop--;
	}
	
	CRTX_VDBG("epoll stops (%d %d %d)\n", epl->stop, onetime, evloop->after_fork_close);
	
	return 0;
}

static int evloop_start(struct crtx_event_loop *evloop) {
	return evloop_start_intern(evloop, 0);
}

static int evloop_oneshot(struct crtx_event_loop *evloop) {
	return evloop_start_intern(evloop, 1);
}

void *crtx_epoll_main(void *data) {
	evloop_start( (struct crtx_event_loop*) data );
	
	return 0;
}

static int evloop_stop(struct crtx_event_loop *evloop) {
	struct crtx_event_loop_control_pipe ecp;
	struct crtx_epoll_listener *epl;
	ssize_t ret;
	
	CRTX_VDBG("sending stop to epoll pipe %d %d\n", evloop->ctrl_pipe[0], evloop->ctrl_pipe[1]);
	
	epl = (struct crtx_epoll_listener *) evloop->listener;
	epl->stop = 1;
	
	ecp.graph = 0;
	ecp.el_cb = 0;
	
	ret = write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	if (ret < sizeof(struct crtx_event_loop_control_pipe)) {
		CRTX_ERROR("evloop_stop() write returned %zd %s\n", ret, strerror(errno));
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
	
// 	crtx_evloop_release(epl->evloop);
	
// 	epl->evloop->listener = 0;
	
	stop_thread(0, epl->evloop);
	
	free(epl->events);
	
	if (epl->epoll_fd >= 0) {
		CRTX_VDBG("close epoll fd %d\n", epl->epoll_fd);
		
		close(epl->epoll_fd);
	}
}

static char stop_epoll_listener(struct crtx_listener_base *lbase) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) lbase;
	
	evloop_stop(epl->evloop);
	
	return 0;
}

struct crtx_listener_base *crtx_setup_epoll_listener(void *options) {
	struct crtx_epoll_listener *epl;
	int flags;
	
	
	epl = (struct crtx_epoll_listener*) options;
	
	if (crtx_root->global_fd_flags & FD_CLOEXEC)
		flags = EPOLL_CLOEXEC;
	else
		flags = 0;
	
	epl->epoll_fd = epoll_create1(flags);
	if (epl->epoll_fd < 0) {
		CRTX_ERROR("epoll_create1 failed: %s\n", strerror(errno));
		return 0;
	}
	
	epl->base.shutdown = &shutdown_epoll_listener;
	epl->base.stop_listener = &stop_epoll_listener;
	
	epl->base.thread_job.fct = &crtx_epoll_main;
	epl->base.thread_job.fct_data = epl->evloop;
	epl->base.thread_job.do_stop = &stop_thread;
	
	// either the main thread will execute crtx_epoll_main or we spawn a new thread
	epl->base.mode = CRTX_PREFER_THREAD;
	
	
	if (epl->max_n_events < 1)
		epl->max_n_events = 1;
	
	// 0 is our default, -1 is no timeout for epoll
	epl->timeout -= 1;
	
	
	epl->events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * epl->max_n_events);
	epl->n_rdy_events = 0;
	epl->cur_event_idx = 0;
	
	CRTX_VDBG("new epoll fd %d\n", epl->epoll_fd);
	
	return &epl->base;
}

static int evloop_create(struct crtx_event_loop *evloop) {
	int ret;
	struct crtx_epoll_listener *epl;
	
	if (!evloop->listener)
		evloop->listener = (struct crtx_listener_base*) calloc(1, sizeof(struct crtx_epoll_listener));
	epl = (struct crtx_epoll_listener *) evloop->listener;
	
	epl->evloop = evloop;
	
	ret = crtx_setup_listener("epoll", evloop->listener);
	if (ret) {
		CRTX_ERROR("create_listener(epoll) failed\n");
	}
	
	return ret;
}

static int evloop_release(struct crtx_event_loop *evloop) {
	crtx_shutdown_listener(evloop->listener);
	free(evloop->listener);
	
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
	.onetime = &evloop_oneshot,
	
	.mod_fd = &crtx_epoll_mod_fd,
};

void crtx_epoll_init() {
	epoll_loop.ll.data = &epoll_loop;
	crtx_ll_append((struct crtx_ll**) &crtx_event_loops, &epoll_loop.ll);
}

void crtx_epoll_finish() {
}


#else /* CRTX_TEST */

#include <fcntl.h>
#include "timer.h"

struct crtx_timer_listener tlist;
struct itimerspec newtimer;
int testpipe[2];
int count = 0;

static int epoll_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char buf[1024];
	ssize_t n_read;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	n_read = read(*el_cb->fd_entry->fd, buf, sizeof(buf)-1);
	if (n_read < 0) {
		CRTX_ERROR("epoll test read failed: %s\n", strerror(errno));
		return 1;
	}
	buf[n_read] = 0;
	
	printf("read: \"%s\" (%zd)\n", buf, n_read);
	
	return 1;
}

static int timer_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	ssize_t srv;
	
	printf("timerhandler\n");
	
	if (count > 2) {
		crtx_init_shutdown();
	} else {
		srv = write(testpipe[1], "success", 7);
		if (srv != 7)
			fprintf(stderr, "write failed: %zd\n", srv);
		count++;
	}
	
	return 1;
}

void start_timer() {
	int r;
	
	
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 3;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	
	r = crtx_setup_listener("timer", &tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		exit(1);
	}
	
	crtx_create_task(tlist.base.graph, 0, "timer", timer_handler, 0);
	
	crtx_start_listener(&tlist.base);
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
		CRTX_ERROR("creating test pipe failed: %s\n", strerror(errno));
		return 1;
	}
	
	memset(&evloop_fd, 0, sizeof(struct crtx_evloop_fd));
	memset(&default_el_cb, 0, sizeof(struct crtx_evloop_callback));
	
	crtx_evloop_create_fd_entry(&evloop_fd, &default_el_cb,
						0, testpipe[0],
						CRTX_EVLOOP_READ,
						0,
						&epoll_test_handler,
						0,
						0, 0
					);
	
	crtx_evloop_enable_cb(&default_el_cb);
	
	if (argc > 1) {
		start_timer();
		
		crtx_loop();
	} else {
		ret = crtx_start_listener(crtx_root->event_loop->listener);
		if (ret) {
			CRTX_ERROR("starting epoll listener failed\n");
			return 1;
		}
		
		sleep(1);
		ssize_t srv = write(testpipe[1], "success", 7);
		if (srv != 7)
			fprintf(stderr, "write failed: %zd\n", srv);
		
		sleep(1);
		
		printf("stop\n");
	}
	
	crtx_evloop_disable_cb(&default_el_cb);
	
	close(testpipe[0]);
	
	return 0;
}

CRTX_TEST_MAIN(epoll_main);
#endif
