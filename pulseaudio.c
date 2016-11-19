
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pulse/introspect.h>

#include "pulseaudio.h"


#include "pulseaudio_2dict.c"

char *crtx_pa_msg_etype[] = { "pulseaudio/event", 0 };

char *crtx_pa_subscription_etypes[] = {
	[PA_SUBSCRIPTION_EVENT_SINK_INPUT] = "pulseaudio/sink_input",
};


// char crtx_pa_raw2dict_sink_input(struct crtx_dict **dictionary, pa_sink_input_info *info) {
// 	struct crtx_dict *dict;
// 	struct crtx_dict_item *di, *sdi;
// 	char **a;
// 	
// 	if (!dictionary)
// 		return 1;
// 	
// 	if (*dictionary) {
// 		dict = *dictionary;
// 	} else {
// 		dict = crtx_init_dict(0, 0, 0);
// 		*dictionary = dict;
// 	}
// 	
// 	// store info->name as dict payload?
// 	crtx_append_to_dict(&dict, "us",
// 			"index", info->index, sizeof(info->index), 0,
// 			"name", info->name, 0, DIF_COPY_STRING,
// 			"owner_module", info->owner_module, sizeof(info->owner_module), 0,
// 			"client", info->client, sizeof(info->client), 0,
// 			"sink", info->sink, sizeof(info->sink), 0,
// 			// sample_spec
// 			// channel map
// 			// volume
// 			// buffer_usec
// 			// sink_usec
// 			"resample_method", info->resample_method, 0, DIF_COPY_STRING,
// 			"driver", info->driver, 0, DIF_COPY_STRING,
// 			"mute", info->mute, sizeof(info->mute), 0,
// 			// proplist
// 			"corked", info->corked, sizeof(info->corked), 0,
// 			"has_volume", info->has_volume, sizeof(info->has_volume), 0,
// 			"volume_writable", info->volume_writable, sizeof(info->volume_writable), 0,
// 			// format
// 		);
// 		
// 	// get only requested or all attributes
// 	if (r2ds && (r2ds->subsystem || r2ds->device_type)) {
// 		i = r2ds;
// 		while (i->subsystem || i->device_type) {
// 			subsystem = udev_device_get_subsystem(dev);
// 			device_type = udev_device_get_devtype(dev);
// 			
// 			if (strcmp(subsystem, i->subsystem) || strcmp(device_type, i->device_type)) {
// 				new_dev = udev_device_get_parent_with_subsystem_devtype(dev, i->subsystem, i->device_type);
// 				
// 				if (new_dev)
// 					dev = new_dev;
// 				else
// 					continue;
// 			}
// 			
// 			di = crtx_alloc_item(dict);
// 			di->type = 'D';
// 			
// 			di->key = malloc( (i->subsystem?strlen(i->subsystem):0) + 1 + (i->device_type?strlen(i->device_type):0) + 1);
// 			sprintf(di->key, "%s-%s", i->subsystem?i->subsystem:"", i->device_type?i->device_type:"");
// 			di->flags |= DIF_KEY_ALLOCATED;
// 			di->ds = crtx_init_dict(0, 0, 0);
// 			
// 			a = i->attributes;
// 			while (*a) {
// 				value = udev_device_get_sysattr_value(dev, *a);
// 				if (value) {
// 					sdi = crtx_alloc_item(di->ds);
// 					crtx_fill_data_item(sdi, 's', *a, value, strlen(value), DIF_DATA_UNALLOCATED);
// 				}
// 				
// 				a++;
// 			}
// 			
// 			i++;
// 		}
// 	} else {
// 		struct udev_list_entry *entry;
// 		const char *key;
// 		
// 		udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(dev)) {
// 			key = udev_list_entry_get_name(entry);
// 			value = udev_list_entry_get_value(entry);
// 			
// 			di = crtx_alloc_item(dict);
// 			if (value)
// 				crtx_fill_data_item(di, 's', key, value, strlen(value), DIF_DATA_UNALLOCATED);
// 		}
// 	}
// 	
// 	return 0;
// }


struct generic_callback_helper {
	enum pa_subscription_event_type type;
	struct crtx_pa_listener *palist;
	uint32_t index;
};

void generic_callback(pa_context *c, void *info, int eol, void *userdata) {
	struct generic_callback_helper *helper;
	struct crtx_event *nevent;
	struct crtx_dict_item *di;
	pa_subscription_event_type_t ev_op;
	
	
	helper = (struct generic_callback_helper *) userdata;
	
	if (eol < 0) {
		fprintf(stderr, "%s error in %s: \n", __func__,
				pa_strerror(pa_context_errno(c)));
		
		// TODO ?
		if (pa_context_errno(c) == PA_ERR_NOENTITY)
			return;
		
		return;
	}
	if (eol > 0)
		return;
	
	
	nevent = create_event(crtx_pa_subscription_etypes[helper->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK], 0, 0);
// 	nevent->data.raw.flags |= DIF_DATA_UNALLOCATED;
	
// 	nevent->cb_before_release = &udev_event_before_release_cb;
// 	reference_event_release(nevent);
	
	nevent->data.dict = crtx_init_dict(0, 0, 0);

	
	di = crtx_alloc_item(nevent->data.dict);
	
	ev_op = helper->type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
	
	if (ev_op == PA_SUBSCRIPTION_EVENT_REMOVE) {
		crtx_fill_data_item(di, 's', "operation", "remove", strlen("remove"), DIF_DATA_UNALLOCATED);
	} else {
		if (ev_op == PA_SUBSCRIPTION_EVENT_CHANGE)
			crtx_fill_data_item(di, 's', "operation", "change", strlen("change"), DIF_DATA_UNALLOCATED);
		else if (ev_op == PA_SUBSCRIPTION_EVENT_NEW)
			crtx_fill_data_item(di, 's', "operation", "new", strlen("new"), DIF_DATA_UNALLOCATED);
		
		switch ((helper->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)) {
			case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
				if (ev_op == PA_SUBSCRIPTION_EVENT_NEW) {
// 					crtx_pa_raw2dict_sink_input(&nevent->data.dict, (pa_sink_input_info*) info);
					crtx_pa_sink_input_info2dict(info, &nevent->data.dict);
				};
				break;
		}
	}
	
	add_event(helper->palist->parent.graph, nevent);
}

static void pa_subscription_callback(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
	struct crtx_pa_listener *palist;
	pa_operation *op;
	pa_subscription_event_type_t ev_op;
	struct generic_callback_helper *helper;
	
	palist = (struct crtx_pa_listener*) userdata;
	ev_op = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
	
	if (ev_op == PA_SUBSCRIPTION_EVENT_REMOVE) {
		helper = (struct generic_callback_helper*) malloc(sizeof(struct generic_callback_helper));
		helper->type = t;
		helper->palist = palist;
		helper->index = index;
		
		generic_callback(palist->context, 0, 0, helper);
		
		return;
	}
	
// 	PA_SUBSCRIPTION_EVENT_NEW
// 	PA_SUBSCRIPTION_EVENT_CHANGE
	
	helper = (struct generic_callback_helper*) malloc(sizeof(struct generic_callback_helper));
	helper->type = t;
	helper->palist = palist;
	
	switch ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)) {
		case PA_SUBSCRIPTION_EVENT_CARD:
// 			if (ev_op == PA_SUBSCRIPTION_EVENT_NEW) {
				op = pa_context_get_card_info_by_index(palist->context, index, (pa_card_info_cb_t) &generic_callback, helper);
				if (!op) {
					printf("error pa_context_get_card_info_list\n");
				}
				pa_operation_unref(op);
// 			}
			break;
		
		case PA_SUBSCRIPTION_EVENT_SOURCE:
// 			if (ev_op == PA_SUBSCRIPTION_EVENT_NEW) {
				op = pa_context_get_source_info_by_index(palist->context, index, (pa_source_info_cb_t) &generic_callback, helper);
				if (!op) {
					printf("error pa_context_get_source_info_list\n");
				}
				pa_operation_unref(op);
// 			}
			break;
		
		case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
// 			if (ev_op == PA_SUBSCRIPTION_EVENT_NEW) {
				op = pa_context_get_sink_input_info(palist->context, index, (pa_sink_input_info_cb_t) &generic_callback, helper);
				if (!op) {
					printf("pa_context_get_sink_input_info() failed");
				}
				pa_operation_unref(op);
// 			}
			break;
	}
}

// static int pa_poll_func(struct pollfd *ufds, unsigned long nfds, int timeout, void *userdata) {
// 	
// }

static void * pa_tmain(void *data) {
	struct crtx_pa_listener *palist;
	
	
	palist = (struct crtx_pa_listener *) data;
	
	while (1) {
		pa_mainloop_iterate(palist->mainloop, 1, 0);
	}
	
	return 0;
}

static void connect_state_cb(pa_context* context, void* raw) {
	struct crtx_pa_listener *palist;
	
	palist = (struct crtx_pa_listener*) raw;
	palist->state = pa_context_get_state(context);
}

void crtx_free_pa_listener(struct crtx_listener_base *data) {
	struct crtx_pa_listener *palist;
	
	palist = (struct crtx_pa_listener*) data;
	
	pa_context_unref(palist->context);
	pa_mainloop_free(palist->mainloop);
}

void wait_for_op_result(struct crtx_pa_listener *palist, pa_operation* op) {
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
	new_eventgraph(&palist->parent.graph, "pulseaudio", crtx_pa_msg_etype);
	
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
// 	struct crtx_dict *dict;
// 	struct crtx_pa_listener *palist;
	
	
// 	palist = (struct crtx_pa_listener *) event->origin;
	
// 	sd_bus_message *m = event->data.raw.pointer;
// 	sd_bus_print_msg(m);
	crtx_print_dict(event->data.dict);
	
// 	crtx_free_dict(dict);
	
	return 0;
}

int pa_main(int argc, char **argv) {
	struct crtx_pa_listener palist;
	struct crtx_listener_base *lbase;
	char ret;
	
	crtx_root->no_threads = 1;
	
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
	
	free_listener(lbase);
	
	return 0;  
}

CRTX_TEST_MAIN(pa_main);

#endif
