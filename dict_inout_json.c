
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>

#include "core.h"
#include "dict.h"

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
			ditem->string = stracpy(json_object_get_string(jobj), &ditem->size);
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
		
		if (type == json_type_array) {
			ditem->type = 'D';
			ditem->ds = crtx_init_dict(0, 0, 0);
			
			inspect_array(jvalue, ditem->ds);
		} else
		if (type == json_type_object) {
			ditem->type = 'D';
			ditem->ds = crtx_init_dict(0, 0, 0);
			
			inspect_obj(jvalue, ditem->ds);
		} else {
			inspect_primitive(jvalue, ditem);
		}
	}
	
// 	if (ditem)
// 		ditem->flags |= DIF_LAST;
}

static void inspect_obj(json_object * jobj, struct crtx_dict *dict) {
	enum json_type type;
	struct crtx_dict_item *ditem;
	
	json_object_object_foreach(jobj, key, val) {
		type = json_object_get_type(val);
		
		ditem = crtx_alloc_item(dict);
		
		if (key) {
			ditem->flags = DIF_KEY_ALLOCATED;
			ditem->key = stracpy(key, 0);
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
				ditem->ds = crtx_init_dict(0, 0, 0);
				
				inspect_obj(val, ditem->ds);
				break;
			case json_type_array:
				ditem->type = 'D';
				ditem->ds = crtx_init_dict(0, 0, 0);
				
				inspect_array(val, ditem->ds);
				break;
			default:break;
		}
	}
}

void crtx_load_dict_json(struct crtx_dict *dict, char *string) {
	enum json_tokener_error error;
// 	json_object * jobj = json_tokener_parse(string);
	json_object * jobj = json_tokener_parse_verbose(string, &error);
	
	if (!jobj) {
		ERROR("parsing json failed:\n%s\n-----\n%s", json_tokener_error_desc(error), string);
		return;
	}
	
	inspect_obj(jobj, dict);
}
