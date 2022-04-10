/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>

#include "intern.h"
#include "core.h"
#include "timer.h"
#include "curl.h"

#define CURLM_CHECK(curlr) { if (curlr != CURLM_OK) { CRTX_ERROR("curl returned: %s (%d)\n", curl_multi_strerror(curlr), curlr); } }
#define CURL_CHECK(curlr) { if (curlr != CURLE_OK) { CRTX_ERROR("curl returned: %s (%d)\n", curl_easy_strerror(curlr), curlr); } }

#ifndef CRTX_TEST

struct crtx_curl_socket {
	struct crtx_ll next;
	
	struct crtx_curl_listener *clist;
	
	curl_socket_t socket_fd;
	struct crtx_evloop_fd el_fd;
	struct crtx_evloop_callback el_cb;
};

static CURLM *crtx_curlm;
static struct crtx_timer_listener timer_list;
static struct crtx_curl_socket *sockets;

static char timeout_handler(struct crtx_event *event, void *userdata, void **sessiondata);
static void init_curl();



static size_t write_callback(char *ptr, size_t size, size_t nmemb, struct crtx_curl_listener *clist) {
	if (clist->write_file_ptr)
		return fwrite(ptr, size, nmemb, clist->write_file_ptr);
	if (clist->write_fd)
		return write(clist->write_fd, ptr, size * nmemb);
	
	return size * nmemb;
}

int crtx_curl_progress_callback(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	struct crtx_event *user_event;
	struct crtx_dict *dict;
	struct crtx_curl_listener *clist;
	
	
	clist = (struct crtx_curl_listener *) userdata;
	
	crtx_create_event(&user_event);
	user_event->type = CRTX_CURL_ET_PROGRESS;
	
	printf("curl progress: %"PRId64" / %"PRId64" %"PRId64" / %"PRId64"\n", dlnow, dltotal, ulnow, ultotal);
	
	dict = crtx_create_dict("IIII",
					    "dltotal", dltotal, sizeof(dltotal), 0,
					    "dlnow", dlnow, sizeof(dlnow), 0,
					    "ultotal", ultotal, sizeof(ultotal), 0,
					    "ulnow", ulnow, sizeof(ulnow), 0
					);
	
	crtx_event_set_dict_data(user_event, dict, 0);
	
	crtx_add_event(clist->base.graph, user_event);
	
	return 0;
}

size_t crtx_curl_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	struct crtx_event *user_event;
	struct crtx_dict *dict;
	char *string;
	struct crtx_curl_listener *clist;
	
	
	clist = (struct crtx_curl_listener *) userdata;
	
	crtx_create_event(&user_event);
	user_event->type = CRTX_CURL_ET_HEADER;
	
	string = (char *) malloc(size * nitems + 1);
	memcpy(string, buffer, size * nitems);
	string[size * nitems] = 0;
	
	dict = crtx_create_dict("s",
					    "line", string, size * nitems, 0
					);
	
	crtx_event_set_dict_data(user_event, dict, 0);
	
	crtx_add_event(clist->base.graph, user_event);
	
	return nitems * size;
}

static char stop_listener(struct crtx_listener_base *data) {
	struct crtx_curl_listener *clist;
	
	clist = (struct crtx_curl_listener*) data;
	
	// TODO test
	
	if (clist->easy) {
		curl_multi_remove_handle(crtx_curlm, clist->easy);
		curl_easy_cleanup(clist->easy);
		
		clist->easy = 0;
	}
	
	return 1;
}

static char start_listener(struct crtx_listener_base *base) {
	struct crtx_curl_listener *clist;
	CURLMcode curlr;
	
	
	clist = (struct crtx_curl_listener *) base;
	
	curlr = curl_multi_add_handle(crtx_curlm, clist->easy);
	if (curlr != CURLM_OK) {
		CRTX_ERROR("curl_multi_add_handle failed: %s\n", curl_easy_strerror(curlr));
		return 0;
	}
	
	// we have no fd or thread to monitor
	clist->base.mode = CRTX_NO_PROCESSING_MODE;
	
	return 1;
}

struct crtx_listener_base *crtx_setup_curl_listener(void *options) {
	struct crtx_curl_listener *clist;
	CURLcode curlr;
	
	clist = (struct crtx_curl_listener *) options;
	
	if (!clist->url) {
		CRTX_ERROR("curl: missing url\n");
		return 0;
	}
	
	clist->base.start_listener = &start_listener;
	clist->base.stop_listener = &stop_listener;
	
	clist->error[0] = 0;
	
	if (!crtx_curlm)
		init_curl();
	
	clist->easy = curl_easy_init();
	if(!clist->easy) {
		CRTX_ERROR("curl_easy_init() failed, exiting!\n");
		return 0;
	}
	
	curlr = curl_easy_setopt(clist->easy, CURLOPT_URL, clist->url); CURL_CHECK(curlr);
	if (!clist->write_callback) {
		curlr = curl_easy_setopt(clist->easy, CURLOPT_WRITEFUNCTION, write_callback); CURL_CHECK(curlr);
		curlr = curl_easy_setopt(clist->easy, CURLOPT_WRITEDATA, clist); CURL_CHECK(curlr);
	} else {
		curlr = curl_easy_setopt(clist->easy, CURLOPT_WRITEFUNCTION, clist->write_callback); CURL_CHECK(curlr);
		curlr = curl_easy_setopt(clist->easy, CURLOPT_WRITEDATA, clist->write_cb_data); CURL_CHECK(curlr);
	}
	
	curlr = curl_easy_setopt(clist->easy, CURLOPT_ERRORBUFFER, clist->error); CURL_CHECK(curlr);
	curlr = curl_easy_setopt(clist->easy, CURLOPT_PRIVATE, clist); CURL_CHECK(curlr);
	
	if (clist->progress_callback) {
		#if LIBCURL_VERSION_NUM >= 0x072000 // 0.7.32
		curlr = curl_easy_setopt(clist->easy, CURLOPT_XFERINFOFUNCTION, clist->progress_callback); CURL_CHECK(curlr);
		curlr = curl_easy_setopt(clist->easy, CURLOPT_XFERINFODATA, clist->progress_cb_data); CURL_CHECK(curlr);
		#else
		curlr = curl_easy_setopt(clist->easy, CURLOPT_PROGRESSFUNCTION, clist->progress_callback); CURL_CHECK(curlr);
		curlr = curl_easy_setopt(clist->easy, CURLOPT_PROGRESSDATA, clist->progress_cb_data); CURL_CHECK(curlr);
		#endif
		curlr = curl_easy_setopt(clist->easy, CURLOPT_NOPROGRESS, 0L); CURL_CHECK(curlr);
	}
	
	if (clist->header_callback) {
		curlr = curl_easy_setopt(clist->easy, CURLOPT_HEADERFUNCTION, clist->header_callback); CURL_CHECK(curlr);
		curlr = curl_easy_setopt(clist->easy, CURLOPT_HEADERDATA, clist->header_cb_data); CURL_CHECK(curlr);
	}
	
	curlr = curl_easy_setopt(clist->easy, CURLOPT_FOLLOWLOCATION, 1L); CURL_CHECK(curlr);
	
	// curlr = curl_easy_setopt(clist->easy, CURLOPT_VERBOSE, 1L); CURL_CHECK(curlr);
	
	return &clist->base;
}

static int timer_callback(CURLM *multi, long timeout_ms, CURLM *curlm) {
	int err;
	
	// WARNING, explicitly discouraged by upstream, see https://curl.haxx.se/libcurl/c/CURLMOPT_TIMERFUNCTION.html
// 	if (timeout_ms == 0) {
// 		timeout_handler(0, 0, 0);
// 	}
	
	if (timeout_ms == -1) {
		// disable the timer
		timeout_ms = 0;
	}
	
	CRTX_DBG("curl set timeout %ld ms\n", timeout_ms);
	
	// set time for (first) alarm
	timer_list.newtimer.it_value.tv_sec = timeout_ms / 1000;
	timer_list.newtimer.it_value.tv_nsec = timeout_ms * 1000000 % 1000000000;
	
	// workaround - as setting both fields to 0 would deactivate the timer
	if (timeout_ms == 0)
		timer_list.newtimer.it_value.tv_nsec = 1;
	
	// set inteerr for repeating alarm, set to 0 to disable repetition
	timer_list.newtimer.it_interval.tv_sec = 0;
	timer_list.newtimer.it_interval.tv_nsec = 0;
	
	timer_list.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	timer_list.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	
	err = crtx_update_listener(&timer_list.base);
	if (err) {
		CRTX_ERROR("starting timer failed: %d\n", err);
		return 1;
	}
	
	return 0;
}

static void process_curl_infos() {
	CURLMsg *msg;
	struct crtx_curl_listener *clist;
	char *eff_url;
	int msgs_left;
	struct crtx_event *user_event;
	
	
	while (1) {
		msg = curl_multi_info_read(crtx_curlm, &msgs_left);
		if (!msg)
			break;
		
		if (msg->msg == CURLMSG_DONE) {
			long response_code, error_code;
			curl_off_t dltotal;
			
			
			curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &clist);
			curl_easy_getinfo(msg->easy_handle, CURLINFO_EFFECTIVE_URL, &eff_url);
			curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
			curl_easy_getinfo(msg->easy_handle, CURLINFO_OS_ERRNO, &error_code);
			
			#if (LIBCURL_VERSION_NUM < 0x073700)
			double total;
			
			curl_easy_getinfo(msg->easy_handle, CURLINFO_SIZE_DOWNLOAD, &total);
			dltotal = total;
			#else
			curl_easy_getinfo(msg->easy_handle, CURLINFO_SIZE_DOWNLOAD_T, &dltotal);
			#endif
			
			
			CRTX_DBG("curl done: %s (response: %ld, errno: %ld, result %d, error \"%s\", size %"CURL_FORMAT_CURL_OFF_T" bytes)\n",
					eff_url, response_code, error_code, msg->data.result, clist->error, dltotal);
			
			if (clist->done_callback) {
				clist->done_callback(clist->done_cb_data, msg, response_code, error_code);
			}
			
			if (clist->easy) {
				curl_multi_remove_handle(crtx_curlm, clist->easy);
				curl_easy_cleanup(clist->easy);
				
				clist->easy = 0;
			}
			
			if (!clist->done_callback) {
				crtx_create_event(&user_event);
				user_event->type = CRTX_CURL_ET_FINISHED;
				crtx_event_set_raw_data(user_event, 'p', clist, sizeof(clist), CRTX_DIF_DONT_FREE_DATA);
				
				crtx_add_event(clist->base.graph, user_event);
			}
		}
	}
}

static char fd_event_callback(struct crtx_event *event, void *userdata, void **sessiondata) {
	CURLMcode curlr;
	int action;
	struct crtx_evloop_callback *el_cb;
	struct crtx_curl_socket *socketp;
	int still_running;
	
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	socketp = (struct crtx_curl_socket *) userdata;
	
	if (el_cb->triggered_flags & CRTX_EVLOOP_TIMEOUT) {
		timeout_handler(0, 0, 0);
	} else {
		action = 0;
		if (el_cb->triggered_flags & CRTX_EVLOOP_READ)
			action |= CURL_CSELECT_IN;
		if (el_cb->triggered_flags & CRTX_EVLOOP_WRITE)
			action |= CURL_CSELECT_OUT;
		
		curlr = curl_multi_socket_action(crtx_curlm, socketp->socket_fd, action, &still_running);
		if (curlr != CURLM_OK) {
			CRTX_ERROR("curl_multi_socket_action() failed: %s\n", curl_easy_strerror(curlr));
		}
		
		process_curl_infos();
	}
	
	return 1;
}

static char timeout_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	int still_running;
	CURLMcode curlr;
	
	curlr = curl_multi_socket_action(crtx_curlm, CURL_SOCKET_TIMEOUT, 0, &still_running);
	if (curlr != CURLM_OK) {
		CRTX_ERROR("curl_multi_socket_action(CURL_SOCKET_TIMEOUT) failed: %s\n", curl_easy_strerror(curlr));
		return 0;
	}
	
	process_curl_infos();
	
	return 1;
}

static int update_socket_fd(struct crtx_curl_socket *s, int what) {
	switch (what) {
		case CURL_POLL_IN:
			s->el_cb.crtx_event_flags = CRTX_EVLOOP_READ;
			break;
		case CURL_POLL_OUT:
			s->el_cb.crtx_event_flags = CRTX_EVLOOP_WRITE;
			break;
		case CURL_POLL_INOUT:
			s->el_cb.crtx_event_flags = CRTX_EVLOOP_READ | CRTX_EVLOOP_WRITE;
			break;
		default:
			CRTX_ERROR("unexpected \"what\" value in curl socket callback: %d\n", what);
			return -1;
	}
	
	return 0;
}

static int socket_callback(CURL *easy, curl_socket_t curl_socket, int what, CURLM *curlm, struct crtx_curl_socket *socketp) {
	struct crtx_curl_listener *clist;
	
	
	CRTX_DBG("new curl socket event: \"%s\" (fd %d)\n",
		(what == CURL_POLL_REMOVE) ? "remove": (what == CURL_POLL_IN) ? "in": (what == CURL_POLL_OUT) ? "out": (what == CURL_POLL_INOUT) ? "inout": "unknown",
		curl_socket);
	
	curl_easy_getinfo(easy, CURLINFO_PRIVATE, &clist);
	
	if (what == CURL_POLL_REMOVE) {
		socketp->el_cb.crtx_event_flags = 0;
		
		if (fcntl(socketp->el_fd.fd, F_GETFD) == -1) {
			// CURL sometimes closes the fd before we get this callback
			socketp->el_fd.fd = 0;
		}
		
		crtx_evloop_disable_cb(&socketp->el_cb);
	} else {
		if(!socketp) {
			socketp = (struct crtx_curl_socket*) calloc(1, sizeof(struct crtx_curl_socket));
			
			socketp->socket_fd = curl_socket;
			socketp->clist = clist;
			
			crtx_evloop_create_fd_entry(
					&socketp->el_fd,
					&socketp->el_cb,
					socketp->socket_fd,
					0, // access type
					0, // graph
					&fd_event_callback,
					socketp,
					0, 0
					);
			socketp->el_cb.active = 1;
			socketp->el_fd.listener = clist;
			
			update_socket_fd(socketp, what);
			
// 			crtx_evloop_add_el_fd(&socketp->el_fd);
			crtx_evloop_enable_cb(&socketp->el_cb);
			
			crtx_ll_append((struct crtx_ll **) &sockets, &socketp->next);
			
			curl_multi_assign(crtx_curlm, curl_socket, socketp);
		} else {
			update_socket_fd(socketp, what);
			
// 			crtx_evloop_set_el_fd(&socketp->el_fd);
			crtx_evloop_enable_cb(&socketp->el_cb);
		}
	}
	
	return 0;
}

static void init_curl() {
	CURLMcode curlr;
	int err;
	
	
	memset(&timer_list, 0, sizeof(struct crtx_timer_listener));
	
	err = crtx_setup_listener("timer", &timer_list);
	if (err) {
		CRTX_ERROR("create_listener(timer) failed: %d\n", err);
		// TODO
		return;
	}
	
	crtx_create_task(timer_list.base.graph, 0, 0, &timeout_handler, 0);
	
	err = crtx_start_listener(&timer_list.base);
	if (err) {
		CRTX_ERROR("starting curl listener failed\n");
		return;
	}
	
	crtx_curlm = curl_multi_init();
	
	curlr = curl_multi_setopt(crtx_curlm, CURLMOPT_SOCKETFUNCTION, socket_callback); CURLM_CHECK(curlr);
	curlr = curl_multi_setopt(crtx_curlm, CURLMOPT_SOCKETDATA, crtx_curlm); CURLM_CHECK(curlr);
	curlr = curl_multi_setopt(crtx_curlm, CURLMOPT_TIMERFUNCTION, timer_callback); CURLM_CHECK(curlr);
	curlr = curl_multi_setopt(crtx_curlm, CURLMOPT_TIMERDATA, crtx_curlm); CURLM_CHECK(curlr);
}

CRTX_DEFINE_ALLOC_FUNCTION(curl)

void crtx_curl_init() {
	curl_global_init(CURL_GLOBAL_ALL);
	
	sockets = 0;
	crtx_curlm = 0;
}

void crtx_curl_finish() {
	if (crtx_curlm)
		curl_multi_cleanup(crtx_curlm);
	
	curl_global_cleanup();
}

#else /* CRTX_TEST */

size_t last_dlnow = 0;
struct crtx_curl_listener clist;

static char curl_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_CURL_ET_FINISHED) {
		printf("curl finished, shutdown now\n");
		
		crtx_init_shutdown();
	}
	
	if (event->type == CRTX_CURL_ET_PROGRESS) {
		int r;
		int64_t dlnow, dltotal;
		
		r = crtx_event_get_value_by_key(event, "dlnow", 'I', &dlnow, sizeof(dlnow));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		r = crtx_event_get_value_by_key(event, "dltotal", 'I', &dltotal, sizeof(dltotal));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		
		if (last_dlnow != dlnow) {
			fprintf(stderr, "progress: %"PRId64" / %"PRId64"\n", dlnow, dltotal);
			last_dlnow = dlnow;
		}
	}
	
	if (event->type == CRTX_CURL_ET_HEADER) {
		int r;
		char *s;
		
		r = crtx_event_get_value_by_key(event, "line", 's', &s, sizeof(s));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		
		printf("H: %s\n", s);
	}
	
	return 0;
}

int curl_main(int argc, char **argv) {
	int ret;
	
	
	memset(&clist, 0, sizeof(struct crtx_curl_listener));
	
	clist.url = argv[1];
	clist.write_file_ptr = stdout;
	clist.progress_callback = &crtx_curl_progress_callback;
	clist.progress_cb_data = &clist;
	
	clist.header_callback = &crtx_curl_header_callback;
	clist.header_cb_data = &clist;
	
	ret = crtx_setup_listener("curl", &clist);
	if (ret) {
		CRTX_ERROR("create_listener(curl) failed\n");
		return 0;
	}
	
// 	curl_easy_setopt(clist.easy, CURLOPT_NOBODY, 1);
// 	curl_easy_setopt(clist.easy, CURLOPT_VERBOSE, 1L);
	
	crtx_create_task(clist.base.graph, 0, "curl_test", &curl_event_handler, 0);
	
	ret = crtx_start_listener(&clist.base);
	if (ret) {
		CRTX_ERROR("starting curl listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(curl_main);
#endif
