

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "core.h"
#include "event_comm.h"

void send_event_as_dict(struct event *event, send_fct send, void *conn_id) {
	struct serialized_event sev;
	struct data_struct *ds;
	struct data_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
	char *s;
	int ret;
	
	if (!event->data_dict) {
		if (!event->raw_to_dict) {
			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
			return;
		}
		
		event->raw_to_dict(event);
		
		if (!event->data_dict) {
			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
			return;
		}
	}
	
// 	req.data_length = event->data_struct->size + event->data_struct->payload_size;
	ds = event->data_dict;
	
	sev.version = 0;
	if (event->original_event)
		sev.id = (uint64_t) (uintptr_t) event->original_event->id;
	else
		sev.id = (uint64_t) (uintptr_t) event;
// 	sev.flags = CRTX_SREQ_DICT;
	sev.type_length = strlen(event->type);
	
	ret = send(conn_id, &sev, sizeof(struct serialized_event));
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = send(conn_id, event->type, sev.type_length);
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	
	sdict.signature_length = strlen(ds->signature);
	
	ret = send(conn_id, &sdict, sizeof(struct serialized_dict));
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = send(conn_id, ds->signature, sdict.signature_length);
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
		sdi.key_length = strlen(di->key);
		
		sdi.type = di->type;
		sdi.flags = di->flags;
		
		switch (*s) {
			case 'u':
			case 'i':
				sdi.payload_size = sizeof(uint32_t);
				break;
			case 's':
			case 'D':
				sdi.payload_size = di->size;
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return;
		}
		
		ret = send(conn_id, &sdi, sizeof(struct serialized_dict_item));
		if (ret < 0) {
			printf("sending event failed: %s\n", strerror(errno));
			return;
		}
		
		ret = send(conn_id, di->key, sdi.key_length);
		if (ret < 0) {
			printf("sending event failed: %s\n", strerror(errno));
			return;
		}
		
		switch (*s) {
			case 'u':
			case 'i':
				ret = send(conn_id, &di->uint32, sdi.payload_size);
				if (ret < 0) {
					printf("sending event failed: %s\n", strerror(errno));
					return;
				}
				break;
			case 's':
			case 'D':
				ret = send(conn_id, di->pointer, sdi.payload_size);
				if (ret < 0) {
					printf("sending event failed: %s\n", strerror(errno));
					return;
				}
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return;
		}
		
		di++;
		s++;
	}
}

char my_recv(recv_fct recv, void *conn_id, void *buffer, size_t read_bytes) {
	int n;
	
	n = recv(conn_id, buffer, read_bytes);
	if (n < 0) {
		printf("error while reading from socket: %s\n", strerror(errno));
		return 0;
	} else
	if (n != read_bytes) {
		printf("did not receive enough data (%d != %zu), aborting\n", n, read_bytes);
		return 0;
	}
	
	return 1;
}

#define FREE_EVENT(event) { free((event)->type); free(event); }
#define FREE_DICT(dict) { free_dict(dict); }

struct event *recv_event_as_dict(recv_fct recv, void *conn_id) {
	struct serialized_event sev;
	struct data_struct *ds;
	struct data_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
	struct event *event;
	char *s;
	char ret;
	uint8_t i;
	
	ret = my_recv(recv, conn_id, &sev, sizeof(struct serialized_event));
	if (!ret) {
		return 0;
	}
	
	if (sev.version != 0) {
		printf("error parsing event, version mismatch %d\n", sev.version);
		return 0;
	}
	
// 	event = (struct event*) calloc(1, sizeof(struct event));
	event = new_event();
	event->id = sev.id;
	
	event->type = (char*) malloc(sev.type_length+1);
	ret = my_recv(recv, conn_id, event->type, sev.type_length);
	if (!ret) {
		FREE_EVENT(event);
		return 0;
	}
	event->type[sev.type_length] = 0;
	
	
	ret = my_recv(recv, conn_id, &sdict, sizeof(struct serialized_dict));
	if (!ret) {
		FREE_EVENT(event);
		return 0;
	}
	
	if (sdict.signature_length >= CRTX_MAX_SIGNATURE_LENGTH) {
		printf("Too long signature (%u >= %u)\n", sdict.signature_length, CRTX_MAX_SIGNATURE_LENGTH);
		
		FREE_EVENT(event);
		return 0;
	}
	
	event->data_dict = (struct data_struct*) calloc(1, sizeof(struct data_struct) + sizeof(struct data_item)*sdict.signature_length);
	ds = event->data_dict;
	
	ds->signature = (char*) malloc(sdict.signature_length+1);
	ret = my_recv(recv, conn_id, ds->signature, sdict.signature_length);
	if (!ret) {
		FREE_DICT(event->data_dict);
		FREE_EVENT(event);
		return 0;
	}
	ds->signature[sdict.signature_length] = 0;
	
	i = 0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		ret = my_recv(recv, conn_id, &sdi, sizeof(struct serialized_dict_item));
		if (!ret) {
			FREE_DICT(event->data_dict);
			FREE_EVENT(event);
			return 0;
		}
		
		di->type = sdi.type;
		di->flags = sdi.flags | DIF_KEY_ALLOCATED;
		
		di->key = (char*) malloc(sdi.key_length+1);
		ret = my_recv(recv, conn_id, di->key, sdi.key_length);
		if (!ret) {
			FREE_DICT(event->data_dict);
			FREE_EVENT(event);
			return 0;
		}
		di->key[sdi.key_length] = 0;
		
		switch (*s) {
			case 'u':
			case 'i':
				if (sdi.payload_size != sizeof(uint32_t)) {
					FREE_DICT(event->data_dict);
					FREE_EVENT(event);
					return 0;
				}
				
				ret = my_recv(recv, conn_id, &di->uint32, sdi.payload_size);
				if (!ret) {
					FREE_DICT(event->data_dict);
					FREE_EVENT(event);
					return 0;
				}
				break;
			case 's':
				di->string = (char*) malloc(sdi.payload_size+1);
				ret = my_recv(recv, conn_id, di->string, sdi.payload_size);
				if (!ret) {
					FREE_DICT(event->data_dict);
					FREE_EVENT(event);
					return 0;
				}
				di->string[sdi.payload_size] = 0;
				break;
			case 'D':
				// TODO
				printf("ERROR handle D recv_event_as_dict\n");
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return 0;
		}
		
		di++;
		s++;
		i++;
	}
	
	crtx_print_dict(ds);
	
	return event;
}

static struct queue_entry *sent_events = 0;

static void inbox_handler(struct event *event, void *userdata, void **sessiondata) {
	add_raw_event(event);
}

static void response_out_handler(struct event *event, void *userdata, void **sessiondata) {
	event_ll_add(&sent_events, event);
}

static void response_in_handler(struct event *event, void *userdata, void **sessiondata) {
	if (!strcmp(event->type, "cortex/socket/response")) {
		
	}
}

// typedef char *(*create_key_cb_t)(struct event *event);
// typedef char (*match_cb_t)(struct response_cache *rc, struct event *event, struct response_cache_entry *c_entry);

static char *create_key_cb(struct event *event) {
	char *key;
	
	key = (char*) malloc(sizeof(uint64_t)*2+1);
	snprintf(key, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event);
	
	return key;
}

void create_in_out_box() {
	struct event_task *task;
	struct event_graph *graph;
	
	graph = get_graph_for_event_type(CRTX_EVT_INBOX, crtx_evt_inbox);
	
	task = new_task();
	task->id = "inbox_handler";
	task->handle = &inbox_handler;
	task->userdata = 0;
	task->position = 200;
	add_task(graph, task);
	
	graph = get_graph_for_event_type(CRTX_EVT_OUTBOX, crtx_evt_outbox);
	
// 	task = new_task();
// 	task->id = "response_out_handler";
// 	task->handle = &response_out_handler;
// 	task->userdata = 0;
// 	task->position = 10;
// 	add_task(graph, task);
	
	task = create_response_cache_task("zp", create_key_cb);
	add_task(graph, task);
}
