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

#define SIGNAL(id) { id, #id, "cortex.signals." #id, 0 }
struct signal_map {
	int id;
	char *name;
	char *etype;
	struct crtx_listener_base *lstnr;
} sigmap[] = {
	SIGNAL(SIGHUP),
	SIGNAL(SIGINT),
	SIGNAL(SIGPIPE),
	SIGNAL(SIGTERM),
	SIGNAL(SIGUSR1),
// 	SIGNAL(SIGCHLD),
	{0, 0}
};

static struct signal_map *signal_table = 0;
static unsigned int n_signal_table_rows = 0;

static int std_signals[] = {SIGTERM, SIGINT, 0};
static struct crtx_signal_listener default_signal_lstnr = { 0 };
static struct crtx_signal_listener main_lstnr = { 0 };
static char main_initialized = 0;
static char main_started = 0;

static int lstnr_enable_signals(struct crtx_signal_listener *signal_lstnr);



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
	
	crtx_add_event(default_signal_lstnr.base.graph, event);
}

// static char start_sigaction_signal_handler(struct crtx_listener_base *listener) {
// 	// no-op function to silence the event loop warnings as we do not use the event loop
// 	
// 	return 0;
// }

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
	
#if 0
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
#endif
	
	struct crtx_event *new_event;
	struct signal_map *smap;
	int i;
	
	
	smap = get_signal(signal_num);
	
	for (i=0; i < n_signal_table_rows; i++) {
		if (signal_table[i].id == signal_num) {
			crtx_create_event(&new_event);
			
			if (smap)
				new_event->description = smap->etype;
			else
				new_event->description = "";
			
			crtx_event_set_raw_data(new_event, 'i', signal_num, sizeof(signal_num));
			crtx_add_event(signal_table[i].lstnr->graph, new_event);
			
			break;
		}
	}
	
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
	CRTX_DBG("sigpipe %d\n", main_lstnr.pipe_lstnr.fds[1]);
// 	write(default_signal_lstnr.pipe_lstnr.fds[1], &signum, sizeof(int));
	write(main_lstnr.pipe_lstnr.fds[1], &signum, sizeof(int));
}
#endif

static char start_main_listener(struct crtx_listener_base *listener) {
	struct crtx_signal_listener *slistener;
// 	int *i;
	int r;
	
	slistener = (struct crtx_signal_listener *) listener;
	
	if (slistener->type == CRTX_SIGNAL_SIGACTION) {
// 		struct sigaction new_action, old_action;
// 		
// 		
		DBG("signal mode sigaction\n");
// 		
// 		if (!slistener->signal_handler)
// 			new_action.sa_handler = signal_handler;
// 		else
// 			new_action.sa_handler = slistener->signal_handler;
		
		// we have no fd or thread to monitor
		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
		
// 		sigemptyset(&new_action.sa_mask);
// 		new_action.sa_flags = 0;
// 		
// 		i=slistener->signals;
// 		while (*i) {
// 			sigaction(*i, 0, &old_action);
// 			
// 			// only set actions for signals not previously set to ignore
// 			if (old_action.sa_handler != SIG_IGN) {
// 				struct signal_map *smap;
// 				
// 				smap = get_signal(*i);
// 				
// 				DBG("new signal handler for %s (%d)\n", smap?smap->name:"", *i);
// 				
// 				if (smap) {
// 					default_signal_lstnr.base.graph->n_descriptions++;
// 					default_signal_lstnr.base.graph->descriptions = (char**) realloc(default_signal_lstnr.base.graph->descriptions,
// 															sizeof(char*) * default_signal_lstnr.base.graph->n_descriptions);
// 					
// 					default_signal_lstnr.base.graph->descriptions[default_signal_lstnr.base.graph->n_descriptions-1] = smap->etype;
// 				}
// 				
// 				sigaction(*i, &new_action, 0);
// 			}
// 			
// 			i++;
// 		}
	} else
	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
// 		#ifndef WITHOUT_SIGNALFD
// 		sigset_t mask;
		
// 		DBG("signal mode signalfd\n");
// 		
// 		/*
// 		 * block old signal handling for signals we want to receive over the signalfd
// 		 */
// 		
// 		sigemptyset(&mask);
// 		
// 		i=slistener->signals;
// 		while (*i) {
// 			r = sigaddset(&mask, *i);
// 			if (r < 0) {
// 				ERROR("signals: sigaddset %d failed: %s\n", *i, strerror(-r));
// 				return r;
// 			}
// 			i++;
// 		}
// 		
// 		r = sigprocmask(SIG_BLOCK, &mask, NULL);
// 		if (r < 0) {
// 			ERROR("signals: sigprocmask failed: %s\n", strerror(-r));
// 			return r;
// 		}
	/*	
		slistener->fd = signalfd(slistener->fd, &mask, SFD_CLOEXEC);
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
		#endif*/
	} else
	if (slistener->type == CRTX_SIGNAL_SELFPIPE || slistener->type == CRTX_SIGNAL_DEFAULT) {
		/*
		 * in this mode, we install a regular sigaction signal handler but this handler
		 * only writes into our "self-pipe" to call the actual signal handler through our
		 * event loop
		 */
		
		DBG("signal mode selfpipe\n");
		
		// this lstnr is just a container for the sigaction and pipe listeners
		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
		
		slistener->pipe_lstnr.fd_event_handler = selfpipe_event_handler;
		slistener->pipe_lstnr.fd_event_handler_data = slistener;
		
		r = crtx_create_listener("pipe", &slistener->pipe_lstnr);
		if (r < 0) {
			ERROR("create_listener(pipe) failed: %s\n", strerror(-r));
			return r;
		}
		
		slistener->sub_lstnr = (struct crtx_signal_listener *) calloc(1, sizeof(struct crtx_signal_listener));
// 		slistener->sub_lstnr = &default_signal_lstnr;
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
	
	return lstnr_enable_signals(slistener);
	
// 	return 0;
}

static int lstnr_enable_signals(struct crtx_signal_listener *signal_lstnr) {
	if (!signal_lstnr->signals) {
		DBG("no signals, will not enable handler\n");
		return 0;
	}
	
	if (signal_lstnr->type == CRTX_SIGNAL_SIGACTION) {
		struct sigaction new_action, old_action;
		int *i;
		
		
		if (!signal_lstnr->signal_handler)
			new_action.sa_handler = signal_handler;
		else
			new_action.sa_handler = signal_lstnr->signal_handler;
		
		sigemptyset(&new_action.sa_mask);
		new_action.sa_flags = 0;
		
		i=signal_lstnr->signals;
		while (*i) {
			sigaction(*i, 0, &old_action);
			
			// only set actions for signals not previously set to ignore
			if (old_action.sa_handler != SIG_IGN) {
				struct signal_map *smap;
				
				smap = get_signal(*i);
				
				DBG("new signal handler for %s (%d)\n", smap?smap->name:"", *i);
				
				if (smap) {
					default_signal_lstnr.base.graph->n_descriptions++;
					default_signal_lstnr.base.graph->descriptions = (char**) realloc(default_signal_lstnr.base.graph->descriptions,
																					 sizeof(char*) * default_signal_lstnr.base.graph->n_descriptions);
					
					default_signal_lstnr.base.graph->descriptions[default_signal_lstnr.base.graph->n_descriptions-1] = smap->etype;
				}
				
				sigaction(*i, &new_action, 0);
			}
			
			i++;
		}
	} else 
	if (signal_lstnr->type == CRTX_SIGNAL_SIGNALFD) {
		#ifndef WITHOUT_SIGNALFDD
		sigset_t mask;
		int r;
		int *i;
		
		
		DBG("signal mode signalfd\n");
		
		/*
		 * block old signal handling for signals we want to receive over the signalfd
		 */
		
		sigemptyset(&mask);
		
		i=signal_lstnr->signals;
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
		
		signal_lstnr->fd = signalfd(signal_lstnr->fd, &mask, SFD_CLOEXEC);
		if (signal_lstnr->fd < 0) {
			ERROR("signalfd failed: %s", strerror(errno));
			return errno;
		}
		
		DBG("new signalfd with fd %d\n", signal_lstnr->fd);
		
		crtx_evloop_init_listener(&signal_lstnr->base,
								  signal_lstnr->fd,
							EVLOOP_READ,
							0,
							&signalfd_event_handler,
							signal_lstnr,
							0, 0
		);
		
		signal_lstnr->base.shutdown = &shutdown_listener;
		#else
		ERROR("libcortex was compiled without signalfd support\n");
		#endif
	} else
	if (signal_lstnr->type == CRTX_SIGNAL_SELFPIPE || signal_lstnr->type == CRTX_SIGNAL_DEFAULT) {
		signal_lstnr->sub_lstnr->signals = signal_lstnr->signals;
		return lstnr_enable_signals(signal_lstnr->sub_lstnr);
	}
	
	return 0;
}

static char start_sub_listener(struct crtx_listener_base *listener) {
	int i, *s;
	struct crtx_signal_listener *signal_lstnr;
	char found, changed;
	
	signal_lstnr = (struct crtx_signal_listener*) listener;
	
	signal_lstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	if (!main_started) {
		crtx_start_listener(&main_lstnr.base);
	}
	
	changed = 0;
	for (s=signal_lstnr->signals; s && *s; s++) {
		found = 0;
		
		for (i=0; i < n_signal_table_rows; i++) {
			if (signal_table[i].id == *s) {
				signal_table[i].lstnr = listener;
				found = 1;
				break;
			}
		}
		
		if (!found) {
			n_signal_table_rows += 1;
			signal_table = (struct signal_map*) realloc(signal_table, sizeof(struct signal_map)*n_signal_table_rows);
			signal_table[n_signal_table_rows-1].id = *s;
			signal_table[n_signal_table_rows-1].lstnr = listener;
			
			main_lstnr.signals = (int*) realloc(main_lstnr.signals, sizeof(int)*(n_signal_table_rows + 1));
			main_lstnr.signals[n_signal_table_rows-1] = *s;
			main_lstnr.signals[n_signal_table_rows] = 0;
			
			changed = 1;
		}
	}
	
	if (changed)
		lstnr_enable_signals(&main_lstnr);
	
	return 0;
}

struct crtx_listener_base *crtx_new_signals_listener(void *options) {
	struct crtx_signal_listener *slistener;
	
	slistener = (struct crtx_signal_listener *) options;
	
// 	if (options != &main_lstnr && !slistener->signals) {
// 		ERROR("no signals\n");
// 		return 0;
// 	}
	
	if (slistener->type != CRTX_SIGNAL_SIGACTION) {
		if (!main_initialized) {
			int err;
			
			main_initialized = 1;
			
			err = crtx_create_listener("signals", &main_lstnr);
			if (err < 0) {
				return 0;
			}
			
			main_lstnr.base.start_listener = &start_main_listener;
		}
		
		if (options != &main_lstnr) {
			slistener->base.start_listener = &start_sub_listener;
		}
	} else {
		slistener->base.start_listener = &start_main_listener;
	}
	
	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
		slistener->fd = -1;
	}
	
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
	
	
	default_signal_lstnr.signals = std_signals;
	
	err = crtx_create_listener("signals", &default_signal_lstnr);
	if (err < 0) {
		return err;
	}
	
	t = new_task();
	t->id = "sigterm_handler";
	t->handle = &sigterm_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGTERM)->etype;
	add_task(default_signal_lstnr.base.graph, t);
	
	t = new_task();
	t->id = "sigint_handler";
	t->handle = &sigint_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGINT)->etype;
	add_task(default_signal_lstnr.base.graph, t);
	
	signal(SIGPIPE, SIG_IGN);
	
	crtx_start_listener(&default_signal_lstnr.base);
	
	if (signal_lstnr)
		*signal_lstnr = &default_signal_lstnr;
	
	return 0;
}

void crtx_signals_init() {
	memset(&default_signal_lstnr, 0, sizeof(struct crtx_signal_listener));
}

void crtx_signals_finish() {
	if (main_initialized) {
		crtx_free_listener(&main_lstnr.base);
	}
	
	if (default_signal_lstnr.base.graph->types) {
		free(default_signal_lstnr.base.graph->types);
		default_signal_lstnr.base.graph->types = 0;
	}
}

