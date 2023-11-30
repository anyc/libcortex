/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#ifndef WITHOUT_SIGNALFD
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#endif

#include "intern.h"
#include "signals.h"

#define SIGNAL(id) { id, #id, "cortex.signals." #id }
struct crtx_signal sigmap[] = {
	SIGNAL(SIGHUP),
	SIGNAL(SIGINT),
	SIGNAL(SIGPIPE),
	SIGNAL(SIGTERM),
	SIGNAL(SIGUSR1),
	SIGNAL(SIGCHLD),
	{0, 0}
};

struct sigchld_cb_data {
	sigchld_cb cb;
	void *userdata;
};

static struct crtx_signals_listener main_lstnr = { { { 0 } } };
static char main_initialized = 0;
static char main_started = 0;

static struct crtx_signal std_signals[] = {{SIGTERM}, {SIGINT}, {0}};
static struct crtx_signals_listener default_signal_lstnr = { { { 0 } } };

// static sigchld_cb *sigchld_cbs = 0;
// static unsigned int n_sigchld_cbs 0 0;
static struct crtx_dll *sigchld_cbs = 0;
struct crtx_signals_listener sigchld_lstnr;
static struct crtx_signal sigchld_signals[] = {{SIGCHLD}, {0}};

// static int lstnr_enable_signals(struct crtx_signals_listener *signal_lstnr);


struct crtx_signal *crtx_get_signal_info(int signum) {
	struct crtx_signal *sm;
	
	sm = sigmap;
	while (sm->name) {
		if (sm->signum == signum)
			return sm;
		
		sm++;
	}
	
	return 0;
}

// classic signal handler for CRTX_SIGNAL_SIGACTION listeners
static void default_signal_handler(int signum) {
	struct crtx_event *event;
	struct crtx_signal *smap;
	
	
	smap = crtx_get_signal_info(signum);
	if (crtx_verbosity > 0) {
		if (smap)
			CRTX_DBG("received signal %s (%d)\n", smap->name, signum);
		else
			CRTX_DBG("received signal %d\n", signum);
	}
	
	crtx_create_event(&event);
	
	event->description = smap->etype;
	
	crtx_event_set_raw_data(event, 'i', (int32_t) signum, sizeof(signum));
	
	crtx_add_event(default_signal_lstnr.base.graph, event);
}

#ifndef WITHOUT_SIGNALFD
static char signalfd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_signals_listener *slistener;
	struct signalfd_siginfo si;
	ssize_t r;
	
	slistener = (struct crtx_signals_listener *) userdata;
	
	r = read(slistener->fd, &si, sizeof(si));
	
	if (r < 0) {
		CRTX_ERROR("read from signalfd %d failed: %s\n", slistener->fd, strerror(r));
		return r;
	}
	if (r != sizeof(si)) {
		CRTX_ERROR("read from signalfd %d failed: %s\n", slistener->fd, strerror(r));
		return -EBADE;
	}
	
	default_signal_handler(si.ssi_signo);
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *data) {
	struct crtx_signals_listener *slistener;
	sigset_t mask;
	int r;
	
	
	slistener = (struct crtx_signals_listener*) data;
	
	sigemptyset(&mask);
	r = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (r < 0) {
		CRTX_ERROR("signals: clearing sigprocmask failed: %s\n", strerror(-r));
		return;
	}
	
	if (slistener->fd >= 0)
		close(slistener->fd);
}
#endif

#ifndef WITHOUT_SELFPIPE
static char selfpipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_signals_listener *slistener;
	int32_t signal_num;
	ssize_t r;
	struct crtx_dll *lit;
	
	slistener = (struct crtx_signals_listener *) userdata;
	
	r = read(slistener->pipe_lstnr.fds[CRTX_READ_END], &signal_num, sizeof(signal_num));
	if (r < 0) {
		CRTX_ERROR("read from signal selfpipe %d failed: %s\n", slistener->pipe_lstnr.fds[CRTX_READ_END], strerror(r));
		return r;
	}
	if (r != sizeof(signal_num)) {
		CRTX_ERROR("read invalid data from signal selfpipe %d: %zd != %zd\n", slistener->pipe_lstnr.fds[CRTX_READ_END], r, sizeof(signal_num));
		return -EBADE;
	}
	
	
	struct crtx_event *new_event;
	struct crtx_signal *smap;
	int i;
	
	smap = crtx_get_signal_info(signal_num);
	
	for (i=0; i < slistener->n_signals; i++) {
		if (slistener->signals[i].signum == signal_num) {
			crtx_create_event(&new_event);
			
			if (smap)
				new_event->description = smap->etype;
			else
				new_event->description = "";
			
			crtx_event_set_raw_data(new_event, 'i', signal_num, sizeof(signal_num), 0);
			
			for (lit=slistener->signals[i].lstnrs; lit; lit=lit->next) {
				crtx_add_event(((struct crtx_listener_base*)lit->data)->graph, new_event);
			}
			
			break;
		}
	}
	
	return 0;
}

// classic signal handler for CRTX_SIGNAL_SELFPIPE listeners
static void selfpipe_signal_handler(int signum) {
	struct crtx_signal *smap;
	int rv;
	
	
	if (crtx_verbosity > 0) {
		smap = crtx_get_signal_info(signum);
		if (smap)
			CRTX_DBG("received signal %s (%d)\n", smap->name, signum);
		else
			CRTX_DBG("received signal %d\n", signum);
	}
	
	rv = write(main_lstnr.pipe_lstnr.fds[CRTX_WRITE_END], &signum, sizeof(int));
	if (rv != sizeof(int)) {
		CRTX_ERROR("write to selfpipe %d (signal %d) failed: %d != %zu\n", main_lstnr.pipe_lstnr.fds[CRTX_WRITE_END], signum, rv, sizeof(int));
		return;
	}
}
#endif

// static int update_signals(struct crtx_signals_listener *slistener) {
// 	int r;
// 	
// 	if (slistener->type == CRTX_SIGNAL_SIGACTION) {
// 		CRTX_DBG("signal mode sigaction\n");
// 		
// 		// we have no fd or thread to monitor
// 		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
// 	} else
// 	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
// 		CRTX_DBG("signal mode signalfd\n");
// 	} else
// 	if (slistener->type == CRTX_SIGNAL_SELFPIPE || slistener->type == CRTX_SIGNAL_DEFAULT) {
// 		/*
// 		 * in this mode, we install a regular sigaction signal handler but this handler
// 		 * only writes into our "self-pipe" to call the actual signal handler through our
// 		 * event loop
// 		 */
// 		
// 		CRTX_DBG("signal mode selfpipe\n");
// 		
// 		// this lstnr is just a container for the sigaction and pipe listeners
// 		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
// 		
// 		slistener->pipe_lstnr.fd_event_handler = selfpipe_event_handler;
// 		slistener->pipe_lstnr.fd_event_handler_data = slistener;
// 		
// 		r = crtx_setup_listener("pipe", &slistener->pipe_lstnr);
// 		if (r < 0) {
// 			CRTX_ERROR("create_listener(pipe) failed: %s\n", strerror(-r));
// 			return r;
// 		}
// 		
// 		slistener->sub_lstnr = (struct crtx_signals_listener *) calloc(1, sizeof(struct crtx_signals_listener));
// 		slistener->sub_lstnr->signals = slistener->signals;
// 		slistener->sub_lstnr->type = CRTX_SIGNAL_SIGACTION;
// 		slistener->sub_lstnr->signal_handler = selfpipe_signal_handler;
// 		
// 		r = crtx_setup_listener("signals", slistener->sub_lstnr);
// 		if (r < 0) {
// 			CRTX_ERROR("create_listener(signals) failed: %s\n", strerror(-r));
// 			return r;
// 		}
// 		
// 		crtx_start_listener(&slistener->pipe_lstnr.base);
// 		crtx_start_listener(&slistener->sub_lstnr->base);
// 	} else {
// 		CRTX_ERROR("invalid type of signal listener: %d\n", slistener->type);
// 	}
// 	
// 	return lstnr_enable_signals(slistener);
// }

static int update_signals(struct crtx_signals_listener *signal_lstnr) {
	if (!signal_lstnr->signals) {
		CRTX_DBG("no signals, will not enable handler\n");
		return 0;
	}
	
	if (signal_lstnr->type == CRTX_SIGNAL_SIGACTION) {
		struct sigaction old_action;
		struct crtx_signal *sig;
		
		sig=signal_lstnr->signals;
		while (sig->signum) {
			sigaction(sig->signum, 0, &old_action);
			
			// only set actions for signals not previously set to ignore
			if (old_action.sa_handler != SIG_IGN) {
				struct crtx_signal *smap;
				
				smap = crtx_get_signal_info(sig->signum);
				
				CRTX_DBG("new signal handler for %s (%d)\n", smap?smap->name:"", sig->signum);
				
				sigaction(sig->signum, &sig->sa, 0);
			}
			
			sig++;
		}
	} else 
	if (signal_lstnr->type == CRTX_SIGNAL_SIGNALFD) {
		#ifndef WITHOUT_SIGNALFDD
		sigset_t mask;
		int r, flags;
		struct crtx_signal *sig;
		
		
		CRTX_DBG("signal mode signalfd\n");
		
		/*
		 * block old signal handling for signals we want to receive over the signalfd
		 */
		
		sigemptyset(&mask);
		
		sig=signal_lstnr->signals;
		while (sig->signum) {
			r = sigaddset(&mask, sig->signum);
			if (r < 0) {
				CRTX_ERROR("signals: sigaddset %d failed: %s\n", sig->signum, strerror(-r));
				return r;
			}
			sig++;
		}
		
		r = sigprocmask(SIG_BLOCK, &mask, NULL);
		if (r < 0) {
			CRTX_ERROR("signals: sigprocmask failed: %s\n", strerror(-r));
			return r;
		}
		
		if (crtx_root->global_fd_flags & FD_CLOEXEC)
			flags = SFD_CLOEXEC;
		else
			flags = 0;
		
		signal_lstnr->fd = signalfd(signal_lstnr->fd, &mask, flags);
		if (signal_lstnr->fd < 0) {
			CRTX_ERROR("signalfd failed: %s", strerror(errno));
			return errno;
		}
		
		CRTX_DBG("new signalfd with fd %d\n", signal_lstnr->fd);
		
		crtx_evloop_init_listener(&signal_lstnr->base,
							signal_lstnr->fd,
							CRTX_EVLOOP_READ,
							0,
							&signalfd_event_handler,
							signal_lstnr,
							0, 0
						);
		
		signal_lstnr->base.shutdown = &shutdown_listener;
		#else
		CRTX_ERROR("libcortex was compiled without signalfd support\n");
		#endif
	} else
	if (signal_lstnr->type == CRTX_SIGNAL_SELFPIPE || signal_lstnr->type == CRTX_SIGNAL_DEFAULT) {
		signal_lstnr->sub_lstnr->signals = signal_lstnr->signals;
		return update_signals(signal_lstnr->sub_lstnr);
	}
	
	return 0;
}

static char start_sub_listener(struct crtx_listener_base *listener) {
	int i;
	struct crtx_signals_listener *signal_lstnr;
	struct crtx_signal *sig;
	char found, changed;
	
	
	signal_lstnr = (struct crtx_signals_listener*) listener;
	
	signal_lstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	if (!main_started) {
		crtx_start_listener(&main_lstnr.base);
	}
	
	changed = 0;
	for (sig=signal_lstnr->signals; sig && sig->signum; sig++) {
		found = 0;
		
		for (i=0; i < main_lstnr.n_signals; i++) {
			if (main_lstnr.signals[i].signum == sig->signum) {
				crtx_dll_append_new(&main_lstnr.signals[i].lstnrs, signal_lstnr);
				
				if (!main_lstnr.signal_handler)
					main_lstnr.signals[i].sa.sa_handler = &default_signal_handler;
				else
					main_lstnr.signals[i].sa.sa_handler = main_lstnr.signal_handler;
				
				found = 1;
				break;
			}
		}
		
		if (!found) {
			struct crtx_signal *main_sig;
		
			main_lstnr.n_signals += 1;
			main_lstnr.signals = (struct crtx_signal*) realloc(main_lstnr.signals, sizeof(struct crtx_signal)*(main_lstnr.n_signals + 1));
			// main_lstnr.signals[main_lstnr.n_signals-1] = sig;
			main_lstnr.signals[main_lstnr.n_signals].signum = 0;
			
			main_sig = &main_lstnr.signals[main_lstnr.n_signals-1];
			memset(main_sig, 0, sizeof(struct crtx_signal));
			
			main_sig->signum = sig->signum;
			
			if (!main_lstnr.signal_handler)
				main_sig->sa.sa_handler = &default_signal_handler;
			else
				main_sig->sa.sa_handler = main_lstnr.signal_handler;
			
			sigemptyset(&main_sig->sa.sa_mask);
			main_sig->sa.sa_flags = 0;
			
			crtx_dll_append_new(&main_sig->lstnrs, signal_lstnr);
			
			changed = 1;
		}
	}
	
	if (changed)
		update_signals(&main_lstnr);
	
	return 0;
}

static char stop_sub_listener(struct crtx_listener_base *listener) {
	struct crtx_signals_listener *signal_lstnr;
	int i;
	char skip, changed;
	struct crtx_signal *sig;
	
	
	signal_lstnr = (struct crtx_signals_listener*) listener;
	
	changed = 0;
	for (sig=signal_lstnr->signals; sig && sig->signum; sig++) {
		skip = 0;
		
		for (i=0; i < main_lstnr.n_signals; i++) {
			if (main_lstnr.signals[i].signum == sig->signum) {
				crtx_dll_unlink_data(&main_lstnr.signals[i].lstnrs, signal_lstnr);
				
				if (main_lstnr.signals[i].lstnrs == 0) {
					// main_lstnr.signals[i].signum = -1;
					main_lstnr.signals[i].sa.sa_handler = SIG_DFL;
					changed = 1;
				}
				
				break;
			}
		}
		
		if (skip)
			continue;
	}
	
	if (changed)
		update_signals(&main_lstnr);
	
	return 0;
}

static void free_sub_listener(struct crtx_listener_base *listener, void *userdata) {
	free(listener);
}

static char start_main_listener(struct crtx_listener_base *listener) {
	struct crtx_signals_listener *slistener;
	int r;
	
	slistener = (struct crtx_signals_listener *) listener;
	
	if (slistener->type == CRTX_SIGNAL_SIGACTION) {
		CRTX_DBG("signal mode sigaction\n");
		
		// we have no fd or thread to monitor
		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
	} else
	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
		// pass
	} else
	if (slistener->type == CRTX_SIGNAL_SELFPIPE || slistener->type == CRTX_SIGNAL_DEFAULT) {
		/*
		 * In this mode, we install a regular sigaction signal handler but this handler
		 * only writes into our "self-pipe". So we can call the actual signal handler
		 * through our event loop
		 */
		
		CRTX_DBG("signal mode selfpipe\n");
		
		// this lstnr is just a container for the sigaction and pipe listeners
		slistener->base.mode = CRTX_NO_PROCESSING_MODE;
		
		slistener->pipe_lstnr.fd_event_handler = selfpipe_event_handler;
		slistener->pipe_lstnr.fd_event_handler_data = slistener;
		
		r = crtx_setup_listener("pipe", &slistener->pipe_lstnr);
		if (r < 0) {
			CRTX_ERROR("create_listener(pipe) failed: %s\n", strerror(-r));
			return r;
		}
		
		slistener->sub_lstnr = (struct crtx_signals_listener *) calloc(1, sizeof(struct crtx_signals_listener));
// 		slistener->sub_lstnr = &default_signal_lstnr;
		slistener->sub_lstnr->signals = slistener->signals;
		slistener->sub_lstnr->type = CRTX_SIGNAL_SIGACTION;
		// slistener->sub_lstnr->signal_handler = selfpipe_signal_handler;
		slistener->signal_handler = selfpipe_signal_handler;
		slistener->sub_lstnr->base.free_cb = &free_sub_listener;
		
		r = crtx_setup_listener("signals", slistener->sub_lstnr);
		if (r < 0) {
			CRTX_ERROR("create_listener(signals) failed: %s\n", strerror(-r));
			return r;
		}
		
		crtx_start_listener(&slistener->pipe_lstnr.base);
		crtx_start_listener(&slistener->sub_lstnr->base);
	} else {
		CRTX_ERROR("invalid type of signal listener: %d\n", slistener->type);
	}
	
	return update_signals(slistener);
}

static void shutdown_main_listener(struct crtx_listener_base *listener) {
	main_initialized = 0;
	// TODO stop_listener
	main_started = 0;
}

struct crtx_listener_base *crtx_setup_signals_listener(void *options) {
	struct crtx_signals_listener *slistener;
	
	
	slistener = (struct crtx_signals_listener *) options;
	
	if (slistener->type != CRTX_SIGNAL_SIGACTION) {
		if (!main_initialized) {
			int err;
			
			main_initialized = 1;
			
			err = crtx_setup_listener("signals", &main_lstnr);
			if (err < 0) {
				return 0;
			}
			
			main_lstnr.base.start_listener = &start_main_listener;
			main_lstnr.base.shutdown = &shutdown_main_listener;
		}
		
		// We only have one listener that can receive signals. If multiple
		// listeners shall be used, these listeners receive their signals
		// from the main listener.
		if (options != &main_lstnr) {
			slistener->base.start_listener = &start_sub_listener;
			slistener->base.stop_listener = &stop_sub_listener;
		}
	} else {
		slistener->base.start_listener = &start_main_listener;
	}
	
	if (slistener->type == CRTX_SIGNAL_SIGNALFD) {
		slistener->fd = -1;
	}
	
	return &slistener->base;
}

static char sigchild_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	// 	unsigned int i;
	struct crtx_dll *it, *itn;
	struct sigchld_cb_data *data;
	
	while (1) {
		int status;
		
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0) {
			break;
		}
		
		if (WIFEXITED(status)) {
			CRTX_DBG("child %u finished with exit value %d\n", pid, WEXITSTATUS(status));
		} else
		if (WIFSIGNALED(status)) {
			CRTX_DBG("child %u got terminated by signal %d (%s coredump)\n",
				pid, WTERMSIG(status), WCOREDUMP(status)?"with":"without"
				);
		}
		
		for (it=sigchld_cbs; it; it=itn) {
			itn = it->next;
			
			data = (struct sigchld_cb_data *) it->data;
			
			data->cb(pid, status, data->userdata);
		}
	}
	
	return 0;
}

void * crtx_signals_add_child_handler(sigchld_cb cb, void *userdata) {
// 	n_sigchld_cbs += 1;
// 	sigchld_cbs = (sigchld_cb*) realloc(sigchld_cbs, sizeof(sigchld_cb)*n_sigchld_cbs);
// 	sigchld_cbs[n_sigchld_cbs-1] = cb;
	
	if (sigchld_cbs == 0) {
		int ret;
		
		sigchld_lstnr.signals = sigchld_signals;
		
		ret = crtx_setup_listener("signals", &sigchld_lstnr);
		if (ret) {
			CRTX_ERROR("create_listener(signal) failed\n");
			return 0;
		}
		
		crtx_create_task(sigchld_lstnr.base.graph, 0, "sigchld_handler", &sigchild_event_handler, 0);
		
		ret = crtx_start_listener(&sigchld_lstnr.base);
		if (ret) {
			CRTX_ERROR("starting signal listener failed\n");
			return 0;
		}
	}
	
	struct sigchld_cb_data *data;
	
	data = (struct sigchld_cb_data*) malloc(sizeof(struct sigchld_cb_data));
	data->cb = cb;
	data->userdata = userdata;
	
	crtx_dll_append_new(&sigchld_cbs, data);
	
	return data;
}

int crtx_signals_rem_child_handler(void *sigchld_cb) {
	struct crtx_dll *item;
	
	item = crtx_dll_unlink_data(&sigchld_cbs, sigchld_cb);
	if (item) {
		free(sigchld_cb);
		free(item);
	} else {
		CRTX_ERROR("callback %p not found in list of sigchld signal handlers\n", sigchld_cb);
		return 1;
	}
	
	return 0;
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


int crtx_signals_handle_std_signals(struct crtx_signals_listener **signal_lstnr) {
	struct crtx_task *t;
	int err;
	
	
	default_signal_lstnr.signals = std_signals;
	
	err = crtx_setup_listener("signals", &default_signal_lstnr);
	if (err < 0) {
		return err;
	}
	
	t = new_task();
	t->id = "sigterm_handler";
	t->handle = &sigterm_handler;
	t->userdata = 0;
	t->event_type_match = crtx_get_signal_info(SIGTERM)->etype;
	add_task(default_signal_lstnr.base.graph, t);
	
	t = new_task();
	t->id = "sigint_handler";
	t->handle = &sigint_handler;
	t->userdata = 0;
	t->event_type_match = crtx_get_signal_info(SIGINT)->etype;
	add_task(default_signal_lstnr.base.graph, t);
	
	signal(SIGPIPE, SIG_IGN);
	
	crtx_start_listener(&default_signal_lstnr.base);
	
	if (signal_lstnr)
		*signal_lstnr = &default_signal_lstnr;
	
	return 0;
}

CRTX_DEFINE_ALLOC_FUNCTION(signals)

void crtx_signals_init() {
	memset(&default_signal_lstnr, 0, sizeof(struct crtx_signals_listener));
	memset(&main_lstnr, 0, sizeof(struct crtx_signals_listener));
}

void crtx_signals_finish() {
	if (main_initialized) {
		crtx_shutdown_listener(&main_lstnr.base);
	}
	
	if (main_lstnr.signals)
		free(main_lstnr.signals);
	
// 	if (default_signal_lstnr.base.graph && default_signal_lstnr.base.graph->types) {
// 		free(default_signal_lstnr.base.graph->types);
// 		default_signal_lstnr.base.graph->types = 0;
// 	}
// 	
// 	if (default_signal_lstnr.base.graph && default_signal_lstnr.base.graph->descriptions) {
// 		free(default_signal_lstnr.base.graph->descriptions);
// 		default_signal_lstnr.base.graph->descriptions = 0;
// 	}
}

