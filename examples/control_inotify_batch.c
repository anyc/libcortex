
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "core.h"
#include "inotify.h"
#include "sd_bus_notifications.h"
#include "dict.h"
#include "event_comm.h"

static struct crtx_listener_base *in_base;
static struct crtx_inotify_listener listener;
static void *notifier_data = 0;


struct crtx_dict_transformation to_notification[] = {
	{ "title", 's', 0, "Inotify" },
	{ "message", 's', 0, "Event \"%[mask](%[*]s)\" for file \"%[name]s\"" }
};

struct crtx_transform_dict_handler transform_task;

struct crtx_config;
typedef char *(*on_new_entry_cb)(struct crtx_config *cfg, char *key, struct crtx_dict *entry);

struct crtx_config {
	char *name;
	struct crtx_dict *data;
	
	on_new_entry_cb new_entry;
};

struct crtx_config config;

char init2() {
	char *path;
	
	printf("starting inotify example plugin\n");
	
	crtx_init_notification_listeners(&notifier_data);
	
	path = getenv("INOTIFY_PATH");
	if (!path)
		path = ".";
	
	// setup inotify
	listener.path = path;
	listener.mask = IN_CREATE | IN_DELETE;
	
	// create a listener (queue) for inotify events
	in_base = create_listener("inotify", &listener);
	if (!in_base) {
		printf("cannot create inotify listener\n");
		return 0;
	}
	
	// setup a task that will process the inotify events
// 	crtx_create_task(in_base->graph, 0, "inotify_event_handler", &inotify_event_handler, in_base);
	
	transform_task.signature = "ss";
	transform_task.transformation = to_notification;
	transform_task.graph = 0;
	transform_task.type = CRTX_EVT_NOTIFICATION;
	
	crtx_create_transform_task(in_base->graph, "inotify_to_notification", &transform_task);
	
	return 1;
}

#ifdef WITH_JSON
#include <json.h>
struct crtx_dict_item * parse_json_value(json_object *jobj, char *key, struct crtx_dict *dict){
	enum json_type type;
	struct crtx_dict_item *ditem;
	
	ditem = crtx_alloc_item(dict);
	
	if (key) {
		ditem->flags = DIF_KEY_ALLOCATED;
		ditem->key = stracpy(key, 0);
	}
	
	type = json_object_get_type(jobj); /*Getting the type of the json object*/
	switch (type) {
		case json_type_boolean: 
			ditem->type = 'i';
			ditem->size = sizeof(int32_t);
			ditem->int32 = json_object_get_boolean(jobj);
			break;
		case json_type_double:
			printf("TODO double %lf", json_object_get_double(jobj));
			break;
		case json_type_int: 
			ditem->type = 'i';
			ditem->size = sizeof(int32_t);
			ditem->int32 = json_object_get_int(jobj);
			break;
		case json_type_string: 
			ditem->type = 's';
			ditem->size = 0;
			ditem->string = stracpy(json_object_get_string(jobj), &ditem->size);
			ditem->size++;
			break;
		default:break;
	}
	
	return ditem;
}

void json_parse(json_object * jobj, struct crtx_dict *dict);

void json_parse_array(json_object *jobj, char *key, struct crtx_dict *dict) {
	enum json_type type;
	struct crtx_dict_item *ditem;

	ditem = 0;
	
	json_object *jarray = jobj; /*Simply get the array*/
	if(key) {
		jarray = json_object_object_get(jobj, key); /*Getting the array if it is a key value pair*/
	}

	int arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
	printf("Array Length: %d\n",arraylen);
	int i;
	json_object * jvalue;

	for (i=0; i< arraylen; i++){
		jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
		type = json_object_get_type(jvalue);
		if (type == json_type_array) {
			ditem = crtx_alloc_item(dict);
			ditem->type = 'D';
			ditem->ds = crtx_init_dict(0, 0);
			
			json_parse_array(jvalue, NULL, ditem->ds);
		}
		else if (type != json_type_object) {
			printf("value[%d]: ",i);
			ditem = parse_json_value(jvalue, 0, dict);
		}
		else {
			json_parse(jvalue, dict);
		}
	}
	
	if (ditem)
		ditem->flags |= DIF_LAST;
}

void json_parse(json_object * jobj, struct crtx_dict *dict) {
	enum json_type type;
	// 	struct crtx_dict *dict2;
	struct crtx_dict_item *ditem;
	
	json_object_object_foreach(jobj, key, val) {
		type = json_object_get_type(val);
		switch (type) {
			case json_type_boolean:
			case json_type_double:
			case json_type_int: 
			case json_type_string: 	printf("string\n");
								parse_json_value(val, key, dict);
								break;
			case json_type_object:printf("obj\n");
								jobj = json_object_object_get(jobj, key);
								
								ditem = crtx_alloc_item(dict);
								
								if (key) {
									ditem->flags = DIF_KEY_ALLOCATED;
									ditem->key = stracpy(key, 0);
								}
								
								ditem->type = 'D';
								ditem->ds = crtx_init_dict(0, 0);
								json_parse(jobj, ditem->ds);
								break;
			case json_type_array:printf("array\n");
								json_parse_array(jobj, key, dict);
								break;
			default:break;
		}
	}
}

void crtx_load_json_config(struct crtx_config *config, char *string) {
	json_object * jobj = json_tokener_parse(string);
	
	config->data = crtx_init_dict(0, 0);
	
	json_parse(jobj, config->data);
}
#endif

char *crtx_readfile(char *path) {
	FILE *f;
	size_t size, read_size;
	char *s;
	
	f = fopen(path, "r");
	if (!f) {
		printf("error reading file \"%s\": %s\n", path, strerror(errno));
		return 0;
	}
	
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	s = (char*) malloc(size + 1);
	read_size = fread(s, 1, size, f);
	if (read_size != size) {
		printf("error while reading file %s\n", path);
		return 0;
	}
	
	fclose(f);
	
	return s;
}

void crtx_load_config(struct crtx_config *config) {
	char *s;
	
	int f = open(config->name, O_RDONLY);
	if (f == -1) {
		printf("error %s\n", strerror(errno));
		return;
	}
	
// 	config->data = recv_dict(crtx_wrapper_read, &f);
	
	s = crtx_readfile(config->name);
	if (!s)
		return;
	
	crtx_load_json_config(config, s);
	
	close(f);
	
	crtx_print_dict(config->data);
}

void crtx_store_config(struct crtx_config *config) {
	int f = open(config->name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
	if (f == -1) {
		printf("error %s\n", strerror(errno));
		return;
	}
	
// 	send_dict(crtx_wrapper_write, &f, d);
	
	close(f);
}

char init() {
	config.name = "testdict";
	crtx_load_config(&config);
	
	return 1;
}

void finish() {
	crtx_store_config(&config);
	
	crtx_finish_notification_listeners(notifier_data);
	
	free_listener(in_base);
}