
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "intern.h"
#include "core.h"
#include "threads.h"

#ifndef CRTX_INIT_N_THREADS
#define CRTX_INIT_N_THREADS 0
#endif

struct crtx_thread *pool = 0;
MUTEX_TYPE pool_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int n_threads = 0;
static char pool_stop = 0;

void init_signal(struct crtx_signal *signal) {
	int ret;
	
	ret = pthread_mutex_init(&signal->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&signal->cond, NULL); ASSERT(ret >= 0);
	
	signal->local_condition = 0;
	signal->condition = &signal->local_condition;
	
	ret = pthread_mutex_init(&signal->ref_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&signal->ref_cond, NULL); ASSERT(ret >= 0);
	
	signal->n_refs = 0;
}

struct crtx_signal *new_signal() {
	struct crtx_signal *signal;
	
	signal = (struct crtx_signal*) calloc(1, sizeof(struct crtx_signal));
	
	init_signal(signal);
	
	return signal;
}

void reset_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	while (s->n_refs > 0)
		pthread_cond_wait(&s->ref_cond, &s->ref_mutex);
	UNLOCK(s->ref_mutex);
	
	*s->condition = 0;
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

void reference_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	
	s->n_refs += 1;
	
	UNLOCK(s->ref_mutex);
}

void dereference_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	
	if (s->n_refs == 0)
		ERROR("signal referance < 0\n");
	else
		s->n_refs -= 1;
	if (s->n_refs == 0)
		SIGNAL_BCAST(s->ref_cond);
	
	UNLOCK(s->ref_mutex);
}

void free_signal(struct crtx_signal *s) {
	LOCK(s->mutex);
	UNLOCK(s->mutex);
	
	pthread_mutex_destroy(&s->ref_mutex);
	pthread_cond_destroy(&s->ref_cond);
	
	pthread_mutex_destroy(&s->mutex);
	pthread_cond_destroy(&s->cond);
}

char crtx_signal_is_active(struct crtx_signal *s) {
	return *s->condition;
}

// void test(int sig) {
// 	printf("thread sign\n");
// }

static void * crtx_thread_tmain(void *data) {
	struct crtx_thread *thread = (struct crtx_thread*) data;
	
	DBG("thread %p started\n", data);
	
// 	signal(SIGINT,test);
	
	while (!thread->stop) {
		// we wait until someone has work for us
		wait_on_signal(&thread->start);
		if (thread->stop) {
			send_signal(&thread->finished, 1);
			break;
		}
		
		// execute
		thread->job->fct(thread->job->fct_data);
		
		send_signal(&thread->finished, 1);
		
		if (thread->job->on_finish)
			thread->job->on_finish(thread, thread->job->on_finish_data);
		
		reset_signal(&thread->finished);
		
		LOCK(pool_mutex);
		reset_signal(&thread->start);
		thread->in_use = 0;
		thread->job = 0;
// 		thread->fct = 0;
// 		thread->fct_data = 0;
// 		thread->on_finish = 0;
// 		thread->on_finish_data = 0;
// 		thread->do_stop = 0;
		UNLOCK(pool_mutex);
	}
	
// 	printf("thread %p ends\n", data);
	
	return 0;
}

struct crtx_thread *spawn_thread(char create) {
	struct crtx_thread *t;
	
	n_threads++;
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
	init_signal(&t->finished);
	
	if (create) {
		pthread_create(&t->handle, NULL, &crtx_thread_tmain, t);
	} else {
		t->handle = 0;
		crtx_thread_tmain(t);
	}
	
	return t;
}

// static struct crtx_thread *get_thread_intern(thread_fct fct, void *data, char start, char main_thread) {
static struct crtx_thread *get_thread_intern(char main_thread) {
	struct crtx_thread *t;
	
	LOCK(pool_mutex);
	
	if (pool_stop) {
		UNLOCK(pool_mutex);
		return 0;
	}
	
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
	
// 	t->fct = fct;
// 	t->fct_data = data;
	
// 	if (start)
// 		send_signal(&t->start, 0);
// 		start_thread(t);
	
	return t;
}

// struct crtx_thread *get_thread(thread_fct fct, void *data, char start) {
struct crtx_thread *crtx_thread_assign_job(struct crtx_thread_job_description *job) {
	struct crtx_thread *t;
	
	t = get_thread_intern(0);
	t->job = job;
	
	return t;
}

// struct crtx_thread *get_main_thread(thread_fct fct, void *data, char start) {
// 	return get_thread_intern(fct, data, start, 1);
// }

void crtx_thread_start_job(struct crtx_thread *t) {
// 	if (t->handle == 0)
// 		pthread_create(&t->handle, NULL, &thread_main, t);
	
	send_signal(&t->start, 0);
}

void crtx_threads_init() {
	int i;
	
	for (i=0;i < CRTX_INIT_N_THREADS; i++) {
		spawn_thread(1);
	}
}

void crtx_threads_interrupt_thread(struct crtx_thread *t) {
	pthread_kill(t->handle, SIGUSR1);
}

void crtx_threads_stop(struct crtx_thread *t) {
// 	LOCK(pool_mutex);
	
	t->stop = 1;
	
// 	if (!t->in_use) {
	crtx_thread_start_job(t);
// 	} else {
		if (t->job->do_stop)
			t->job->do_stop(t, t->job->fct_data);
// 	}
	
// 	UNLOCK(pool_mutex);
}

void crtx_threads_stop_all() {
	struct crtx_thread *t;
	
	LOCK(pool_mutex);
	
	if (pool_stop) {
		UNLOCK(pool_mutex);
		return;
	}
	
	pool_stop = 1;
	
	DBG("shutdown pool\n");
	
	for (t=pool; t; t = t->next) {
		crtx_threads_stop(t);
	}
	UNLOCK(pool_mutex);
}

void crtx_threads_finish() {
	struct crtx_thread *t, *tnext;
	
	for (t=pool; t; t = tnext) {
		tnext = t->next;
		
		if (t->handle) {
// 			pthread_kill(t->handle, 2);
			pthread_join(t->handle, 0);
		}
		
		free(t);
	}
	
}
