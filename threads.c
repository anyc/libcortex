
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "core.h"
#include "threads.h"

#ifndef CRTX_INIT_N_THREADS
#define CRTX_INIT_N_THREADS 2
#endif

struct crtx_thread *pool = 0;
MUTEX_TYPE pool_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int n_threads = 0;


void init_signal(struct crtx_signal *signal) {
	int ret;
	
	ret = pthread_mutex_init(&signal->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&signal->cond, NULL); ASSERT(ret >= 0);
	
	signal->local_condition = 0;
	signal->condition = &signal->local_condition;
}

struct crtx_signal *new_signal() {
	struct crtx_signal *signal;
	
	signal = (struct crtx_signal*) calloc(1, sizeof(struct crtx_signal));
	
	init_signal(signal);
	
	return signal;
}

void reset_signal(struct crtx_signal *signal) {
	*signal->condition = 0;
}

void wait_on_signal(struct crtx_signal *signal) {
	LOCK(signal->mutex);
	
	while (!*signal->condition)
		pthread_cond_wait(&signal->cond, &signal->mutex);
	
	UNLOCK(signal->mutex);
}

void send_signal(struct crtx_signal *s, char brdcst) {
	LOCK(s->mutex);
	
	if (!*s->condition)
		*s->condition = 1;
	
	if (brdcst) {
		SIGNAL_BCAST(s->cond);
	} else {
		SIGNAL_SINGLE(s->cond);
	}
	
	UNLOCK(s->mutex);
}

void free_signal(struct crtx_signal *s) {
	LOCK(s->mutex);
	UNLOCK(s->mutex);
	
	pthread_mutex_destroy(&s->mutex);
	pthread_cond_destroy(&s->cond);
	
// 	free(s);
}

void * thread_main(void *data) {
	struct crtx_thread *thread = (struct crtx_thread*) data;
	
	printf("tstart %p\n", data);
	
	while (!thread->stop) {
		// we wait until someone has work for us
		wait_on_signal(&thread->start);
		if (thread->stop)
			break;
		
		// execute
		thread->fct(thread->fct_data);
		
		if (thread->on_finish)
			thread->on_finish(thread, thread->on_finish_data);
		
		LOCK(pool_mutex);
		reset_signal(&thread->start);
		thread->in_use = 0;
		thread->fct = 0;
		thread->fct_data = 0;
		thread->on_finish = 0;
		thread->on_finish_data = 0;
		UNLOCK(pool_mutex);
	}
	
	printf("tstop %p\n", data);
	
	return 0;
}

// void spawn_thread(struct crtx_graph *graph) {
// 	graph->n_consumers++;
// 	graph->consumers = (struct crtx_thread*) realloc(graph->consumers, sizeof(struct crtx_thread)*graph->n_consumers); ASSERT(graph->consumers);
// 	
// 	graph->consumers[graph->n_consumers-1].stop = 0;
// 	graph->consumers[graph->n_consumers-1].graph = graph;
// 	pthread_create(&graph->consumers[graph->n_consumers-1].handle, NULL, &graph_consumer_main, &graph->consumers[graph->n_consumers-1]);
// }

struct crtx_thread *spawn_thread(char create) {
	struct crtx_thread *t;
	
	n_threads++;
// 	pool = (struct crtx_thread*) realloc(pool, sizeof(struct crtx_thread)*n_threads);
// 	t = &pool[n_threads-1];
	for (t=pool; t && t->next; t = t->next) {}
	
	if (t) {
		t->next = (struct crtx_thread*) calloc(1, sizeof(struct crtx_thread));
		t = t->next;
	} else {
		pool = (struct crtx_thread*) calloc(1, sizeof(struct crtx_thread));
		t = pool;
	}
	
	memset(t, 0, sizeof(struct crtx_thread));
	
	init_signal(&t->start);
	
	if (create) {
		pthread_create(&t->handle, NULL, &thread_main, t);
	} else {
// 		struct crtx_signal signal;
		
// 		init_signal(&signal);
		
// 		t->handle = pthread_self();
		t->handle = 0;
// 		t->main_thread_finished = &signal;
		thread_main(t);
// 		printf("main finished\n");
// 		send_signal(&signal, 0);
		
// 		free_signal(&signal);
	}
	
	return t;
}
// void (*do_stop)(), 
static struct crtx_thread *get_thread_intern(thread_fct fct, void *data, char start, char main_thread) {
	struct crtx_thread *t;
// 	unsigned int i;
	
	LOCK(pool_mutex);
// 	for (i=0; i < n_threads; i++) {
// 		if ( pool[i].in_use == 0 && CAS(&pool[i].in_use, 0, 1) ) {
// 			t = &pool[i];
// 			break;
// 		}
// 	}
	for (t=pool; t; t = t->next) {
		if (main_thread && t->handle == 0) {
			
		} else {
			if ( t->in_use == 0 && CAS(&t->in_use, 0, 1) )
				break;
		}
	}
	
	if (!t) {
		t = spawn_thread(1);
		t->in_use = 1;
	}
	
	UNLOCK(pool_mutex);
	
	t->fct = fct;
	t->fct_data = data;
// 	t->do_stop = do_stop;
	
	if (start)
		send_signal(&t->start, 0);
	
	return t;
}

struct crtx_thread *get_thread(thread_fct fct, void *data, char start) {
	return get_thread_intern(fct, data, start, 0);
}

struct crtx_thread *get_main_thread(thread_fct fct, void *data, char start) {
	return get_thread_intern(fct, data, start, 1);
}

void start_thread(struct crtx_thread *t) {
	send_signal(&t->start, 0);
}

void crtx_threads_init() {
	int i;
	
	for (i=0;i < CRTX_INIT_N_THREADS; i++) {
		spawn_thread(1);
	}
}

void crtx_threads_stop() {
	struct crtx_thread *t;
	
	printf("wait shutdown pool\n");
	LOCK(pool_mutex);
	printf("shutdown pool\n");
	for (t=pool; t; t = t->next) {
		t->stop = 1;
		if (!t->in_use) {
			start_thread(t);
		} else {
			if (t->do_stop)
				t->do_stop(t->fct_data);
		}
	}
	UNLOCK(pool_mutex);
}

void crtx_threads_finish() {
	struct crtx_thread *t, *tnext;
	
	for (t=pool; t; t = tnext) {
		tnext = t->next;
// 		if (pthread_equal(t->handle, pthread_self()))
// 			continue;
// 		printf("wait %p\n", t);
		if (t->handle)
			pthread_join(t->handle, 0);
// 		else
// 			wait_on_signal(t->main_thread_finished);
// 		printf("joined %p\n", t);
		
		free(t);
	}
	
}