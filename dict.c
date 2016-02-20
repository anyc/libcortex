
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "core.h"
#include "dict.h"


char crtx_fill_data_item_va(struct crtx_dict_item *di, char type, va_list va) {
	// 	if (ds) {
	// 		size_t idx;
	// 		
	// 		idx = di - ds->items;
	// 		printf("idx %zu\n", idx);
	// 		if (ds->signature[idx] != type) {
	// 			printf("type mismatch %c %c\n", ds->signature[idx], type);
	// 			return -1;
	// 		}
	// 	}
	
	di->type = type;
	di->key = va_arg(va, char*);
	
	switch (di->type) {
		case 'i':
		case 'u':
			di->uint32 = va_arg(va, uint32_t);
			
			di->size = va_arg(va, size_t);
			if (di->size > 0 && di->size != sizeof(uint32_t)) {
				ERROR("size mismatch %zu %zu\n", di->size, sizeof(uint32_t));
				return 0;
			}
			break;
		case 's':
			di->string = va_arg(va, char*);
			
			di->size = va_arg(va, size_t) + 1; // add space for \0 delimiter
			break;
		case 'D':
			di->ds = va_arg(va, struct crtx_dict*);
			di->size = va_arg(va, size_t);
			break;
		default:
			ERROR("unknown literal '%c'\n", di->type);
			return -1;
	}
	
	di->flags = va_arg(va, int);
	
	return 0;
}

char crtx_fill_data_item(struct crtx_dict_item *di, char type, ...) {
	va_list va;
	char ret;
	
	va_start(va, type);
	
	ret = crtx_fill_data_item_va(di, type, va);
	
	va_end(va);
	
	return ret;
}

struct crtx_dict_item * crtx_alloc_item(struct crtx_dict *dict) {
// 	dict->size += sizeof(struct crtx_dict_item);
	dict->signature_length++;
	dict->items = (struct crtx_dict_item*) realloc(dict->items, dict->signature_length*sizeof(struct crtx_dict_item));
	
	// TODO payload
// 	return (struct crtx_dict_item *) ((char*)dict) + dict->size - sizeof(struct crtx_dict_item);
	
	memset(&dict->items[dict->signature_length-1], 0, sizeof(struct crtx_dict_item));
	return &dict->items[dict->signature_length-1];
}

struct crtx_dict * crtx_init_dict(char *signature, size_t payload_size) {
	struct crtx_dict *ds;
	size_t size;
	
	size = sizeof(struct crtx_dict) + payload_size;
	
	if (signature)
		size += strlen(signature) * sizeof(struct crtx_dict_item);
	
	ds = (struct crtx_dict*) malloc(size);
	
	ds->signature = signature;
	ds->size = size;
	
	if (signature) {
		ds->signature_length = strlen(signature);
		ds->items = (struct crtx_dict_item *) ((char*) ds + sizeof(struct crtx_dict));
	} else {
		ds->signature_length = 0;
		ds->items = 0;
	}
	
	return ds;
}

struct crtx_dict * crtx_create_dict(char *signature, ...) {
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	char *s, ret;
	va_list va;
	
	
	ds = crtx_init_dict(signature, 0);
	
	va_start(va, signature);
	
	s = signature;
	di = ds->items;
	while (*s) {
		ret = crtx_fill_data_item_va(di, *s, va);
		if (ret < 0)
			return 0;
		
		di++;
		s++;
		ds->signature_length++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	va_end(va);
	
	return ds;
}


void free_dict(struct crtx_dict *ds) {
	struct crtx_dict_item *di;
// 	char *s;
	size_t i;
	
	if (!ds)
		return;
	
// 	s = ds->signature;
	di = ds->items;
// 	while (*s) {
	for (i=0; i < ds->signature_length; i++) {
		if (di->flags & DIF_KEY_ALLOCATED)
			free(di->key);
		
		switch (di->type) {
			case 's':
				if (!(di->flags & DIF_DATA_UNALLOCATED))
					free(di->string);
				break;
			case 'D':
				free_dict(di->ds);
				break;
		}
		
		if (DIF_IS_LAST(di))
			break;
		
		di++;
// 		s++;
	}
	
	free(ds);
}

void crtx_print_dict_rec(struct crtx_dict *ds, unsigned char level) {
	char *s;
	struct crtx_dict_item *di;
	uint8_t i,j;
	
	if (!ds) {
		ERROR("error, trying to print non-existing dict\n");
		return;
	}
	
// 	for (j=0;j<level-1;j++) INFO("   ");
	INFO("(dict) %s (%zu==%u)\n", ds->signature?ds->signature:"\"\"", ds->signature?strlen(ds->signature):0, ds->signature_length);
	
	i=0;
	s = ds->signature;
	di = ds->items;
	while ( s?*s:1 && (!s)? i<ds->signature_length:1 ) {
		for (j=0;j<level;j++) INFO("   "); // indent
		INFO("%u: %s = ", i, di->key?di->key:"\"\"");
		
		if (s && *s != di->type)
			INFO("error type %c != %c\n", *s, di->type);
		
		switch (di->type) {
			case 'u': INFO("(uint32_t) %d\n", di->uint32); break;
			case 'i': INFO("(int32_t) %d\n", di->int32); break;
			case 's': INFO("(char*) %s\n", di->string); break;
			case 'D':
// 				INFO("(dict) \n");
				if (di->ds)
					crtx_print_dict_rec(di->ds, level+1);
				break;
		}
		
		if (s) s++;
		di++;
		i++;
	}
}

void crtx_print_dict(struct crtx_dict *ds) {
	crtx_print_dict_rec(ds, 1);
}

char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size) {
	switch (di->type) {
		case 'u':
		case 'i':
			if (buffer_size == sizeof(uint32_t)) {
				memcpy(buffer, &di->uint32, buffer_size);
				return 1;
			}
		case 's':
		case 'D':
			if (buffer_size == sizeof(void*)) {
				memcpy(buffer, &di->pointer, buffer_size);
				return 1;
			} else
				ERROR("error, size does not match type: %zu != %zu\n", buffer_size, sizeof(void*));
			break;
		default:
			ERROR("error, unimplemented value type %c\n", di->type);
			return 0;
	}
	
	return 0;
}

struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds) {
	return ds?ds->items:0;
}

struct crtx_dict_item *crtx_get_next_item(struct crtx_dict *ds, struct crtx_dict_item *di) {
	return di+1;
}

struct crtx_dict_item * crtx_get_item(struct crtx_dict *ds, char *key) {
// 	char *s;
	uint32_t i;
	struct crtx_dict_item *di;
	
	if (!ds)
		return 0;
	
// 	s = ds->signature;
	di = ds->items;
// 	while (*s) {
	for (i=0; i < ds->signature_length; i++) {
		if (di->key && !strcmp(di->key, key)) {
			return di;
		}
		
// 		s++;
		di++;
	}
	
	return 0;
}

struct crtx_dict_item * crtx_get_item_by_idx(struct crtx_dict *ds, size_t idx) {
	return &ds->items[idx];
}

char crtx_get_value(struct crtx_dict *ds, char *key, char type, void *buffer, size_t buffer_size) {
	struct crtx_dict_item *di;
	
	di = crtx_get_item(ds, key);
	if (!di)
		return 0;
	
	if (type > 0 && di->type != type) {
		ERROR("type mismatch for \"%s\": %c != %c\n", key, type, di->type);
		return 0;
	}
	
	return crtx_copy_value(di, buffer, buffer_size);
}

struct crtx_dict *crtx_get_dict(struct crtx_dict *ds, char *key) {
	struct crtx_dict *dict;
	char ret;
	
	ret = crtx_get_value(ds, key, 'D', &dict, sizeof(struct crtx_dict *));
	return ret?dict:0;
}

char *crtx_get_string(struct crtx_dict *ds, char *key) {
	char *s;
	char ret;
	
	ret = crtx_get_value(ds, key, 's', &s, sizeof(char *));
	return ret?s:0;
}

/**
 * parse format string and substitute %-specifiers with values from the dictionary
 * %[key]item_type %[key](format string for sub-dictionary)
 * E.g.: "Event \"%[my_array](%[* ]s)\" for file \"%[name]s\""
 */
static char dict_printf(struct crtx_dict *ds, char *format, char **result, size_t *rlen, size_t *alloc) {
	char *f, *e;
	char *key;
	char *s;
	size_t len;
	char *fmt;
// 	char buf[32]; // assume primitive data types fit in 32 chars
	unsigned char min, keylen;
	struct crtx_dict_item *di;
	
	
	f = format;
	while (*f) {
		if (*f == '%' && *(f+1) != '%') {
			/*
			 * first, get the [key]
			 */
			
			if (*(f+1) != '[') {
				ERROR("character after %% neither %% nor [: %c\n", *(f+1));
				return 0;
			}
			
			e = f;
			while (*e != ']')
			{
				e++;
			}
			keylen = e - (f+2);
			e++;
			
			key = (char*) malloc(keylen+1);
			strncpy(key, f+2, keylen);
			key[keylen] = 0;
			
			// do we print specific item referenced by key or all items (*)?
			if (*key != '*')
				di = crtx_get_item(ds, key);
			else
				di = &ds->items[0];
			
			
			// check if this part of the format string contains a sub-group
			if (*e == '(') {
				/*
				 * key refers to another dictionary, starting recursion with
				 * format string "fmt" from "%[key](fmt)"
				 */
				
				char *fmt_start;
				
				fmt_start = e;
				while (*e != ')')
				{
					e++;
				}
				min = e - (fmt_start+1);
				
				fmt = (char*) malloc(min+1);
				strncpy(fmt, fmt_start+1, min);
				fmt[min] = 0;
				
				char ret;
				ret = dict_printf(di->ds, fmt, result, rlen, alloc);
				
				free(fmt);
				if (!ret)
					return 0;
			} else {
				uint32_t i;
				
// 				while (*e != 's' &&
// 					*e != 'd' &&
// 					*e != 'D' &&
// 					*e != 'u')
// 				{
// 					e++;
// 				}
// 				min = e - f + 1;
// 				
// 				fmt = (char*) malloc(min+1);
// 				strncpy(fmt, f, min);
// 				fmt[min] = 0;
				
				switch (*e) {
					case 's':
						i = 0;
						do {
							if (di->type != 's') {
								ERROR("unexpected type in array of strings: %c\n", di->type);
								return 0;
							}
							
							s = di->string;
							len = strlen(s);
							
							*alloc += len + 1;
							*result = (char*) realloc(*result, *alloc);
							
							*rlen += sprintf( &((*result)[*rlen]), "%s", s);
							
							// add separator, if present
							if (keylen > 1 && i < ds->signature_length-1)
								*rlen += sprintf( &((*result)[*rlen]), "%s", &key[1]);
							
// 							di = crtx_get_next_item(di);
							i++;
							di = &ds->items[i];
						} while (*key == '*' && i < ds->signature_length);
						
						break;
					case 'd':
					case 'u':
						ERROR("TODO\n");
// 						if (!di || (di->type != 's' && di->type != 'u')) {
// 							ERROR("get_value failed\n");
// 							return 0;
// 						}
// 						
// 						snprintf(buf, 32, fmt, di->uint32);
// 						len = strlen(buf);
// 						
// 						*alloc += len;
// 						*result = (char*) realloc(*result, *alloc);
// 						
// 						*rlen += sprintf( &((*result)[*rlen]), "%s", buf);
						
						break;
					default:
						ERROR("%c unknown", *e);
						return 0;
				}
			}
			
			free(key);
			f = e;
		} else {
			/*
			 * copy character
			 */
			
			if (*rlen >= (*alloc)-1) {
				*alloc += 24;
				*result = (char*) realloc(*result, *alloc);
			}
			
			(*result)[*rlen] = *f;
			
			(*rlen)++;
			
			// ignore the second % in a row
			if (*f == '%' && *(f+1) == '%')
				f++;
		}
		f++;
	}
	
	return 1;
}

struct crtx_dict * crtx_dict_transform(struct crtx_dict *dict, char *signature, struct crtx_dict_transformation *transf) {
	struct crtx_dict_transformation *pit;
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	size_t alloc;
	size_t i;
	
	ds = crtx_init_dict(signature, 0);
	
	di = ds->items;
	pit = transf;
// 	for (pit = transf; pit && pit->type; pit++) {
	for (i=0; i<strlen(signature); i++) {
		di->type = pit->type;
		di->key = pit->key;
		
		alloc = 24;
		di->size = 0;
		di->string = (char*) malloc(alloc);
		
		if (pit->type == 's') {
			dict_printf(dict, pit->format, &di->string, &di->size, &alloc);
		} else
			if (pit->type == 'D') {
				
			} else {
				
			}
			
			di->flags = pit->flag;
		
		if (di->size >= alloc-1)
			di->string = (char*) realloc(di->string, alloc+1);
		
		di->string[di->size] = 0;
		
		di++;
		pit++;
// 		ds->signature_length++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	return ds;
}

void crtx_transform_dict_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	struct crtx_event *new_event;
	struct crtx_transform_dict_handler *trans;
	
	if (!event->data.dict)
		event->data.raw_to_dict(&event->data);
	
	if (!event->data.dict) {
		ERROR("cannot convert %s to dict", event->type);
		return;
	}
	
	trans = (struct crtx_transform_dict_handler*) userdata;
	
	dict = crtx_dict_transform(event->data.dict, trans->signature, trans->transformation);
	
	new_event = create_event(trans->type, 0, 0);
	new_event->data.dict = dict;
	
	if (trans->graph)
		add_event(trans->graph, new_event);
	else
		add_raw_event(new_event);
}

struct crtx_task *crtx_create_transform_task(struct crtx_graph *in_graph, char *name, struct crtx_transform_dict_handler *trans) {
	return crtx_create_task(in_graph, 0, name, &crtx_transform_dict_handler, trans);
}
