#ifndef CRTX_THREADS_H
#define CRTX_THREADS_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <pthread.h>

#define MUTEX_TYPE pthread_mutex_t
#define INIT_MUTEX(mutex) pthread_mutex_init( &(mutex), 0)
#define INIT_REC_MUTEX(mutex)  \
	{ \
		pthread_mutexattr_t attr; \
		pthread_mutexattr_init(&attr); \
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
		pthread_mutex_init( &(mutex), &attr); \
		pthread_mutexattr_destroy(&attr); \
	}
#define LOCK(mutex) pthread_mutex_lock( &(mutex) )
#define UNLOCK(mutex) pthread_mutex_unlock( &(mutex) )
#define SIGNAL_BCAST(cond) pthread_cond_broadcast( &(cond) )
#define SIGNAL_SINGLE(cond) pthread_cond_signal( &(cond) )

#define CAS(ptr, old_val, new_val) __sync_bool_compare_and_swap( (ptr), (old_val), (new_val) )
#define ATOMIC_FETCH_ADD(ptr, val) __sync_fetch_and_add ( &(ptr), (val) );
#define ATOMIC_FETCH_SUB(ptr, val) __sync_fetch_and_sub ( &(ptr), (val) );

typedef void *(*thread_fct)(void *data);

struct crtx_signal {
	unsigned char *condition;
	unsigned char local_condition;
	signed char bitflag_idx;
	
	unsigned char n_refs;
	pthread_mutex_t ref_mutex;
	pthread_cond_t ref_cond;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct crtx_thread;
struct crtx_thread_job_description {
	thread_fct fct;
	void *fct_data;
	
	void (*do_stop)(struct crtx_thread *thread, void *data);
	
	void (*on_finish)(struct crtx_thread *thread, void *on_finish_data);
	void *on_finish_data;
	
// 	struct crtx_thread *thread;
};

struct crtx_thread {
	pthread_t handle;
	
	char stop;
	
	char in_use;
	
	struct crtx_signal start;
	struct crtx_signal finished;
	
	struct crtx_thread_job_description *job;
	
// 	MUTEX_TYPE mutex;
	
// 	struct crtx_signal *main_thread_finished;
	
	struct crtx_thread *next;
};


void crtx_init_signal(struct crtx_signal *signal);
int crtx_wait_on_signal(struct crtx_signal *s, struct timespec *ts);
void crtx_send_signal(struct crtx_signal *s, char brdcst);
void crtx_shutdown_signal(struct crtx_signal *s);
void crtx_reset_signal(struct crtx_signal *signal);
void crtx_reference_signal(struct crtx_signal *s);
void crtx_dereference_signal(struct crtx_signal *s);
char crtx_signal_is_active(struct crtx_signal *s);

struct crtx_thread *crtx_thread_assign_job(struct crtx_thread_job_description *job);
void crtx_thread_start_job(struct crtx_thread *t);
struct crtx_thread *spawn_thread(char create);

void crtx_threads_stop(struct crtx_thread *t);
void crtx_threads_stop_all();
void crtx_threads_interrupt_thread(struct crtx_thread *t);

void crtx_threads_init();
void crtx_threads_finish();

#endif
