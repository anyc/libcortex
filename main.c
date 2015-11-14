#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

//  gcc main.c -o main `pkg-config --cflags --libs libsystemd` -lpthread

#define ASSERT(x) { if (!(x)) { printf("assertion failed: " #x); exit(1); } }

char ** event_types = 0;
unsigned n_event_types = 0;

char * local_event_types[] = { "test" };
unsigned n_local_event_types = 1;

struct event {
	char *type;
	
	void *data;
	
	struct event *next;
};

struct event_task {
	void (*handle)(struct event *event);
};

struct thread {
	pthread_t handle;
	
	char stop;
	
	struct event_graph *graph;
};

struct event_graph {
	pthread_mutex_t mutex;
	
	struct event_task *entry_task;
	
	struct thread *consumers;
	unsigned int n_consumers;
	
	struct event *equeue;
	unsigned int n_queue_entries;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
};

void add_event(struct event_graph *graph, struct event *event) {
	struct event *eit;
	
	pthread_mutex_lock(&graph->mutex);
	
	for (eit=graph->equeue; eit && eit->next; eit = eit->next) {}
	
	if (eit) {
		eit->next = event;
	} else {
		graph->equeue = event;
	}
	event->next = 0;
	graph->n_queue_entries++;
	
// 	printf("added event\n");
	
	pthread_cond_signal(&graph->queue_cond);
	
	pthread_mutex_unlock(&graph->mutex);
}

void graph_consumer_main(void *arg) {
	struct thread *self = (struct thread*) arg;
	struct event *e;
	
// 	printf("consumer starts\n");
	
	while (!self->stop) {
		pthread_mutex_lock(&self->graph->queue_mutex);
		
// 		while (self->graph->n_queue_entries > 0)
// 		printf("wait on entry\n");
		
		while (self->graph->n_queue_entries == 0)
			pthread_cond_wait(&self->graph->queue_cond, &self->graph->queue_mutex);
		
		e = self->graph->equeue;
		self->graph->equeue = self->graph->equeue->next;
		self->graph->n_queue_entries--;
		
// 		printf("handle entry\n");
		self->graph->entry_task->handle(e);
		
		free(e);
		
		pthread_mutex_unlock(&self->graph->queue_mutex);
	}
}

void spawn_thread(struct event_graph *graph) {
	pthread_mutex_lock(&graph->mutex);
	
	graph->n_consumers++;
	graph->consumers = (struct thread*) realloc(graph->consumers, sizeof(struct thread)*graph->n_consumers); ASSERT(graph->consumers);
	
	graph->consumers[graph->n_consumers-1].stop = 0;
	graph->consumers[graph->n_consumers-1].graph = graph;
	pthread_create(&graph->consumers[graph->n_consumers-1].handle, NULL, &graph_consumer_main, &graph->consumers[graph->n_consumers-1]);
	
	pthread_mutex_unlock(&graph->mutex);
}

void new_eventgraph(struct event_graph **event_graph) {
	struct event_graph *graph;
	int ret;
	
	graph = (struct event_graph*) malloc(sizeof(struct event_graph)); ASSERT(graph);
	
	*event_graph = graph;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
	// TODO spawn on demand
	spawn_thread(graph);
	
	
}

void add_event_type(char *event_type) {
	n_event_types++;
	event_types = (char**) realloc(event_types, sizeof(char*)*n_event_types);
	event_types[n_event_types-1] = event_type;
}







struct dbus_thread {
	sd_bus *bus;
	struct event_graph *graph;
};

void handle_event(struct event *event) {
	uint8_t type;
	const char *sig;
	char *val;
	
	sd_bus_message *m = (sd_bus_message *) event->data;
	
	sig = sd_bus_message_get_signature(m, 1);
	
	printf("got bus signal... %s %s %s %s %s \"%s\"\n",
		  sd_bus_message_get_path(m),
		  sd_bus_message_get_interface(m),
		  sd_bus_message_get_member(m),
		  sd_bus_message_get_destination(m),
		  sd_bus_message_get_sender(m),
		  sig
	);
	
	if (sig[0] == 's') {
		sd_bus_message_read_basic(m, 's', &val);
		printf("\tval %s\n", val);
	}
}

// static int bus_signal_cb(sd_bus *bus, int ret, sd_bus_message *m, void *user_data)
static int bus_signal_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
// 	uint8_t type;
// 	const char *sig;
// 	char *val;
	struct dbus_thread * dthread;
	struct event *event;
	
	dthread = (struct dbus_thread*) userdata;
	
	event = (struct event*) malloc(sizeof(struct event));
	event->type = local_event_types[0];
	event->data = m;
	
// 	sig = sd_bus_message_get_signature(m, 1);
// 	
// 	printf("got bus signal... %s %s %s %s %s \"%s\"\n",
// 		  sd_bus_message_get_path(m),
// 		  sd_bus_message_get_interface(m),
// 		  sd_bus_message_get_member(m),
// 		  sd_bus_message_get_destination(m),
// 		  sd_bus_message_get_sender(m),
// 		  sig
// 	);
// 	
// 	if (sig[0] == 's') {
// 		sd_bus_message_read_basic(m, 's', &val);
// 		printf("\tval %s\n", val);
// 	}
	
	add_event(dthread->graph, event);
	
	return 0;
}

void * tmain(void *data) {
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_slot* slot;
	sd_bus *bus;
	int r;
	struct dbus_thread * dthread;
	
	dthread = (struct dbus_thread*) data;
	bus = dthread->bus;
	
	slot = sd_bus_get_current_slot(bus);
	
// 	r = sd_bus_add_match(bus, NULL, "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'", match_callback, NULL);
// 	sd_bus_add_match(bus, &slot, "type='signal',path='/Mixers/PulseAudio__Playback_Devices_1'" , bus_signal_cb, bus);
	sd_bus_add_match(bus, &slot, "type='signal'" , bus_signal_cb, dthread);
	
	printf("listening...\n");
	while (1) {
		r = sd_bus_process(bus, NULL);
		if (r < 0) {
			fprintf(stderr, "sd_bus_process failed %d\n", r);
			break;
		}
		if (r == 0) {
			r = sd_bus_wait(bus, (uint64_t) -1);
			if (r < 0) {
				fprintf(stderr, "Failed to wait: %d", r);
				break;
			}
		}
	}
	
	sd_bus_flush(bus);
	sd_bus_error_free(&error);
	sd_bus_unref(bus);
}

int main(int argc, char *argv[]) {
	sd_bus_message *m = NULL;
	sd_bus *bus = NULL;
	sd_bus *ubus = NULL;
	const char *path;
	int r,i;
	
	struct event_graph *event_graph;
	
	for (i=0; i<n_local_event_types; i++) {
		add_event_type(local_event_types[i]);
	}
	
	new_eventgraph(&event_graph);
	
	struct event_task *etask = (struct event_task*) malloc(sizeof(struct event_task));
	etask->handle = &handle_event;
	
	event_graph->entry_task = etask;
	
	/* Connect to the system bus */
	r = sd_bus_open_system(&bus);
	if (r < 0) {
			fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
			return 1;
	}
	
	r = sd_bus_open_user(&ubus);
	if (r < 0) {
			fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
			return 1;
	}
	
	pthread_t uthread, sthread;
	struct dbus_thread duthread, dsthread;
	
	duthread.bus = ubus;
	duthread.graph = event_graph;
	dsthread.bus = bus;
	dsthread.graph = event_graph;
	
	pthread_create(&sthread, NULL, tmain, &dsthread);
	pthread_create(&uthread, NULL, tmain, &duthread);
	
	pthread_join(sthread, NULL);
	pthread_join(uthread, NULL);
	
// 	/* Issue the method call and store the respons message in m */
// 	r = sd_bus_call_method(bus,
// 						"org.freedesktop.UDisks2",           /* service to contact */
// 						"/org/freedesktop/UDisks2/drives/hp________DDVDW_TS_H653R_R3786GASC5277600",          /* object path */
// 						"org.freedesktop.DBus.Peer",   /* interface name */
// 						"GetMachineId",                          /* method name */
// 						&error,                               /* object to return error in */
// 						&m,                                   /* return message on success */
// 						"",                                 /* input signature */
// 						"",                       /* first argument */
// 						"");                           /* second argument */
// 	if (r < 0) {
// 			fprintf(stderr, "Failed to issue method call: %s\n", error.message);
// 			goto finish;
// 	}
// 
// 	/* Parse the response message */
// 	r = sd_bus_message_read(m, "s", &path);
// 	if (r < 0) {
// 			fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
// 			goto finish;
// 	}
// 
// 	printf("Queued service job as %s.\n", path);

// 	while (1) {
// 		r = sd_bus_process(bus, NULL);
// 		if (r < 0) {
// 			fprintf(stderr, "sd_bus_process failed %d\n", r);
// 			break;
// 		}
// 		if (r == 0) {
// 			r = sd_bus_wait(bus, (uint64_t) -1);
// 			if (r < 0) {
// 				fprintf(stderr, "Failed to wait: %d", r);
// 				goto finish;
// 			}
// 		}
// 	}
	
// finish:
// 	sd_bus_flush(bus);
// 	sd_bus_error_free(&error);
// 	sd_bus_message_unref(m);
// 	sd_bus_unref(bus);
// 
// 	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}