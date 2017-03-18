
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <inttypes.h>


#include "core.h"
#include "inotify.h"
#include "sd_bus_notifications.h"
#include "dict_inout_json.h"
#include "event_comm.h"
#include "linkedlist.h"
#include "linkedlist.h"


struct crtx_ll *listeners = 0;
struct crtx_ll *transformations = 0;


// execute script with inotify parameters
static char exec_script_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *script;
	char *cmd, *pos;
	struct inotify_event *in_event;
	int ret;
	size_t mask_size, cmd_size;
	
	script = (char *) userdata;
	in_event = (struct inotify_event *) event->data.raw.pointer;
	
	// calculate required string length
	ret = crtx_inotify_mask2string(in_event->mask, 0, &mask_size);
	if (!ret)
		return 1;
	
	cmd_size = strlen(script) + 1 + strlen(in_event->name) + 1 + 10 /*uint32*/ + 1 + mask_size - 1 + 1;
	
	cmd = (char*) malloc(cmd_size);
	
	pos = cmd;
	pos += snprintf(pos, cmd_size, "%s %s %" PRIu32 " ", script, in_event->name, in_event->cookie);
	
	mask_size = cmd_size - (pos - cmd);
	ret = crtx_inotify_mask2string(in_event->mask, pos, &mask_size);
	
	DBG("executing cmd %s\n", cmd);
	system(cmd);
	
	free(cmd);
	
	return 1;
}

char init() {
	struct crtx_dict *config, *monitors, *monitor, *mask, *actions;
	struct crtx_dict *transforms;
	struct crtx_inotify_listener *listener;
	struct crtx_listener_base *in_base;
	uint32_t i,j;
	char *path;
	uint32_t in_mask;
	
	crtx_dict_json_from_file(&config, 0, "control_inotify_batch");
	
	monitors = transforms = 0;
	crtx_get_value(config, "monitors", 'D', &monitors, sizeof(void*));
	crtx_get_value(config, "transforms", 'D', &transforms, sizeof(void*));
	
	if (!monitors) {
		DBG("control_inotify_batch: no monitors in config\n");
		return 1;
	}
	
	for (i=0;i < monitors->signature_length; i++) {
		if (monitors->items[i].type != 'D')
			continue;
		monitor = monitors->items[i].ds;
		
		path = crtx_get_string(monitor, "path");
		mask = crtx_get_dict(monitor, "mask");
		actions = crtx_get_dict(monitor, "actions");
		
		INFO("new monitor for path \"%s\"\n", path);
		
		if (!path || !mask || !actions) {
			ERROR("invalid monitor entry\n");
			continue;
		}
		
		in_mask = 0;
		for (j=0; j < mask->signature_length; j++) {
			if (mask->items[j].type != 's')
				continue;
			in_mask |= crtx_inotify_string2mask(mask->items[j].string);
		}
		
		listener = (struct crtx_inotify_listener *) calloc(1, sizeof(struct crtx_inotify_listener));
		crtx_ll_append_new(&listeners, listener);
		listener->path = path;
		listener->mask = in_mask;
		
		// create a listener (queue) for inotify events
		in_base = create_listener("inotify", listener);
		if (!in_base) {
			printf("cannot create inotify listener\n");
			return 0;
		}
		
		{
			struct crtx_dict *scripts, *transform_actions;
			struct crtx_transform_dict_handler *transformation;
			char *transform_id;
			
			scripts = crtx_get_dict(actions, "scripts");
			transform_actions = crtx_get_dict(actions, "transforms");
			
			if (scripts) {
				for (j=0; j < scripts->signature_length; j++) {
					if (scripts->items[j].type != 's')
						continue;
					
					crtx_create_task(in_base->graph, 0, "exec_script_handler", &exec_script_handler, scripts->items[j].string);
				}
			}
			
			if (transform_actions) {
				for (j=0; j < transform_actions->signature_length; j++) {
					struct crtx_dict *transform_def, *transform_rules;
					uint32_t k;
					struct crtx_dict_transformation *t;
					
					if (transform_actions->items[j].type != 's')
						continue;
					
					transform_id = transform_actions->items[j].string;
					
					transformation = (struct crtx_transform_dict_handler *) malloc(sizeof(struct crtx_transform_dict_handler));
					crtx_ll_append_new(&transformations, transformation);
					
					transform_def = crtx_get_dict(transforms, transform_id);
					
					transformation->graph = 0; // TODO
					transformation->type = crtx_get_string(transform_def, "etype");
					transformation->transformation = 0;
					
					transform_rules = crtx_get_dict(transform_def, "rules");
					
					transformation->signature = (char*) malloc(transform_rules->signature_length+1);
					transformation->transformation = (struct crtx_dict_transformation *) calloc(1, sizeof(struct crtx_dict_transformation)*transform_rules->signature_length);
					
					for (k=0; k < transform_rules->signature_length; k++) {
						char *key, *type, *flag, *format;
						struct crtx_dict *trule;
						
						if (transform_rules->items[k].type != 'D')
							continue;
						
						trule = transform_rules->items[k].ds;
						
						key = crtx_get_string(trule, "key");
						type = crtx_get_string(trule, "type");
						flag = crtx_get_string(trule, "flag");
						format = crtx_get_string(trule, "format");
						
						if (!key || !type || !flag || !format) {
							ERROR("invalid transformation rule\n");
							crtx_print_dict(transform_rules);
							continue;
						}
						
						switch (type[0]) {
							case 's':
							case 'D':
								break;
							default:
								ERROR("invalid transformation rule: unknown type %c\n", type[0]);
								continue;
						}
						
						t = &transformation->transformation[k];
						
						t->key = key;
						t->type = type[0];
						transformation->signature[k] = t->type;
						t->flag = 0; // TODO
						t->format = format;
					}
					transformation->signature[k] = 0;
					
					crtx_create_transform_task(in_base->graph, "transform_task_handler", transformation);
				}
			}
		}
		
		crtx_start_listener(in_base);
	}
	
	return 1;
}

void finish() {
	struct crtx_ll *llit, *llitn;
	
	for (llit=transformations;llit;llit=llitn) {
		llitn = llit->next;
		
		free(llit->transform_handler->signature);
		free(llit->transform_handler->transformation);
		free(llit);
	}
	
	for (llit=listeners;llit;llit=llitn) {
		llitn = llit->next;
		crtx_free_listener(&llit->in_listener->parent);
		free(llit);
	}
}
