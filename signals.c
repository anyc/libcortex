/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifndef WITHOUT_SIGNALFD
#include <sys/signalfd.h>
#endif

#include "intern.h"
#include "signals.h"

#define SIGNAL(id) { id, #id, "cortex.signals." #id }
struct signal_map {
	int id;
	char *name;
	char *etype;
} sigmap[] = {
	SIGNAL(SIGHUP),
	SIGNAL(SIGINT),
	SIGNAL(SIGPIPE),
	SIGNAL(SIGTERM),
	SIGNAL(SIGUSR1),
	{0, 0}
};


static int std_signals[] = {SIGTERM, SIGINT, 0};
static struct crtx_signal_listener signal_list;

struct signal_map *get_signal(int signum) {
	struct signal_map *sm;
	
	sm = sigmap;
	while (sm->name) {
		if (sm->id == signum)
			return sm;
		
		sm++;
	}
	
	return 0;
}

static void signal_handler(int signum) {
	struct crtx_event *event;
	struct signal_map *smap;
	
	if (crtx_verbosity > 0) {
		smap = get_signal(signum);
		if (smap)
			DBG("received signal %s (%d)\n", smap->name, signum);
		else
			DBG("received signal %d\n", signum);
	}
	
	crtx_create_event(&event);
	
	event->description = smap->etype;
	
	crtx_event_set_raw_data(event, 'i', (int32_t) signum, sizeof(signum));
	
	crtx_add_event(signal_list.base.graph, event);
}

#ifndef WITHOUT_SIGNALFD
static char signalfd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_signal_listener *slistener;
	struct signalfd_siginfo si;
	ssize_t r;
	
	slistener = (struct crtx_signal_listener *) userdata;
	
	r = read(slistener->fd, &si, sizeof(si));
	
	if (r < 0) {
		ERROR("read from signalfd %d failed: %s\n", slistener->fd, strerror(r));
		return r;
	}
	if (r != sizeof(si)) {
		ERROR("read from signalfd %d failed: %s\n", slistener->fd, strerror(r));
		return -EBADE;
	}
	
	signal_handler(si.ssi_signo);
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *data) {
	struct crtx_signal_listener *slistener;
	sigset_t mask;
	int r;
	
	
	slistener = (struct crtx_signal_listener*) data;
	
	sigemptyset(&mask);
	r = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (r < 0) {
		ERROR("signals: clearing sigprocmask failed: %s\n", strerror(-r));
		return;
	}
	
	close(slistener->fd);
}
#endif

#ifndef WITHOUT_SELFPIPE
static char selfpipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_signal_listener *slistener;
	int32_t signal_num;
	ssize_t r;
	
	slistener = (struct crtx_signal_listener *) userdata;
	
	r = read(slistener->pipe_lstnr.fds[0], &signal_num, sizeof(signal_num));
	if (r < 0) {
		ERROR("read from signal selfpipe %d failed: %s\n", slistener->pipe_lstnr.fds[0], strerror(r));
		return r;
	}
	if (r != sizeof(signal_num)) {
		ERROR("read from signal selfpipe %d failed: %s\n", slistener->pipe_lstnr.fds[0], strerror(r));
		return -EBADE;
	}
	
	
	struct crtx_event *new_event;
	struct signal_map *smap;
	
	smap = get_signal(signal_num);
	
	crtx_create_event(&new_event);
	if (smap)
		new_event->description = smap->etype;
	else
		new_event->description = "";
	
	crtx_event_set_raw_data(new_event, 'i', signal_num, sizeof(signal_num));
	crtx_add_event(slistener->base.graph, new_event);
	
	return 0;
}

static void selfpipe_signal_handler(int signum) {
	struct signal_map *smap;
	
	if (crtx_verbosity > 0) {
		smap = get_signal(signum);
		if (smap)
			DBG("received signal %s (%d)\n", smap->name, signum);
		else
			DBG("received signal %d\n", signum);
	}
	
	write(signal_list.pipe_lstnr.fds[1], &signum, sizeof(int));
}
#endif

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_signal_listener *slistener;
	int *i;
	int r;
	
	slistener = (struct crtx_signal_listener *) listener;
	
	if (slistener->type == CRTX_SIGNAL_SIGACTION) {
		struct sigaction new_action, old_action;
		
		if (!slistener->signal_handler)
			new_action.sa_handler = signal_handler;
		else
			new_action.sa_handler = slistener->signal_handler;
		
		sigemptyset(&new_action.sa_mask);
		new_action.sa_flags = 0;
		
		i=slistener->signals;
		while (*i) {
			sigaction(*i, 0, &old_action);
			
			// only set actions for signals not previously set to ignore
			if (old_action.sa_handler != SIG_IGN) {
				struct signal_map *smap;
				
				smap = get_signal(*i);
				
				DBG("new signal handler for %s (%d)\n", smap->name, *i);
				
				signal_list.base.graph->n_descriptions++;
				signal_list.base.graph->descriptions = (char**) realloc(signal_list.base.graph->descriptions,
													    sizeof(char*) * signal_list.base.graph->n_descriptions);
				
				signal_list.base.graph->descriptions[signal_list.base.graph->n_descriptions-1] = smap->etype;
				
				sigaction(*i, &new_action, 0);
			}
			
			i++;
		}
	} else
	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
		#ifndef WITHOUT_SIGNALFD
		sigset_t mask;
		
		/*
		 * block old signal handling for signals we want to receive over the signalfd
		 */
		
		sigemptyset(&mask);
		
		i=slistener->signals;
		while (*i) {
			r = sigaddset(&mask, *i);
			if (r < 0) {
				ERROR("signals: sigaddset %d failed: %s\n", *i, strerror(-r));
				return r;
			}
			i++;
		}
		
		r = sigprocmask(SIG_BLOCK, &mask, NULL);
		if (r < 0) {
			ERROR("signals: sigprocmask failed: %s\n", strerror(-r));
			return r;
		}
		
		slistener->fd = signalfd(-1, &mask, SFD_CLOEXEC);
		if (slistener->fd < 0) {
			ERROR("signalfd failed: %s", strerror(errno));
			return errno;
		}
		
		DBG("new signalfd with fd %d\n", slistener->fd);
		
		crtx_evloop_init_listener(&slistener->base,
							 slistener->fd,
							 EVLOOP_READ,
							 0,
							 &signalfd_event_handler,
							 slistener,
							 0, 0
							);
		
		slistener->base.shutdown = &shutdown_listener;
		#else
		ERROR("libcortex was compiled without signalfd support\n");
		#endif
	} else
	if (slistener->type == CRTX_SIGNAL_SELFPIPE || slistener->type == CRTX_SIGNAL_DEFAULT) {
		slistener->pipe_lstnr.fd_event_handler = selfpipe_event_handler;
		slistener->pipe_lstnr.fd_event_handler_data = slistener;
		
		r = crtx_create_listener("pipe", &slistener->pipe_lstnr);
		if (r < 0) {
			ERROR("create_listener(pipe) failed: %s\n", strerror(-r));
			return r;
		}
		
		slistener->sub_lstnr = (struct crtx_signal_listener *) calloc(1, sizeof(struct crtx_signal_listener));
// 		slistener->sub_lstnr = &signal_list;
		slistener->sub_lstnr->signals = slistener->signals;
		slistener->sub_lstnr->type = CRTX_SIGNAL_SIGACTION;
		slistener->sub_lstnr->signal_handler = selfpipe_signal_handler;
		
		r = crtx_create_listener("signals", slistener->sub_lstnr);
		if (r < 0) {
			ERROR("create_listener(signals) failed: %s\n", strerror(-r));
			return r;
		}
		
		crtx_start_listener(&slistener->pipe_lstnr.base);
		crtx_start_listener(&slistener->sub_lstnr->base);
	} else {
		ERROR("invalid type of signal listener: %d\n", slistener->type);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_signals_listener(void *options) {
	struct crtx_signal_listener *slistener;
	
	slistener = (struct crtx_signal_listener *) options;
	
	if (!slistener->signals)
		return 0;
	
	slistener->base.start_listener = &start_listener;
	
	return &slistener->base;
}

static char sigterm_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event *newe;
	
// 	struct sigaction new_action;
// 	new_action.sa_handler = SIG_DFL;
// 	sigemptyset(&new_action.sa_mask);
// 	new_action.sa_flags = 0;
// 	sigaction(SIGTERM, &new_action, 0);
	
	crtx_create_event(&newe);
	newe->description = CRTX_EVT_SHUTDOWN;
	
	crtx_add_event(crtx_root->crtx_ctrl_graph, newe);
	
	return 1;
}

static char sigint_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event *newe;
	
	crtx_create_event(&newe);
	newe->description = CRTX_EVT_SHUTDOWN;
	
	crtx_add_event(crtx_root->crtx_ctrl_graph, newe);
	
	return 1;
}


int crtx_signals_handle_std_signals(struct crtx_signal_listener **signal_lstnr) {
	struct crtx_task *t;
	int err;
	
	
	signal_list.signals = std_signals;
	
	err = crtx_create_listener("signals", &signal_list);
	if (err < 0) {
		return err;
	}
	
	t = new_task();
	t->id = "sigterm_handler";
	t->handle = &sigterm_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGTERM)->etype;
	add_task(signal_list.base.graph, t);
	
	t = new_task();
	t->id = "sigint_handler";
	t->handle = &sigint_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGINT)->etype;
	add_task(signal_list.base.graph, t);
	
	signal(SIGPIPE, SIG_IGN);
	
	crtx_start_listener(&signal_list.base);
	
	if (signal_lstnr)
		*signal_lstnr = &signal_list;
	
	return 0;
}

void crtx_signals_init() {
	memset(&signal_list, 0, sizeof(struct crtx_signal_listener));
}

void crtx_signals_finish() {
	if (signal_list.base.graph->types) {
		free(signal_list.base.graph->types);
		signal_list.base.graph->types = 0;
	}
}

