
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pulse/introspect.h>

#include "pulseaudio.h"


char *crtx_pa_subscription_etypes[] = {
	[PA_SUBSCRIPTION_EVENT_SINK] = "pulseaudio/sink",
	[PA_SUBSCRIPTION_EVENT_SOURCE] = "pulseaudio/source",
	[PA_SUBSCRIPTION_EVENT_SINK_INPUT] = "pulseaudio/sink_input",
	[PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT] = "pulseaudio/source_output",
	[PA_SUBSCRIPTION_EVENT_MODULE] = "pulseaudio/module",
	[PA_SUBSCRIPTION_EVENT_CLIENT] = "pulseaudio/client",
	[PA_SUBSCRIPTION_EVENT_SAMPLE_CACHE] = "pulseaudio/sample_cache",
	[PA_SUBSCRIPTION_EVENT_SERVER] = "pulseaudio/server",
	[PA_SUBSCRIPTION_EVENT_CARD] = "pulseaudio/card",
};

struct generic_pa_callback_helper {
	enum pa_subscription_event_type type;
	struct crtx_pa_listener *palist;
	uint32_t index;
};


// convert proplist to dict by iterating through all keys in the list
char crtx_pa_proplist2dict(pa_proplist *props, struct crtx_dict **dict_ptr) {
	const char *key, *value;
	void *iter;
	struct crtx_dict_item *di;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	
	iter = 0;
	while (1) {
		key = pa_proplist_iterate(props, &iter);
		if (!key)
			break;
		
		value = pa_proplist_gets(props, key);
		if (value) {
			di = crtx_alloc_item(*dict_ptr);
			crtx_fill_data_item(di, 's', crtx_stracpy(key, 0), value, 0, CRTX_DIF_ALLOCATED_KEY | CRTX_DIF_CREATE_DATA_COPY);
		}
	}
	
	return 0;
}

// include convert functions initially created by hdr2dict.py
#include "pulseaudio_2dict.c"

// function that is called by PA when it gathered the request information
static void generic_pa_get_info_callback(pa_context *c, void *info, int eol, void *userdata) {
	struct generic_pa_callback_helper *helper;
	struct crtx_event *nevent;
	struct crtx_dict_item *di;
	pa_subscription_event_type_t ev_op;
	
	
	helper = (struct generic_pa_callback_helper *) userdata;
	
	if (eol < 0) {
		// TODO ?
		if (pa_context_errno(c) == PA_ERR_NOENTITY) {
			free(helper);
			return;
		}
		
		fprintf(stderr, "%s idx %d %d error: %s\n", __func__, helper->index, helper->type,
				pa_strerror(pa_context_errno(c)));
		
		
		free(helper);
		return;
	}
	if (eol > 0) {
		return;
	}
	
	
	nevent = create_event(crtx_pa_subscription_etypes[helper->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK], 0, 0);
	nevent->data.dict = crtx_init_dict(0, 0, 0);
	
	di = crtx_alloc_item(nevent->data.dict);
	
	ev_op = helper->type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
	if (ev_op == PA_SUBSCRIPTION_EVENT_REMOVE) {
		crtx_fill_data_item(di, 's', "operation", "remove", strlen("remove"), CRTX_DIF_DONT_FREE_DATA);
		
		di = crtx_alloc_item(nevent->data.dict);
		crtx_fill_data_item(di, 'u', "index", helper->index, sizeof(helper->index), 0);
	} else {
		if (ev_op == PA_SUBSCRIPTION_EVENT_CHANGE)
			crtx_fill_data_item(di, 's', "operation", "change", strlen("change"), CRTX_DIF_DONT_FREE_DATA);
		else if (ev_op == PA_SUBSCRIPTION_EVENT_NEW)
			crtx_fill_data_item(di, 's', "operation", "new", strlen("new"), CRTX_DIF_DONT_FREE_DATA);
		
		switch ((helper->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)) {
			case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
				crtx_pa_sink_input_info2dict(info, &nevent->data.dict);
				break;
			case PA_SUBSCRIPTION_EVENT_CARD:
				crtx_pa_card_info2dict(info, &nevent->data.dict);
				break;
			default:
				ERROR("TODO no handler yet for type %d\n", helper->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK);
				break;
		}
	}
	
	add_event(helper->palist->parent.graph, nevent);
	
	free(helper);
}

// this function is called by PA if an interesting event occured
static void pa_subscription_callback(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
	struct crtx_pa_listener *palist;
	pa_operation *op;
	pa_subscription_event_type_t ev_op;
	struct generic_pa_callback_helper *helper;
	
	
	palist = (struct crtx_pa_listener*) userdata;
	ev_op = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
	
	if (ev_op == PA_SUBSCRIPTION_EVENT_REMOVE) {
		helper = (struct generic_pa_callback_helper*) malloc(sizeof(struct generic_pa_callback_helper));
		helper->type = t;
		helper->palist = palist;
		helper->index = index;
		
		generic_pa_get_info_callback(palist->context, 0, 0, helper);
		
		return;
	}
	
	helper = (struct generic_pa_callback_helper*) malloc(sizeof(struct generic_pa_callback_helper));
	helper->type = t;
	helper->palist = palist;
	
	switch ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)) {
		case PA_SUBSCRIPTION_EVENT_CARD:
			op = pa_context_get_card_info_by_index(palist->context, index, (pa_card_info_cb_t) &generic_pa_get_info_callback, helper);
			if (!op) {
				printf("error pa_context_get_card_info_list\n");
			}
			pa_operation_unref(op);
			break;
		
		case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
			op = pa_context_get_sink_input_info(palist->context, index, (pa_sink_input_info_cb_t) &generic_pa_get_info_callback, helper);
			if (!op) {
				printf("pa_context_get_sink_input_info() failed");
			}
			pa_operation_unref(op);
			break;
		
		default:
			ERROR("TODO no handler yet for type %d\n", t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK);
			free(helper);
			break;
	}
}

// TODO change from thread-loop to epoll fd
// static int pa_poll_func(struct pollfd *ufds, unsigned long nfds, int timeout, void *userdata) {
// }

static void * pa_tmain(void *data) {
	struct crtx_pa_listener *palist;
	
	
	palist = (struct crtx_pa_listener *) data;
	
	while (!palist->parent.thread->stop) {
		pa_mainloop_iterate(palist->mainloop, 1, 0);
	}
	
	return 0;
}

// function called by PA when our connection state to PA changes
static void connect_state_cb(pa_context* context, void* raw) {
	struct crtx_pa_listener *palist;
	
	palist = (struct crtx_pa_listener*) raw;
	palist->state = pa_context_get_state(context);
}

static void crtx_free_pa_listener(struct crtx_listener_base *data, void *userdata) {
	struct crtx_pa_listener *palist;
	
	palist = (struct crtx_pa_listener*) data;
	
	pa_mainloop_wakeup(palist->mainloop);
	
	wait_on_signal(&palist->parent.thread->finished);
	
	pa_context_unref(palist->context);
	pa_mainloop_free(palist->mainloop);
}

static void wait_for_op_result(struct crtx_pa_listener *palist, pa_operation* op) {
	int r;
	
	while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
		pa_mainloop_iterate(palist->mainloop, 1, &r);
	}
	
	pa_operation_unref(op);
}

struct crtx_listener_base *crtx_new_pa_listener(void *options) {
	struct crtx_pa_listener *palist;
	pa_operation *op;
	
	
	palist = (struct crtx_pa_listener *) options;
	
	palist->state = PA_CONTEXT_CONNECTING;
	palist->mainloop = pa_mainloop_new();
	palist->context = pa_context_new_with_proplist(
											pa_mainloop_get_api(palist->mainloop),
											0,
											palist->client_proplist
										);
	if (!palist->context) {
		ERROR("pa_context_new_with_proplist failed\n");
		return 0;
	}
	
	pa_context_set_state_callback(palist->context, connect_state_cb, palist);
	pa_context_connect(palist->context, 0, PA_CONTEXT_NOFLAGS, 0);
	
	while (palist->state != PA_CONTEXT_READY && palist->state != PA_CONTEXT_FAILED) {
		pa_mainloop_iterate(palist->mainloop, 1, 0);
	}
	
	if (palist->state != PA_CONTEXT_READY) {
		ERROR("failed to connect to pulse daemon: %s\n",
				pa_strerror(pa_context_errno(palist->context)));
		return 0;
	}
	
	pa_context_set_subscribe_callback(palist->context, &pa_subscription_callback, palist);
	op = pa_context_subscribe(palist->context,
							palist->subscription_mask,
							0, 
							0);
	if (op) {
		wait_for_op_result(palist, op);
	} else {
		ERROR("pa_context_subscribe\n");
		return 0;
	}
	
	palist->parent.free = &crtx_free_pa_listener;
// 	new_eventgraph(&palist->parent.graph, "pulseaudio", 0);
	
	// TODO set own poll function that wraps our epoll listener
	// 	pa_mainloop_set_poll_func(palist->mainloop, pa_poll_func, data);
	palist->parent.thread = get_thread(pa_tmain, palist, 0);
	
	return &palist->parent;
}

void crtx_pa_init() {
}

void crtx_pa_finish() {
}


#ifdef CRTX_TEST

static char pa_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_print_dict(event->data.dict);
	
	return 0;
}

int pa_main(int argc, char **argv) {
	struct crtx_pa_listener palist;
	struct crtx_listener_base *lbase;
	char ret;
	
// 	crtx_root->no_threads = 1;
	
	memset(&palist, 0, sizeof(struct crtx_pa_listener));
	
	
	palist.client_proplist = pa_proplist_new();
	pa_proplist_sets(palist.client_proplist, PA_PROP_APPLICATION_NAME, "pulseaudio.test");
	pa_proplist_sets(palist.client_proplist, PA_PROP_APPLICATION_ID, "org.cortexd.pulseaudio.test");
	pa_proplist_sets(palist.client_proplist, PA_PROP_APPLICATION_VERSION, "0.1");
// 	pa_proplist_sets(palist.client_proplist, PA_PROP_APPLICATION_ICON_NAME, "audio-card");
	
	palist.subscription_mask = PA_SUBSCRIPTION_MASK_SINK_INPUT | PA_SUBSCRIPTION_MASK_CLIENT;
	
	lbase = create_listener("pulseaudio", &palist);
	if (!lbase) {
		ERROR("create_listener(pulseaudio) failed\n");
		exit(1);
	}
	
	pa_proplist_free(palist.client_proplist);
	
	crtx_create_task(lbase->graph, 0, "pa_test", pa_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting pulseaudio listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
// 	crtx_free_listener(lbase);
	
	return 0;  
}

CRTX_TEST_MAIN(pa_main);

#endif
