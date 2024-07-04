/*
 * Mario Kicherer (dev@kicherer.org) 2022
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "core.h"
#include "dict.h"
#include "timer.h"
#include "sd_journal.h"
#include "epoll.h"

#ifndef CRTX_TEST

static int set_events_timeout(struct crtx_sd_journal_listener *jlstnr) {
	int epoll_events, rv;
	uint64_t timeout;
	
	
	epoll_events = sd_journal_get_events(jlstnr->sd_journal);
	if (epoll_events < 0) {
		CRTX_ERROR("sd_journal_get_events() failed: %s\n", strerror(-epoll_events));
		return epoll_events;
	}
	
	jlstnr->base.default_el_cb.crtx_event_flags = crtx_epoll_flags2crtx_event_flags(epoll_events);
	
	rv = sd_journal_get_timeout(jlstnr->sd_journal, &timeout);
	if (rv < 0) {
		CRTX_ERROR("sd_journal_get_timeout() failed: %s\n", strerror(-rv));
		return rv;
	}
	if (timeout > (uint64_t) -1) {
		crtx_evloop_set_timeout_rel(&jlstnr->base.default_el_cb, timeout);
	}
	
	return 0;
}

int crtx_sd_journal_print_event(struct crtx_sd_journal_listener *jlstnr, void *userdata) {
	int rv;
	char *data;
	size_t data_length;
	uint64_t usec;
	time_t sec;
	char tbuf[24];
	struct tm tm;
	struct crtx_sd_journal_print_event_userdata *udata;
	
	
	udata = (struct crtx_sd_journal_print_event_userdata*) userdata;
	
	while (sd_journal_next(jlstnr->sd_journal)) {
		rv = sd_journal_get_data(jlstnr->sd_journal, "MESSAGE", (void *) &data, &data_length);
		if (rv < 0) {
			CRTX_ERROR("sd_journal_get_data() failed: %s", strerror(-rv));
			return 0;
		}
		
		rv = sd_journal_get_realtime_usec(jlstnr->sd_journal, &usec);
		if (rv < 0) {
			CRTX_ERROR("sd_journal_get_realtime_usec() failed: %s", strerror(-rv));
			return 0;
		}
		
		sec = usec / 1000000;
		
		localtime_r(&sec, &tm);
		strftime(tbuf, sizeof(tbuf), "%b %d %H:%M:%S", &tm);
		
		if (!udata) {
			printf("%s ", tbuf);
			printf("MSG: %.*s\n", (int) data_length, data);
		} else {
			if (udata->f) {
				fprintf(udata->f, "%s ", tbuf);
				fprintf(udata->f, "MSG: %.*s\n", (int) data_length, data);
			}
			if (udata->buf) {
				snprintf(udata->buf, udata->buf_size, "%s: %.*s", tbuf, (int) data_length, data);
			}
			if (udata->cb)
				udata->cb(udata);
		}
	}
	
	return 0;
}

int crtx_sd_journal_gen_event_dict(struct crtx_sd_journal_listener *jlstnr, void *userdata) {
	int rv;
	char *data, *sep;
	const void *tuple;
	size_t tuple_length, data_length;
	uint64_t usec;
	time_t sec;
	char tbuf[24], key[32];
	struct tm tm;
	
	struct crtx_event *levent;
	struct crtx_dict *dict;
	
	while (sd_journal_next(jlstnr->sd_journal)) {
		rv = sd_journal_get_realtime_usec(jlstnr->sd_journal, &usec);
		if (rv < 0) {
			CRTX_ERROR("sd_journal_get_realtime_usec() failed: %s", strerror(-rv));
			return 0;
		}
		
		sec = usec / 1000000;
		
		localtime_r(&sec, &tm);
		strftime(tbuf, sizeof(tbuf), "%b %d %H:%M:%S", &tm);
		
		rv = crtx_create_event(&levent);
		if (rv) {
			CRTX_ERROR("crtx_create_event() failed: %s\n", strerror(rv));
			return rv;
		}
		
		levent->type = CRTX_SD_JOURNAL_EVT_NEW_ENTRY_ID;
		levent->description = CRTX_SD_JOURNAL_EVT_NEW_ENTRY;
		
		dict = crtx_init_dict(0, 0, 0);
		
		SD_JOURNAL_FOREACH_DATA(jlstnr->sd_journal, tuple, tuple_length) {
			CRTX_DBG("sd_journal: \"%.*s\"\n", (int) tuple_length, (char*) tuple);
			
			sep = strchr(tuple, '=');
			if (!sep) {
				CRTX_ERROR("cannot parse: \"%.*s\"\n", (int) tuple_length, (char*) tuple);
				crtx_dict_unref(dict);
				free_event(levent);
				return rv;
			}
			
			if (sep - (char*) tuple >= sizeof(key)) {
				CRTX_ERROR("error, key in \"%.*s\" too large\n", (int) tuple_length, (char*)tuple);
				continue;
			}
			memcpy(key, tuple, sep-(char*)tuple);
			key[sep-(char*)tuple] = 0;
			
			data_length = tuple_length - (sep-(char*)tuple);
			data = (char*) calloc(1, data_length + 1);
			memcpy(data, sep + 1, data_length);
			
			rv = crtx_dict_new_item(dict, 's', key, data, data_length, CRTX_DIF_CREATE_KEY_COPY);
			if (rv) {
				CRTX_ERROR("crtx_fill_data_item() failed: %d\n", rv);
				crtx_dict_unref(dict);
				free_event(levent);
				return rv;
			}
		}
		
		rv = crtx_fill_data_item(&levent->data, 'D', 0, dict);
		if (rv) {
			CRTX_ERROR("crtx_fill_data_item() failed: %d\n", rv);
			crtx_dict_unref(dict);
			free_event(levent);
			return rv;
		}
		
		crtx_add_event(jlstnr->base.graph, levent);
	}
	
	return 0;
}

static char sd_journal_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_sd_journal_listener *jlstnr;
	int rv;
	
	
	jlstnr = (struct crtx_sd_journal_listener *) userdata;
	
	rv = sd_journal_process(jlstnr->sd_journal);
	if (rv < 0) {
		CRTX_ERROR("sd_journal_process() failed: %s", strerror(-rv));
		return 0;
	}
	
	if (rv == SD_JOURNAL_APPEND || (rv == SD_JOURNAL_NOP && jlstnr->try_read)) {
		if (jlstnr->event_cb)
			jlstnr->event_cb(jlstnr, jlstnr->event_cb_data);
	} else if (rv == SD_JOURNAL_INVALIDATE) {
		// all logs should be reloaded TODO: should we just handle this like APPEND?
		CRTX_DBG("will reopen journal\n");
		crtx_stop_listener(&jlstnr->base);
		crtx_start_listener(&jlstnr->base);
	} else if (rv == SD_JOURNAL_NOP) {
		// do nothing
	} else {
		CRTX_ERROR("unexpected sd_journal_process() return value: %d", rv);
	}
	
	set_events_timeout(jlstnr);
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	int rv, fd, i;
	struct crtx_sd_journal_listener *jlstnr;
	
	
	jlstnr = (struct crtx_sd_journal_listener *) listener;
	
	rv = sd_journal_open(&jlstnr->sd_journal, jlstnr->flags);
	if (rv < 0) {
		CRTX_ERROR("sd_journal_open() failed: %s", strerror(-rv));
		return 0;
	}
	
	fd = sd_journal_get_fd(jlstnr->sd_journal);
	if (fd < 0) {
		CRTX_ERROR("sd_journal_get_fd() failed: %s\n", strerror(-fd));
		return fd;
	}
	
	crtx_evloop_init_listener(&jlstnr->base,
							  fd,
						   CRTX_EVLOOP_READ,
						   0,
						   &sd_journal_fd_event_handler,
						   jlstnr,
						   0, 0
		);
	
	rv = set_events_timeout(jlstnr);
	if (rv) {
		return rv;
	}
	
	if (jlstnr->seek < 0) {
		rv = sd_journal_seek_tail(jlstnr->sd_journal);
		if (rv < 0) {
			CRTX_ERROR("sd_journal_seek_tail() failed: %s", strerror(-rv));
			return 0;
		}
		
		for (i=0; i < -jlstnr->seek + 1; i++) {
			rv = sd_journal_previous(jlstnr->sd_journal);
			if (rv < 0) {
				CRTX_ERROR("sd_journal_previous() failed: %s", strerror(-rv));
				return 0;
			}
		}
		
		jlstnr->try_read = 1;
		crtx_trigger_event_processing(&jlstnr->base);
	} else
	if (jlstnr->seek > 0) {
		for (i=0; i < jlstnr->seek; i++) {
			rv = sd_journal_next(jlstnr->sd_journal);
			if (rv < 0) {
				CRTX_ERROR("sd_journal_next() failed: %s", strerror(-rv));
// 				return 0;
			}
		}
	}
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	struct crtx_sd_journal_listener *jlstnr;
	
	jlstnr = (struct crtx_sd_journal_listener *) listener;
	
	sd_journal_close(jlstnr->sd_journal);
	
	return 0;
}

struct crtx_listener_base *crtx_setup_sd_journal_listener(void *options) {
	struct crtx_sd_journal_listener *jlstnr;
	
	
	jlstnr = (struct crtx_sd_journal_listener *) options;
	
	jlstnr->base.start_listener = &start_listener;
	jlstnr->base.stop_listener = &stop_listener;
	
	return &jlstnr->base;
}

void crtx_sd_journal_clear_listener(struct crtx_sd_journal_listener *lstnr) {
	memset(lstnr, 0, sizeof(struct crtx_sd_journal_listener));
}

CRTX_DEFINE_CALLOC_FUNCTION(sd_journal)

void crtx_sd_journal_init() {
}

void crtx_sd_journal_finish() {
}

#else /* CRTX_TEST */

char * topic = 0;

static char sd_journal_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_print_event(event, 0);
	printf("\n");
	
	return 0;
}

int main(int argc, char **argv) {
	struct crtx_sd_journal_listener jlist;
	int rv;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_sd_journal_clear_listener(&jlist);
	
	jlist.flags = SD_JOURNAL_SYSTEM | SD_JOURNAL_CURRENT_USER | SD_JOURNAL_LOCAL_ONLY;
	jlist.seek = -10;
// 	jlist.event_cb = &crtx_sd_journal_print_event;
	jlist.event_cb = &crtx_sd_journal_gen_event_dict;
	
	rv = crtx_setup_listener("sd_journal", &jlist);
	if (rv) {
		CRTX_ERROR("create_listener(sd_journal) failed\n");
		return 0;
	}
	
	crtx_create_task(jlist.base.graph, 0, "sd_journal_event", &sd_journal_event_handler, &jlist);
	
	rv = crtx_start_listener(&jlist.base);
	if (rv) {
		CRTX_ERROR("starting sd_journal listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}

#endif
