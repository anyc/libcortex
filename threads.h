
#include <pthread.h>

#define MUTEX_TYPE pthread_mutex_t
#define INIT_MUTEX(mutex) pthread_mutex_init( &(mutex), 0)
#define LOCK(mutex) pthread_mutex_lock( &(mutex) )
#define UNLOCK(mutex) pthread_mutex_unlock( &(mutex) )
#define SIGNAL_BCAST(cond) pthread_cond_broadcast( &(cond) )
#define SIGNAL_SINGLE(cond) pthread_cond_signal( &(cond) )

#define CAS(ptr, old_val, new_val) __sync_bool_compare_and_swap( (ptr), (old_val), (new_val) )
#define ATOMIC_FETCH_ADD(ptr, val) __sync_fetch_and_add ( &(ptr), (val) );
#define ATOMIC_FETCH_SUB(ptr, val) __sync_fetch_and_sub ( &(ptr), (val) );

typedef void *(*thread_fct)(void *data);

struct crtx_signal {
	char *condition;
	char local_condition;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct crtx_thread {
	pthread_t handle;
	
	char stop;
	
	char in_use;
	
	thread_fct fct;
	void *fct_data;
	
	struct crtx_signal start;
	
	void (*do_stop)(void *data);
	
	void (*on_finish)(struct crtx_thread *thread, void *on_finish_data);
	void *on_finish_data;
	
// 	MUTEX_TYPE mutex;
	
// 	struct crtx_signal *main_thread_finished;
	
	struct crtx_thread *next;
};


struct crtx_signal *new_signal();
void init_signal(struct crtx_signal *signal);
void wait_on_signal(struct crtx_signal *s);
void send_signal(struct crtx_signal *s, char brdcst);
void free_signal(struct crtx_signal *s);
void reset_signal(struct crtx_signal *signal);

struct crtx_thread *get_thread(thread_fct fct, void *data, char start);
void start_thread(struct crtx_thread *t);
struct crtx_thread *spawn_thread(char create);

void crtx_threads_stop();

void crtx_threads_init();
void crtx_threads_finish();