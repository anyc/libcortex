/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>

#include <linux/limits.h>

#include "intern.h"
#include "core.h"
#include "dict.h"
#include "dict_inout.h"

static void inspect_obj(json_object * jobj, struct crtx_dict *dict);

void inspect_primitive(json_object *jobj, struct crtx_dict_item *ditem){
	enum json_type type;
	
	type = json_object_get_type(jobj);
	switch (type) {
		case json_type_boolean:
			ditem->type = 'i';
			ditem->size = sizeof(int32_t);
			ditem->int32 = json_object_get_boolean(jobj);
			break;
		case json_type_double:
// 			printf("TODO double %lf", json_object_get_double(jobj));
			ditem->type = 'd';
			ditem->size = sizeof(double);
			ditem->double_fp = json_object_get_double(jobj);
			break;
		case json_type_int:
			ditem->type = 'I';
			ditem->size = sizeof(int64_t);
			ditem->int64 = json_object_get_int64(jobj);
			break;
		case json_type_string: 
			ditem->type = 's';
			ditem->size = 0;
			ditem->string = crtx_stracpy(json_object_get_string(jobj), &ditem->size);
			ditem->size++;
			break;
		default:break;
	}
}


void inspect_array(json_object *jobj, struct crtx_dict *dict) {
	enum json_type type;
	struct crtx_dict_item *ditem;
	int i;
	json_object * jvalue;
	
	for (i=0; i < json_object_array_length(jobj); i++){
		jvalue = json_object_array_get_idx(jobj, i);
		type = json_object_get_type(jvalue);
		ditem = crtx_alloc_item(dict);
		ditem->key = 0;
		
		if (type == json_type_array) {
			ditem->type = 'D';
			ditem->dict = crtx_init_dict(0, 0, 0);
			
			inspect_array(jvalue, ditem->dict);
		} else
		if (type == json_type_object) {
			ditem->type = 'D';
			ditem->dict = crtx_init_dict(0, 0, 0);
			
			inspect_obj(jvalue, ditem->dict);
		} else {
			inspect_primitive(jvalue, ditem);
		}
	}
	
// 	if (ditem)
// 		ditem->flags |= CRTX_DIF_LAST_ITEM;
}

static void inspect_obj(json_object * jobj, struct crtx_dict *dict) {
	enum json_type type;
	struct crtx_dict_item *ditem;
	
	json_object_object_foreach(jobj, key, val) {
		type = json_object_get_type(val);
		
		ditem = crtx_alloc_item(dict);
		
		if (key) {
			ditem->flags = CRTX_DIF_ALLOCATED_KEY;
			ditem->key = crtx_stracpy(key, 0);
		}
		
		switch (type) {
			case json_type_boolean:
			case json_type_double:
			case json_type_int:
			case json_type_string:
				inspect_primitive(val, ditem);
				break;
			
			case json_type_object:
				ditem->type = 'D';
				ditem->dict = crtx_init_dict(0, 0, 0);
				
				inspect_obj(val, ditem->dict);
				break;
			case json_type_array:
				ditem->type = 'D';
				ditem->dict = crtx_init_dict(0, 0, 0);
				
				inspect_array(val, ditem->dict);
				break;
			default:break;
		}
	}
}

void crtx_dict_parse_json(struct crtx_dict *dict, char *string) {
	enum json_tokener_error error;
	
	json_object * jobj = json_tokener_parse_verbose(string, &error);
	
	if (!jobj) {
		CRTX_ERROR("parsing json failed:\n%s\n-----\n%s", json_tokener_error_desc(error), string);
		return;
	}
	
	inspect_obj(jobj, dict);
	
	json_object_put(jobj);
}

char crtx_dict_json_from_file(struct crtx_dict **dict, char *dictdb_path, char *id) {
	char *s;
	char file_path[PATH_MAX];
	char *path;
	
	if (!dictdb_path)
		dictdb_path = CRTX_DICT_DB_PATH;
	
	path = getenv("CRTX_DICTDB_PATH");
	if (path)
		dictdb_path = path;
	
	snprintf(file_path, PATH_MAX, "%s%s.json", dictdb_path?dictdb_path:"", id);
	
	s = crtx_readfile(file_path);
	if (!s)
		return -1;
	
	if (!*dict)
		*dict = crtx_init_dict(0, 0, 0);
	
// 	#ifdef WITH_JSON
	crtx_dict_parse_json(*dict, s);
// 	#endif
	
// 	crtx_print_dict(*dict);
	
	free(s);
	
	return 0;
}
