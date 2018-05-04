/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include "intern.h"
#include "core.h"
#include "cache.h"
#include "event_comm.h"
#include "threads.h"
#include "dict.h"

#define CRTX_SEREV_FLAG_EXPECT_RESPONSE 1<<0

struct serialized_event {
	uint8_t version;
	uint64_t id;
	
	uint32_t type_length;
	uint8_t flags;
};



void write_event_as_dict(struct crtx_event *event, write_fct write, void *conn_id) {
	struct serialized_event sev;
	int ret;
	
	if (!event->data.dict) {
// 		if (!event->data.to_dict) {
// 			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
// 			return;
// 		}
		
// 		event->data.to_dict(&event->data);
		ret = crtx_event_raw2dict(event, 0);
		
		if (ret != CRTX_SUCCESS) {
			printf("cannot prepare this event of type \"%s\" for transmission\n", event->type);
			return;
		}
	}
	
	#ifdef DEBUG
	// suppress valgrind warning
	memset(&sev, 0, sizeof(struct serialized_event));
	#endif
	
	sev.version = 0;
	sev.type_length = strlen(event->type);
	sev.flags = 0;
	sev.id = 0;
	
	if (event->response_expected) {
		sev.flags |= CRTX_SEREV_FLAG_EXPECT_RESPONSE;
		sev.id = (uint64_t) (uintptr_t) event;
	} else {
		if (event->original_event_id)
			sev.id = event->original_event_id;
	}
	
	printf("serializing event %s %d %" PRIu64 "\n", event->type, sev.flags, sev.id);
	
	ret = write(conn_id, &sev, sizeof(struct serialized_event));
	if (ret < 0) {
		printf("writeing event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = write(conn_id, event->type, sev.type_length);
	if (ret < 0) {
		printf("writeing event failed: %s\n", strerror(errno));
		return;
	}
	
	crtx_write_dict(write, conn_id, event->data.dict);
}

#define FREE_EVENT(event) { free((event)->type); free(event); }

struct crtx_event *read_event_as_dict(read_fct read, void *conn_id) {
	struct serialized_event sev;
	struct crtx_event *event;
	struct crtx_dict *dict;
	char ret;
	
	printf("waiting on new event\n");
	
	ret = crtx_read(read, conn_id, &sev, sizeof(struct serialized_event));
	if (!ret) {
		return 0;
	}
	
	if (sev.version != 0) {
		printf("error parsing event, version mismatch %d\n", sev.version);
		return 0;
	}
	
	event = crtx_create_event(0);
	event->original_event_id = sev.id;
	
	if ((sev.flags & CRTX_SEREV_FLAG_EXPECT_RESPONSE) != 0)
		event->response_expected = 1;
	
	event->type = (char*) malloc(sev.type_length+1);
	ret = crtx_read(read, conn_id, event->type, sev.type_length);
	if (!ret) {
		FREE_EVENT(event);
		return 0;
	}
	event->type[sev.type_length] = 0;
	
	printf("deserializing event %s %d %" PRIu64 "\n", event->type, sev.flags, sev.id);
	
// 	event->data.dict
	dict = crtx_read_dict(read, conn_id);
	if (!dict) {
		FREE_EVENT(event);
		return 0;
	}
	
	crtx_event_set_dict_data(event, dict, 0);
	
	crtx_print_dict(event->data.dict);
	
	return event;
}

static char inbox_dispatch_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	add_raw_event(event);
	
	return 1;
}

static char in_create_key_cb(struct crtx_event *event, struct crtx_dict_item *key) {
	if (event->original_event_id) {
		char *skey = 0;
		
		skey = (char*) malloc(sizeof(uint64_t)*2+1);
		if (!skey)
			return 0;
		snprintf(skey, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event->original_event_id);
		
		key->type = 's';
		key->string = skey;
		key->flags |= CRTX_DIF_ALLOCATED_KEY;
		
		return 1;
	}
	
	return 0;
	
// 	return key;
}

static char out_create_key_cb(struct crtx_event *event, struct crtx_dict_item *key) {
	char *skey;
	
	skey = (char*) malloc(sizeof(uint64_t)*2+1);
	if (!skey)
		return 0;
	snprintf(skey, sizeof(uint64_t)*2+1, "%" PRIu64, (uint64_t) (uintptr_t) event);
	
	key->type = 's';
	key->string = skey;
	key->flags |= CRTX_DIF_ALLOCATED_KEY;
	
	return 1;
	
// 	return key;
}

static char out_cache_on_hit(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	printf("ERROR duplicate event ID\n");
	
	return 1;
}

static void out_cache_on_miss(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
// 	struct crtx_cache *dc = ct->cache;
// 	struct crtx_dict_item *ditem;
	
	printf("add event outlist\n");
	
	// is somebody interested in a response to this event?
	if (event->refs_before_release == 0)
		return;
	
	reference_event_response(event);
	
	
	
	crtx_cache_add_entry(ct->cache, key, event, 0);
	
	
	
// 	pthread_mutex_lock(&dc->mutex);
// 	
// // 	dc->n_entries++;
// // 	dc->entries = (struct crtx_cache_entry *) realloc(dc->entries, sizeof(struct crtx_cache_entry)*dc->n_entries);
// // 	
// // 	memset(&dc->entries[dc->n_entries-1], 0, sizeof(struct crtx_cache_entry));
// // 	dc->entries[dc->n_entries-1].key = *key;
// // 	dc->entries[dc->n_entries-1].value = event;
// // 	dc->entries[dc->n_entries-1].value_size = sizeof(void*);
// 	
// 	ditem = crtx_alloc_item(dc->entries);
// 	ditem->key = key;
// 	ditem->pointer = event;
// 	ditem->type = 'p';
// 	
// 	pthread_mutex_unlock(&dc->mutex);
// 	
// 	*key = 0;
}

static char in_cache_on_hit(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	struct crtx_event *orig_event;
	
	printf("in_cache_on_hit\n");
	if (!strcmp(event->type, "cortex.socket.response")) {
		// find entry, pass/drop
		orig_event = (struct crtx_event*) c_entry->pointer;
		
		orig_event->response.dict = event->data.dict;
		event->data.dict = 0;
		
		dereference_event_response(orig_event);
	}
	
	return 1;
}

static void in_cache_on_miss(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	if (!strcmp(event->type, "cortex.socket.response")) {
		// drop response
	}printf("in_cache_on_miss\n");
}

void create_in_out_box() {
	struct crtx_graph *graph;
	struct crtx_cache *dc;
	struct crtx_cache_task *ct_out, *ct_in;
	
	
	dc = (struct crtx_cache*) calloc(1, sizeof(struct crtx_cache));
// 	dc->signature = "zp";
	
	
	graph = get_graph_for_event_type(CRTX_EVT_INBOX, crtx_evt_inbox);
	
	ct_in = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct_in->cache = dc;
	ct_in->on_hit = &in_cache_on_hit;
	ct_in->on_miss = &in_cache_on_miss;
	ct_in->create_key = &in_create_key_cb;
	ct_in->match_event = &rcache_match_cb_t_strcmp;
	
	crtx_create_task(graph, 90, "response_in_matching", &response_cache_task, ct_in);
	
	crtx_create_task(graph, 200, "inbox_dispatch_handler", &inbox_dispatch_handler, 0);
	
	
	graph = get_graph_for_event_type(CRTX_EVT_OUTBOX, crtx_evt_outbox);
	
	ct_out = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct_out->cache = dc;
	ct_out->on_hit = &out_cache_on_hit;
	ct_out->on_miss = &out_cache_on_miss;
	ct_out->create_key = &out_create_key_cb;
	ct_out->match_event = &rcache_match_cb_t_strcmp;
	
	crtx_create_task(graph, 90, "response_out_matching", &response_cache_task, ct_out);
	
// 	crtx_create_task(graph, 200, "outbox_dispatch_handler", &outbox_dispatch_handler, 0);
}
