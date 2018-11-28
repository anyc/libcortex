/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

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

/*
 * signals
 */

void crtx_init_signal(struct crtx_signal *s) {
	int ret;
// 	pthread_condattr_t attr;
	
	ret = pthread_mutex_init(&s->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&s->cond, 0); ASSERT(ret >= 0);
	
// 	ret = pthread_condattr_init(&attr); ASSERT(ret >= 0);
// 	ret = pthread_condattr_setclock(&attr, s->clockid);
// 	ret = pthread_cond_init(&s->cond, &attr); ASSERT(ret >= 0);
// 	pthread_condattr_destroy(&attr);
	
	s->local_condition = 0;
	s->condition = &s->local_condition;
	s->bitflag_idx = 0;
	
	ret = pthread_mutex_init(&s->ref_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&s->ref_cond, NULL); ASSERT(ret >= 0);
	
	s->n_refs = 0;
}

// static struct crtx_signal *new_signal() {
// 	struct crtx_signal *signal;
// 	
// 	signal = (struct crtx_signal*) calloc(1, sizeof(struct crtx_signal));
// 	
// 	crtx_init_signal(signal);
// 	
// 	return signal;
// }

void crtx_reset_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	while (s->n_refs > 0)
		pthread_cond_wait(&s->ref_cond, &s->ref_mutex);
	UNLOCK(s->ref_mutex);
	
	if (s->bitflag_idx == -1) {
		*s->condition = 0;
	} else {
		*s->condition &= ~(1 << s->bitflag_idx);
	}
}

int crtx_wait_on_signal(struct crtx_signal *s, struct timespec *ts) {
	int r, err;
	
	r = LOCK(s->mutex);
	if (r < 0) {
		ERROR("cannot lock mutex: %s\n", strerror(-r));
		return r;
	}
	
// 	if (s->bitflag_idx == -1) {
// 		while (!*s->condition)
// 			pthread_cond_wait(&s->cond, &s->mutex);
// 	} else {
// 		while (*s->condition & (1 << s->bitflag_idx) == 0)
// 			pthread_cond_wait(&s->cond, &s->mutex);
// 	}
	while (!crtx_signal_is_active(s)) {
		if (ts) {
			r = pthread_cond_timedwait(&s->cond, &s->mutex, ts);
			if (r == ETIMEDOUT) {
				err = -ETIMEDOUT;
				goto finish;
			} else
			if (r < 0) {
				err = r;
				goto finish;
			}	
		} else {
			r = pthread_cond_wait(&s->cond, &s->mutex);
			if (r < 0) {
				err = r;
				goto finish;
			}
		}
	}
	
finish:
	r = UNLOCK(s->mutex);
	if (r < 0) {
		ERROR("cannot unlock mutex: %s\n", strerror(-r));
		return r;
	}
	
	return err;
}

void crtx_send_signal(struct crtx_signal *s, char brdcst) {
	LOCK(s->mutex);
	
	if (s->bitflag_idx == -1) {
		if (!*s->condition)
			*s->condition = 1;
	} else {
		if ((*s->condition & (1 << s->bitflag_idx)) == 0 )
			*s->condition |= (1 << s->bitflag_idx);
	}
	
	if (brdcst) {
		SIGNAL_BCAST(s->cond);
	} else {
		SIGNAL_SINGLE(s->cond);
	}
	
	UNLOCK(s->mutex);
}

void crtx_reference_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	
	s->n_refs += 1;
	
	UNLOCK(s->ref_mutex);
}

void crtx_dereference_signal(struct crtx_signal *s) {
	LOCK(s->ref_mutex);
	
	if (s->n_refs == 0)
		ERROR("signal referance < 0\n");
	else
		s->n_refs -= 1;
	if (s->n_refs == 0)
		SIGNAL_BCAST(s->ref_cond);
	
	UNLOCK(s->ref_mutex);
}

void crtx_shutdown_signal(struct crtx_signal *s) {
	LOCK(s->mutex);
	UNLOCK(s->mutex);
	
	pthread_mutex_destroy(&s->ref_mutex);
	pthread_cond_destroy(&s->ref_cond);
	
	pthread_mutex_destroy(&s->mutex);
	pthread_cond_destroy(&s->cond);
}

char crtx_signal_is_active(struct crtx_signal *s) {
	if (s->bitflag_idx == -1) {
		return *s->condition;
	} else {
		return (*s->condition & (1 << s->bitflag_idx));
	}
}





/*
 * threads
 */

static void * crtx_thread_tmain(void *data) {
	struct crtx_thread *thread = (struct crtx_thread*) data;
	
	DBG("thread %p started\n", data);
	
// 	signal(SIGINT,test);
	
	while (!thread->stop) {
		// we wait until someone has work for us
		crtx_wait_on_signal(&thread->start, 0);
		if (thread->stop) {
			crtx_send_signal(&thread->finished, 1);
			break;
		}
		
		// execute
		thread->job->fct(thread->job->fct_data);
		
		crtx_send_signal(&thread->finished, 1);
		
		if (thread->job->on_finish)
			thread->job->on_finish(thread, thread->job->on_finish_data);
		
		crtx_reset_signal(&thread->finished);
		
		LOCK(pool_mutex);
		crtx_reset_signal(&thread->start);
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
	
	crtx_init_signal(&t->start);
	crtx_init_signal(&t->finished);
	
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
// 		crtx_send_signal(&t->start, 0);
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
	
	crtx_send_signal(&t->start, 0);
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
