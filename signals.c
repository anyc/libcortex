
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

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
struct crtx_signal_listener signal_list;
struct crtx_listener_base *sl_base;

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
	
	smap = get_signal(signum);
	
	DBG("received signal %s (%d)\n", smap->name, signum);
	
	event = create_event(smap->etype, 0, 0);
	
	add_event(signal_list.parent.graph, event);
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_signal_listener *slistener;
	struct sigaction new_action, old_action;
	int *i;
	
	slistener = (struct crtx_signal_listener *) listener;
	
	
	new_action.sa_handler = signal_handler;
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
			
			signal_list.parent.graph->n_types++;
			signal_list.parent.graph->types = (char**) realloc(signal_list.parent.graph->types,
													 sizeof(char*) * signal_list.parent.graph->n_types);
			
			signal_list.parent.graph->types[signal_list.parent.graph->n_types-1] = smap->etype;
			
			sigaction(*i, &new_action, 0);
		}
		
		i++;
	}
	
	return 1;
}

struct crtx_listener_base *crtx_new_signals_listener(void *options) {
	struct crtx_signal_listener *slistener;
	
	slistener = (struct crtx_signal_listener *) options;
	
	if (!slistener->signals)
		return 0;
	
	signal_list.parent.thread = 0;
	signal_list.parent.start_listener = &start_listener;
	
	return &signal_list.parent;
}

static char sigterm_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event *newe;
	
// 	struct sigaction new_action;
// 	new_action.sa_handler = SIG_DFL;
// 	sigemptyset(&new_action.sa_mask);
// 	new_action.sa_flags = 0;
// 	sigaction(SIGTERM, &new_action, 0);
	
	newe = create_event(CRTX_EVT_SHUTDOWN, 0, 0);
	
	add_event(crtx_root->crtx_ctrl_graph, newe);
	
	return 1;
}

static char sigint_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event *newe;
	
	newe = create_event(CRTX_EVT_SHUTDOWN, 0, 0);
	
	add_event(crtx_root->crtx_ctrl_graph, newe);
	
	return 1;
}


void crtx_handle_std_signals() {
	struct crtx_task *t;
	
	signal_list.signals = std_signals;
	
	sl_base = create_listener("signals", &signal_list);
	if (!sl_base) {
		printf("cannot create crtx_signal_listener\n");
		return;
	}
	
	t = new_task();
	t->id = "sigterm_handler";
	t->handle = &sigterm_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGTERM)->etype;
	add_task(sl_base->graph, t);
	
	t = new_task();
	t->id = "sigint_handler";
	t->handle = &sigint_handler;
	t->userdata = 0;
	t->event_type_match = get_signal(SIGINT)->etype;
	add_task(sl_base->graph, t);
	
	signal(SIGPIPE, SIG_IGN);
	
	crtx_start_listener(sl_base);
}

void crtx_signals_init() {
	new_eventgraph(&signal_list.parent.graph, "cortex.signals", 0);
}

void crtx_signals_finish() {
	if (signal_list.parent.graph->types) {
		free(signal_list.parent.graph->types);
		signal_list.parent.graph->types = 0;
	}
}

