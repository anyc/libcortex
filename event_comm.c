

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "core.h"
#include "cache.h"
#include "event_comm.h"

struct serialized_dict_item {
	uint64_t key_length;
	uint64_t payload_size;
	
	uint8_t type;
	uint8_t flags;
};

struct serialized_dict {
	uint8_t signature_length;
};

#define CRTX_SEREV_FLAG_EXPECT_RESPONSE 1<<0

struct serialized_event {
	uint8_t version;
	uint64_t id;
	
	uint32_t type_length;
	uint8_t flags;
};


void send_dict(send_fct send, void *conn_id, struct crtx_dict *ds) {
	struct crtx_dict_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
	char *s;
	int ret;
	
	#ifdef DEBUG
	// suppress valgrind warning
	memset(&sdi, 0, sizeof(struct serialized_dict_item));
	#endif
	
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
				ret = send(conn_id, di->pointer, sdi.payload_size);
				if (ret < 0) {
					printf("sending event failed: %s\n", strerror(errno));
					return;
				}
				break;
			case 'D':
				send_dict(send, conn_id, di->ds);
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return;
		}
		
		di++;
		s++;
	}
}

void send_event_as_dict(struct crtx_event *event, send_fct send, void *conn_id) {
	struct serialized_event sev;
// 	struct crtx_dict *ds;
// 	struct crtx_dict_item *di;
// 	struct serialized_dict sdict;
// 	struct serialized_dict_item sdi;
// 	char *s;
	int ret;
	
	if (!event->data.dict) {
		if (!event->data.raw_to_dict) {
			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
			return;
		}
		
		event->data.raw_to_dict(event);
		
		if (!event->data.dict) {
			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
			return;
		}
	}
	
// 	req.data_length = event->crtx_dict->size + event->crtx_dict->payload_size;
// 	ds = event->data.dict;
	
	#ifdef DEBUG
	// suppress valgrind warning
	memset(&sev, 0, sizeof(struct serialized_event));
// 	memset(&sdi, 0, sizeof(struct serialized_dict_item));
	#endif
	
	sev.version = 0;
// 	sev.flags = CRTX_SREQ_DICT;
	sev.type_length = strlen(event->type);
	sev.flags = 0;
	sev.id = 0;
	
	if (event->response_expected) {
		sev.flags |= CRTX_SEREV_FLAG_EXPECT_RESPONSE;
		sev.id = (uint64_t) (uintptr_t) event;
	} else {
		if (event->original_event_id)
			sev.id = event->original_event_id;
// 		else
// 			sev.id = (uint64_t) (uintptr_t) event;
		
	}
	
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
	
	send_dict(send, conn_id, event->data.dict);
	
// 	sdict.signature_length = strlen(ds->signature);
// 	
// 	ret = send(conn_id, &sdict, sizeof(struct serialized_dict));
// 	if (ret < 0) {
// 		printf("sending event failed: %s\n", strerror(errno));
// 		return;
// 	}
// 	
// 	ret = send(conn_id, ds->signature, sdict.signature_length);
// 	if (ret < 0) {
// 		printf("sending event failed: %s\n", strerror(errno));
// 		return;
// 	}
// 	
// 	s = ds->signature;
// 	di = ds->items;
// 	while (*s) {
// 		sdi.key_length = strlen(di->key);
// 		
// 		sdi.type = di->type;
// 		sdi.flags = di->flags;
// 		
// 		switch (*s) {
// 			case 'u':
// 			case 'i':
// 				sdi.payload_size = sizeof(uint32_t);
// 				break;
// 			case 's':
// 			case 'D':
// 				sdi.payload_size = di->size;
// 				break;
// 			default:
// 				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
// 				return;
// 		}
// 		
// 		ret = send(conn_id, &sdi, sizeof(struct serialized_dict_item));
// 		if (ret < 0) {
// 			printf("sending event failed: %s\n", strerror(errno));
// 			return;
// 		}
// 		
// 		ret = send(conn_id, di->key, sdi.key_length);
// 		if (ret < 0) {
// 			printf("sending event failed: %s\n", strerror(errno));
// 			return;
// 		}
// 		
// 		switch (*s) {
// 			case 'u':
// 			case 'i':
// 				ret = send(conn_id, &di->uint32, sdi.payload_size);
// 				if (ret < 0) {
// 					printf("sending event failed: %s\n", strerror(errno));
// 					return;
// 				}
// 				break;
// 			case 's':
// 			case 'D':
// 				ret = send(conn_id, di->pointer, sdi.payload_size);
// 				if (ret < 0) {
// 					printf("sending event failed: %s\n", strerror(errno));
// 					return;
// 				}
// 				break;
// 			default:
// 				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
// 				return;
// 		}
// 		
// 		di++;
// 		s++;
// 	}
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

struct crtx_dict *recv_dict(recv_fct recv, void *conn_id) {
// 	struct serialized_event sev;
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	struct serialized_dict sdict;
	struct serialized_dict_item sdi;
// 	struct crtx_event *event;
	char *s;
	char ret;
	uint8_t i;
	
	
	ret = my_recv(recv, conn_id, &sdict, sizeof(struct serialized_dict));
	if (!ret) {
		return 0;
	}
	
	if (sdict.signature_length >= CRTX_MAX_SIGNATURE_LENGTH) {
		printf("Too long signature (%u >= %u)\n", sdict.signature_length, CRTX_MAX_SIGNATURE_LENGTH);
		
		return 0;
	}
	
	ds = (struct crtx_dict*) calloc(1, sizeof(struct crtx_dict) + sizeof(struct crtx_dict_item)*sdict.signature_length);
	
	ds->signature = (char*) malloc(sdict.signature_length+1);
	ret = my_recv(recv, conn_id, ds->signature, sdict.signature_length);
	if (!ret) {
		free_dict(ds);
		return 0;
	}
	ds->signature[sdict.signature_length] = 0;
	ds->signature_length = sdict.signature_length;
	
	i = 0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		ret = my_recv(recv, conn_id, &sdi, sizeof(struct serialized_dict_item));
		if (!ret) {
			free_dict(ds);
			return 0;
		}
		
		di->type = sdi.type;
		di->flags = sdi.flags | DIF_KEY_ALLOCATED;
		
		di->key = (char*) malloc(sdi.key_length+1);
		ret = my_recv(recv, conn_id, di->key, sdi.key_length);
		if (!ret) {
			free_dict(ds);
			return 0;
		}
		di->key[sdi.key_length] = 0;
		
		switch (*s) {
			case 'u':
			case 'i':
				if (sdi.payload_size != sizeof(uint32_t)) {
					free_dict(ds);
					return 0;
				}
				
				ret = my_recv(recv, conn_id, &di->uint32, sdi.payload_size);
				if (!ret) {
					free_dict(ds);
					return 0;
				}
				break;
			case 's':
				di->string = (char*) malloc(sdi.payload_size+1);
				ret = my_recv(recv, conn_id, di->string, sdi.payload_size);
				if (!ret) {
					free_dict(ds);
					return 0;
				}
				di->string[sdi.payload_size] = 0;
				break;
			case 'D':
				di->ds = recv_dict(recv, conn_id);
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
				return 0;
		}
		
		di++;
		s++;
		i++;
	}
	
	return ds;
}

#define FREE_EVENT(event) { free((event)->type); free(event); }
// #define FREE_DICT(dict) { free_dict(dict); }

struct crtx_event *recv_event_as_dict(recv_fct recv, void *conn_id) {
	struct serialized_event sev;
// 	struct crtx_dict *ds;
// 	struct crtx_dict_item *di;
// 	struct serialized_dict sdict;
// 	struct serialized_dict_item sdi;
	struct crtx_event *event;
// 	char *s;
	char ret;
// 	uint8_t i;
	
	printf("waiting on new event\n");
	
	ret = my_recv(recv, conn_id, &sev, sizeof(struct serialized_event));
	if (!ret) {
		return 0;
	}
	
	if (sev.version != 0) {
		printf("error parsing event, version mismatch %d\n", sev.version);
		return 0;
	}
	
// 	event = (struct crtx_event*) calloc(1, sizeof(struct crtx_event));
	event = new_event();
	event->original_event_id = sev.id;
	
	if ((sev.flags & CRTX_SEREV_FLAG_EXPECT_RESPONSE) != 0)
		event->response_expected = 1;
// 		reference_event_release(event);
	
	event->type = (char*) malloc(sev.type_length+1);
	ret = my_recv(recv, conn_id, event->type, sev.type_length);
	if (!ret) {
		FREE_EVENT(event);
		return 0;
	}
	event->type[sev.type_length] = 0;
	
	event->data.dict = recv_dict(recv, conn_id);
	if (!event->data.dict) {
		FREE_EVENT(event);
		return 0;
	}
	
	crtx_print_dict(event->data.dict);
	
// 	ret = my_recv(recv, conn_id, &sdict, sizeof(struct serialized_dict));
// 	if (!ret) {
// 		FREE_EVENT(event);
// 		return 0;
// 	}
// 	
// 	if (sdict.signature_length >= CRTX_MAX_SIGNATURE_LENGTH) {
// 		printf("Too long signature (%u >= %u)\n", sdict.signature_length, CRTX_MAX_SIGNATURE_LENGTH);
// 		
// 		FREE_EVENT(event);
// 		return 0;
// 	}
// 	
// 	event->data.dict = (struct crtx_dict*) calloc(1, sizeof(struct crtx_dict) + sizeof(struct crtx_dict_item)*sdict.signature_length);
// 	ds = event->data.dict;
// 	
// 	ds->signature = (char*) malloc(sdict.signature_length+1);
// 	ret = my_recv(recv, conn_id, ds->signature, sdict.signature_length);
// 	if (!ret) {
// 		FREE_DICT(event->data.dict);
// 		FREE_EVENT(event);
// 		return 0;
// 	}
// 	ds->signature[sdict.signature_length] = 0;
// 	
// 	i = 0;
// 	s = ds->signature;
// 	di = ds->items;
// 	while (*s) {
// 		ret = my_recv(recv, conn_id, &sdi, sizeof(struct serialized_dict_item));
// 		if (!ret) {
// 			FREE_DICT(event->data.dict);
// 			FREE_EVENT(event);
// 			return 0;
// 		}
// 		
// 		di->type = sdi.type;
// 		di->flags = sdi.flags | DIF_KEY_ALLOCATED;
// 		
// 		di->key = (char*) malloc(sdi.key_length+1);
// 		ret = my_recv(recv, conn_id, di->key, sdi.key_length);
// 		if (!ret) {
// 			FREE_DICT(event->data.dict);
// 			FREE_EVENT(event);
// 			return 0;
// 		}
// 		di->key[sdi.key_length] = 0;
// 		
// 		switch (*s) {
// 			case 'u':
// 			case 'i':
// 				if (sdi.payload_size != sizeof(uint32_t)) {
// 					FREE_DICT(event->data.dict);
// 					FREE_EVENT(event);
// 					return 0;
// 				}
// 				
// 				ret = my_recv(recv, conn_id, &di->uint32, sdi.payload_size);
// 				if (!ret) {
// 					FREE_DICT(event->data.dict);
// 					FREE_EVENT(event);
// 					return 0;
// 				}
// 				break;
// 			case 's':
// 				di->string = (char*) malloc(sdi.payload_size+1);
// 				ret = my_recv(recv, conn_id, di->string, sdi.payload_size);
// 				if (!ret) {
// 					FREE_DICT(event->data.dict);
// 					FREE_EVENT(event);
// 					return 0;
// 				}
// 				di->string[sdi.payload_size] = 0;
// 				break;
// 			case 'D':
// 				
// 			default:
// 				printf("unknown literal '%c' in signature \"%s\"\n", *s, ds->signature);
// 				return 0;
// 		}
// 		
// 		di++;
// 		s++;
// 		i++;
// 	}
	
// 	crtx_print_dict(ds);
	
	return event;
}

// static struct crtx_event_ll *sent_events = 0;

static void inbox_dispatch_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	add_raw_event(event);
}

// static void response_out_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	event_ll_add(&sent_events, event);
// }
// 
// static void response_in_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	if (!strcmp(event->type, "cortex/socket/response")) {
// 		
// 	}
// }

// typedef char *(*create_key_cb_t)(struct crtx_event *event);
// typedef char (*match_cb_t)(struct crtx_cache *rc, struct crtx_event *event, struct crtx_cache_entry *c_entry);

static char *in_create_key_cb(struct crtx_event *event) {
	char *key = 0;
	
	
// 	if (event->original_event)
// 		snprintf(key, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event->original_event->id);
// 	else
	if (event->original_event_id) {
		key = (char*) malloc(sizeof(uint64_t)*2+1);
		snprintf(key, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event->original_event_id);
	}
	
	return key;
}

static char *out_create_key_cb(struct crtx_event *event) {
	char *key;
	
	key = (char*) malloc(sizeof(uint64_t)*2+1);
	snprintf(key, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event);
	
	return key;
}

static void out_cache_on_hit(struct crtx_cache_task *ct, void **key, struct crtx_event *event, struct crtx_cache_entry *c_entry) {
	printf("ERROR duplicate event ID\n");
}

static void out_cache_on_miss(struct crtx_cache_task *ct, void **key, struct crtx_event *event) {
	struct crtx_cache *dc = ct->cache;
	
	printf("add event outlist\n");
	
	// is somebody interested in a response to this event?
	if (event->refs_before_release == 0)
		return;
	
	reference_event_response(event);
	
	pthread_mutex_lock(&dc->mutex);
	
	dc->n_entries++;
	dc->entries = (struct crtx_cache_entry *) realloc(dc->entries, sizeof(struct crtx_cache_entry)*dc->n_entries);
	
	memset(&dc->entries[dc->n_entries-1], 0, sizeof(struct crtx_cache_entry));
	dc->entries[dc->n_entries-1].key = *key;
	dc->entries[dc->n_entries-1].value = event;
	dc->entries[dc->n_entries-1].value_size = sizeof(void*);
	
	pthread_mutex_unlock(&dc->mutex);
	
	*key = 0;
}

static void in_cache_on_hit(struct crtx_cache_task *ct, void **key, struct crtx_event *event, struct crtx_cache_entry *c_entry) {
	struct crtx_event *orig_event;
	
	printf("in_cache_on_hit\n");
	if (!strcmp(event->type, "cortex.socket.response")) {
		// find entry, pass/drop
		orig_event = (struct crtx_event*) c_entry->value;
		
		orig_event->response.dict = event->data.dict;
		event->data.dict = 0;
		
		dereference_event_response(orig_event);
	}
}

static void in_cache_on_miss(struct crtx_cache_task *ct, void **key, struct crtx_event *event) {
	if (!strcmp(event->type, "cortex.socket.response")) {
		// drop response
	}printf("in_cache_on_miss\n");
}

void create_in_out_box() {
	struct crtx_task *task;
	struct crtx_graph *graph;
	struct crtx_cache *dc;
	struct crtx_cache_task *ct_out, *ct_in;
	
	
	dc = (struct crtx_cache*) calloc(1, sizeof(struct crtx_cache));
	dc->signature = "zp";
	
	
	graph = get_graph_for_event_type(CRTX_EVT_INBOX, crtx_evt_inbox);
	
	ct_in = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct_in->cache = dc;
	ct_in->on_hit = &in_cache_on_hit;
	ct_in->on_miss = &in_cache_on_miss;
	ct_in->create_key = &in_create_key_cb;
	ct_in->match_event = &rcache_match_cb_t_strcmp;
	
	task = new_task();
	task->id = "response_in_matching";
	task->position = 90;
	task->userdata = ct_in;
	task->handle = &response_cache_task;
	add_task(graph, task);
	
	task = new_task();
	task->id = "inbox_dispatch_handler";
	task->handle = &inbox_dispatch_handler;
	task->userdata = 0;
	task->position = 200;
	add_task(graph, task);
	print_tasks(graph);
	graph = get_graph_for_event_type(CRTX_EVT_OUTBOX, crtx_evt_outbox);
	
// 	task = new_task();
// 	task->id = "response_out_handler";
// 	task->handle = &response_out_handler;
// 	task->userdata = 0;
// 	task->position = 10;
// 	add_task(graph, task);
	
	ct_out = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct_out->cache = dc;
	ct_out->on_hit = &out_cache_on_hit;
	ct_out->on_miss = &out_cache_on_miss;
	ct_out->create_key = &out_create_key_cb;
	ct_out->match_event = &rcache_match_cb_t_strcmp;
	
	task = new_task();
	task->id = "response_out_matching";
	task->position = 90;
	task->userdata = ct_out;
	task->handle = &response_cache_task;
	add_task(graph, task);
}
