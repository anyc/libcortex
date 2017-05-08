
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

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
// 			if (di->size > 0 && di->size != sizeof(uint32_t)) {
			
			// TODO smaller types are promoted to int
			if (di->size > 0 && di->size > sizeof(uint32_t)) {
				ERROR("crtx size mismatch for key \"%s\" type '%c': %zu expected: %zu\n", di->key, di->type, di->size, sizeof(uint32_t));
				return 0;
			}
			break;
		case 'I':
		case 'U':
			di->uint64 = va_arg(va, uint64_t);
			
			di->size = va_arg(va, size_t);
			if (di->size > 0 && di->size != sizeof(uint64_t)) {
				ERROR("crtx size mismatch for key \"%s\" type '%c': %zu expected: %zu\n", di->key, di->type, di->size, sizeof(uint64_t));
				return 0;
			}
			break;
		case 'z':
		case 'Z':
			di->sizet = va_arg(va, size_t);
			
			di->size = va_arg(va, size_t);
			if (di->size > 0 && di->size != sizeof(size_t)) {
				ERROR("crtx size mismatch for key \"%s\" type '%c': %zu expected: %zu\n", di->key, di->type, di->size, sizeof(uint64_t));
				return 0;
			}
			break;
		case 'd':
			di->double_fp = va_arg(va, double);
			
			di->size = va_arg(va, size_t);
			if (di->size > 0 && di->size != sizeof(double)) {
				ERROR("crtx size mismatch for key \"%s\" type '%c': %zu expected: %zu\n", di->key, di->type, di->size, sizeof(double));
				return 0;
			}
			break;
		case 's':
			di->string = va_arg(va, char*);
			
			di->size = va_arg(va, size_t) + 1; // add space for \0 delimiter
			break;
		case 'p':
			di->pointer = va_arg(va, void*);
			
			di->size = va_arg(va, size_t);
			break;
		case 'D':
			di->dict = va_arg(va, struct crtx_dict*);
			di->size = va_arg(va, size_t);
			break;
		default:
			ERROR("unknown literal '%c'\n", di->type);
			return -1;
	}
	
	di->flags = va_arg(va, int);
	
	if ((di->flags & CRTX_DIF_CREATE_DATA_COPY) && di->type == 's') {
		di->size = 0;
		di->string = crtx_stracpy(di->string, &di->size);
		di->size += 1; // add space for \0 delimiter
		
		di->flags = (di->flags & (~CRTX_DIF_CREATE_DATA_COPY));
	}
	
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

char crtx_resize_dict(struct crtx_dict *dict, size_t n_items) {
	if (n_items < dict->signature_length) {
		ERROR("shrinking dicts not yet supported\n");
		return 1;
	}
	
// 	printf("resize %d -> %d\n", dict->signature_length, n_items);
	
	if (dict->size && dict->size != sizeof(struct crtx_dict)+dict->signature_length*sizeof(struct crtx_dict_item)) {
		ERROR("TODO handle payload in crtx_resize_dict (%zu != %zu)\n", dict->size, sizeof(struct crtx_dict)+dict->signature_length*sizeof(struct crtx_dict_item));
	}
	
	dict->items = (struct crtx_dict_item*) realloc(dict->items, n_items*sizeof(struct crtx_dict_item));
	
// 	if (dict->signature_length > 0)
		memset(&dict->items[dict->signature_length], 0, sizeof(struct crtx_dict_item) * (n_items - dict->signature_length));
	
	// clear "last" flag
	if (dict->signature_length > 0)
		dict->items[dict->signature_length-1].flags &= ~((char)CRTX_DIF_LAST_ITEM);
	dict->items[n_items-1].flags = CRTX_DIF_LAST_ITEM;
	
	if (dict->signature)
		dict->signature = (char*) realloc(dict->signature, n_items+1);
	dict->signature_length = n_items;
	dict->size = sizeof(struct crtx_dict)+dict->signature_length*sizeof(struct crtx_dict_item);
	
	// TODO payload
	// 	return (struct crtx_dict_item *) ((char*)dict) + dict->size - sizeof(struct crtx_dict_item);
	
	return 0;
}

struct crtx_dict_item * crtx_alloc_item(struct crtx_dict *dict) {
// 	dict->signature_length++;
// 	dict->items = (struct crtx_dict_item*) realloc(dict->items, dict->signature_length*sizeof(struct crtx_dict_item));
// 	
// 	// TODO payload
// // 	return (struct crtx_dict_item *) ((char*)dict) + dict->size - sizeof(struct crtx_dict_item);
// 	
// 	memset(&dict->items[dict->signature_length-1], 0, sizeof(struct crtx_dict_item));
// 	if (dict->signature_length > 1)
// 		dict->items[dict->signature_length-2].flags &= ~((char)CRTX_DIF_LAST_ITEM);
// 	dict->items[dict->signature_length-1].flags = CRTX_DIF_LAST_ITEM;
// 	return &dict->items[dict->signature_length-1];
	
	struct crtx_dict_item *di;
	
	di = crtx_get_first_item(dict);
	
	while (di && di->type != 0) {
		di = crtx_get_next_item(dict, di);
	}
	
	if (!di) {
		crtx_resize_dict(dict, dict->signature_length+1);
		
		return &dict->items[dict->signature_length-1];
	} else {
		return di;
	}
}

struct crtx_dict * crtx_init_dict(char *signature, uint32_t sign_length, size_t payload_size) {
	struct crtx_dict *ds;
	size_t size;
	
	size = sizeof(struct crtx_dict) + payload_size;
	
	if (sign_length == 0 && signature)
		sign_length = strlen(signature);
	
	ds = (struct crtx_dict*) calloc(1, size);
	
	if (sign_length)
		size += sign_length * sizeof(struct crtx_dict_item);
	
	ds->signature = signature;
	ds->signature_length = sign_length;
	ds->size = size;
	ds->ref_count = 1;
	INIT_MUTEX(ds->mutex);
	
	if (sign_length) {
// 		ds->items = (struct crtx_dict_item *) ((char*) ds + sizeof(struct crtx_dict));
		ds->items = (struct crtx_dict_item *) calloc(1, sign_length * sizeof(struct crtx_dict_item));
		
		ds->items[sign_length-1].flags |= CRTX_DIF_LAST_ITEM;
	}
	
	return ds;
}

// char crtx_add_item(struct crtx_dict **dict, char type, ...) {
// 	char ret;
// 	va_list va;
// 	struct crtx_dict_item *item;
// 	
// 	if (!*dict) {
// 		*dict = crtx_init_dict(0, 1, 0);
// 		item = crtx_get_first_item(*dict);
// 	} else {
// 		item = crtx_alloc_item(*dict);
// 	}
// 	
// 	if (!item)
// 		return 0;
// 	
// 	va_start(va, type);
// 	
// 	ret = crtx_fill_data_item_va(item, type, va);
// 	
// 	va_end(va);
// 	
// 	return ret;
// }

int crtx_append_to_dict(struct crtx_dict **dictionary, char *signature, ...) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	char *s, ret;
	va_list va;
	uint32_t sign_length;
	
	if (!dictionary)
		return 1;
	
	sign_length = strlen(signature);
	
	if (!*dictionary) {
		dict = crtx_init_dict(signature, sign_length, 0);
		*dictionary = dict;
		
		di = dict->items;
	} else {
		dict = *dictionary;
		
		crtx_resize_dict(dict, dict->signature_length + sign_length);
		
		sprintf(&dict->signature[dict->signature_length - sign_length], "%s", signature);
		
		di = &dict->items[dict->signature_length - sign_length];
	}
	
	va_start(va, signature);
	
	s = signature;
	while (*s) {
		ret = crtx_fill_data_item_va(di, *s, va);
		if (ret < 0)
			return 0;
		
		di++;
		s++;
		dict->signature_length++;
	}
	di--;
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	va_end(va);
	
	return 0;
}


struct crtx_dict * crtx_create_dict(char *signature, ...) {
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	char *s, ret;
	va_list va;
	uint32_t sign_length;
	
	sign_length = strlen(signature);
	ds = crtx_init_dict(signature, sign_length, 0);
	
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
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	va_end(va);
	
	return ds;
}

void crtx_free_dict_item(struct crtx_dict_item *di) {
// 	printf("key %s\n", di->key);
	if (di->key && di->flags & CRTX_DIF_ALLOCATED_KEY)
		free(di->key);
	
// 	if (di->pointer) {
	switch (di->type) {
		case 'p':
			if (di->pointer && !(di->flags & CRTX_DIF_DONT_FREE_DATA))
				free(di->pointer);
			di->pointer = 0;
			break;
		case 's':
			if (di->string && !(di->flags & CRTX_DIF_DONT_FREE_DATA))
				free(di->string);
			di->string = 0;
			break;
		case 'D':
// 			crtx_free_dict(di->dict);
			if (di->dict)
				crtx_dict_unref(di->dict);
			di->dict = 0;
			break;
		default:
			// nothing to do here
			break;
	}
// 	}
}

void crtx_free_dict(struct crtx_dict *ds) {
	struct crtx_dict_item *di;
	size_t i;
	
	if (!ds)
		return;
	
	if (ds->items) {
		di = ds->items;
		for (i=0; i < ds->signature_length; i++) {
// 			printf("di %p %p\n", di, di->key);
			crtx_free_dict_item(di);
			
			if (CRTX_DIF_IS_LAST(di))
				break;
			
			di++;
		}
		
		free(ds->items);
	}
	free(ds);
}

void crtx_print_dict_rec(struct crtx_dict *ds, unsigned char level);
void crtx_print_dict_item(struct crtx_dict_item *di, unsigned char level) {
	INFO("%s = ", di->key?di->key:"\"\"");
	
	switch (di->type) {
		case 'u': INFO("(uint32_t) %u", di->uint32); break;
		case 'i': INFO("(int32_t) %d", di->int32); break;
		case 's': INFO("(char*) \"%s\"", di->string); break;
		case 'p': INFO("(void*) %p", di->pointer); break;
		case 'U': INFO("(uint64_t) %" PRIu64, di->uint64); break;
		case 'I': INFO("(int64_t) %" PRId64, di->int32); break;
		case 'd': INFO("(double) %f", di->double_fp); break;
		case 'z': INFO("(size_t) %zu", di->sizet); break;
		case 'Z': INFO("(ssize_t) %zu", di->ssizet); break;
		case 'D':
// 			INFO("(dict) ");
			if (di->dict)
				crtx_print_dict_rec(di->dict, level+1);
			else
				INFO("(null)\n");
			break;
		default: ERROR("print_dict: unknown type '%c'\n", di->type); break;
	}
	
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
		
		INFO("%u: ", i);
		
		crtx_print_dict_item(di, level);
		if (di->type != 'D')
			INFO("\n");
		
		if (s && *s != di->type)
			ERROR("error type %c != %c\n", *s, di->type);
		
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
			break;
		case 'U':
		case 'I':
			if (buffer_size == sizeof(uint64_t)) {
				memcpy(buffer, &di->uint64, buffer_size);
				return 1;
			}
			break;
		case 'z':
		case 'Z':
			if (buffer_size == sizeof(size_t)) {
				memcpy(buffer, &di->sizet, buffer_size);
				return 1;
			}
			break;
		case 'd':
			if (buffer_size == sizeof(double)) {
				memcpy(buffer, &di->double_fp, buffer_size);
				return 1;
			}
			break;
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
	if (ds && ds->items)
		return ds->items;
	else
		return 0;
}

struct crtx_dict_item *crtx_get_next_item(struct crtx_dict *ds, struct crtx_dict_item *di) {
	if (! CRTX_DIF_IS_LAST(di) && (&ds->items[ds->signature_length] > di+1))
		return di+1;
	else
		return 0;
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

char crtx_conv_value(char dtype,  void *dbuffer, size_t dbuffer_size, char stype,  void *sbuffer, size_t sbuffer_size) {
	int ret;
	
	VDBG("convert %c to %c\n", stype, dtype);
	
	if (stype == dtype) {
		memcpy(dbuffer, sbuffer, dbuffer_size);
		return 1;
	}
	
	if (stype == 'u' && dtype == 'U') {
		uint64_t v64;
		uint32_t v32;
		
		memcpy(&v32, sbuffer, sizeof(uint32_t));
		v64 = v32;
		if (sizeof(uint64_t) <= dbuffer_size) {
			memcpy(dbuffer, &v64, sizeof(uint64_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(uint64_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'U' && dtype == 'u') {
		uint64_t u64;
		uint32_t u32;
		
		memcpy(&u64, sbuffer, sizeof(uint32_t));
		if (u64 < UINT32_MAX) {
			u32 = u64;
		} else {
			ERROR("cannot convert %" PRIu64 " to uint32_t\n", u64);
			return 0;
		}
			
		if (sizeof(uint32_t) <= dbuffer_size) {
			memcpy(dbuffer, &u32, sizeof(uint32_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(uint32_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'I' && dtype == 'i') {
		int64_t i64;
		int32_t i32;
		
		memcpy(&i64, sbuffer, sizeof(int64_t));
		if ( (i64 < 0 && i64 > INT32_MIN) ||
			(i64 > 0 && i64 < INT32_MAX))
		{
			i32 = i64;
		} else {
			ERROR("cannot convert %" PRIi64 " to int32_t\n", i64);
			return 0;
		}
		if (sizeof(int32_t) <= dbuffer_size) {
			memcpy(dbuffer, &i32, sizeof(int32_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(int32_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'i' && dtype == 'u') {
		uint32_t u32;
		int32_t i32;
		
		memcpy(&i32, sbuffer, sizeof(int32_t));
		if (i32 >= 0) {
			u32 = i32;
		} else {
			ERROR("cannot convert %" PRId32 " to uint32_t\n", i32);
			return 0;
		}
		if (sizeof(uint32_t) <= dbuffer_size) {
			memcpy(dbuffer, &u32, sizeof(uint32_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(uint32_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'I' && dtype == 'U') {
		uint64_t u64;
		int64_t i64;
		
		memcpy(&i64, sbuffer, sizeof(int64_t));
		if (i64 >= 0) {
			u64 = i64;
		} else {
			ERROR("cannot convert %" PRId64 " to uint64_t\n", i64);
			return 0;
		}
		if (sizeof(uint64_t) <= dbuffer_size) {
			memcpy(dbuffer, &u64, sizeof(uint64_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(uint64_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'd' && dtype == 'I') {
		double d;
		int64_t i64;
		
		memcpy(&d, sbuffer, sizeof(double));
		if (d < INT64_MAX && INT64_MIN < d) {
			i64 = d;
		} else {
			ERROR("cannot convert %f to int64_t\n", d);
			return 0;
		}
		if (sizeof(int64_t) <= dbuffer_size) {
			memcpy(dbuffer, &i64, sizeof(int64_t));
			return 1;
		} else {
			ERROR("crtx_get_item_value insufficient size: %zu > %zu\n", sizeof(int64_t), dbuffer_size);
			return 0;
		}
	} else
	if (stype == 'I' && dtype == 'u') {
		int32_t i32;
		
		ret = crtx_conv_value('i', &i32, sizeof(int32_t), stype, sbuffer, sbuffer_size);
		if (!ret)
			return 0;
		ret = crtx_conv_value(dtype, dbuffer, dbuffer_size, 'i', &i32, sizeof(int32_t));
		if (!ret)
			return 0;
		
		return 1;
	} else
	if (stype == 'd' && dtype == 'U') {
		int64_t i64;
		
		ret = crtx_conv_value('I', &i64, sizeof(int64_t), stype, sbuffer, sbuffer_size);
		if (!ret)
			return 0;
		ret = crtx_conv_value(dtype, dbuffer, dbuffer_size, 'I', &i64, sizeof(int64_t));
		if (!ret)
			return 0;
		
		return 1;
	} else {
		ERROR("TODO conversion %c -> %c\n", stype, dtype);
		return 0;
	}
	
	return 0;
}

char crtx_get_item_value(struct crtx_dict_item *di, char type,  void *buffer, size_t buffer_size) {
	if (di->type == type) {
		return crtx_copy_value(di, buffer, buffer_size);
	}
	
	switch (di->type) {
		case 'i':
		case 'u':
			return crtx_conv_value(type, buffer, buffer_size, di->type, &di->uint32, sizeof(uint32_t));
		case 'I':
		case 'U':
			return crtx_conv_value(type, buffer, buffer_size, di->type, &di->uint64, sizeof(uint64_t));
		case 'd':
			return crtx_conv_value(type, buffer, buffer_size, di->type, &di->double_fp, sizeof(uint64_t));
		default:
			ERROR("TODO conversion2 %c -> %c\n", di->type, type);
			return 0;
	}
}

char crtx_get_value(struct crtx_dict *ds, char *key, char type, void *buffer, size_t buffer_size) {
	struct crtx_dict_item *di;
	
	di = crtx_get_item(ds, key);
	if (!di)
		return 0;
	
	return crtx_get_item_value(di, type, buffer, buffer_size);
	
// 	if (type > 0 && di->type != type) {
// 		ERROR("type mismatch for \"%s\": %c != %c\n", key, type, di->type);
// 		return 0;
// 	}
	
// 	return crtx_copy_value(di, buffer, buffer_size);
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
			if (*key != '*') {
// 				di = crtx_get_item(ds, key);
				di = crtx_dict_locate(ds, key);
				if (!di) {
					ERROR("dict transformation failed, no entry \"%s\"\n", key);
					return 0;
				}
			} else {
				di = &ds->items[0];
			}
			
			
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
				ret = dict_printf(di->dict, fmt, result, rlen, alloc);
				
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
							
							*alloc = *alloc + len + 1;
							*result = (char*) realloc(*result, *alloc);
							
							*rlen += sprintf( &((*result)[*rlen]), "%s", s);
							
							// add separator, if present TODO
							if (*key == '*' && keylen > 1 && i < ds->signature_length-1)
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
	uint32_t sign_length;
	
	sign_length = strlen(signature);
	ds = crtx_init_dict(signature, sign_length, 0);
	
	di = ds->items;
	pit = transf;
// 	for (pit = transf; pit && pit->type; pit++) {
	for (i=0; i<sign_length; i++) {
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
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	return ds;
}

char crtx_transform_dict_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict, *orig_dict;
	struct crtx_event *new_event;
	struct crtx_transform_dict_handler *trans;
	
// 	if (!event->data.dict)
// 		event->data.to_dict(&event->data);
	
	crtx_event_get_payload(event, 0, 0, &orig_dict);
	
	if (!orig_dict) {
		ERROR("cannot convert %s to dict", event->type);
		return 1;
	}
	
	trans = (struct crtx_transform_dict_handler*) userdata;
	
	dict = crtx_dict_transform(orig_dict, trans->signature, trans->transformation);
	
	new_event = crtx_create_event(trans->type, 0, 0);
	new_event->data.dict = dict;
	
	if (trans->graph)
		crtx_add_event(trans->graph, new_event);
	else
		add_raw_event(new_event);
	
	return 1;
}

struct crtx_task *crtx_create_transform_task(struct crtx_graph *in_graph, char *name, struct crtx_transform_dict_handler *trans) {
	return crtx_create_task(in_graph, 0, name, &crtx_transform_dict_handler, trans);
}

#define MY_CMP(a, b) ( (a) < (b) ? -1 : (a) > (b) ? 1 : 0 )
char crtx_cmp_item(struct crtx_dict_item *a, struct crtx_dict_item *b) {
	int res;
	struct crtx_dict_item *ia, *ib;
	
	res = memcmp(&a->type, &b->type, 1);
	if (res)
		return res;
	
	switch (a->type) {
		case 's':
			if (!a->string)
				return -1;
			if (!b->string)
				return 1;
			return strcmp(a->string, b->string);
		case 'u':
			return MY_CMP(a->uint32, b->uint32);
		case 'i':
			return MY_CMP(a->int32, b->int32);
		case 'U':
			return MY_CMP(a->uint64, b->uint64);
		case 'I':
			return MY_CMP(a->int64, b->int64);
		case 'D':
			ia = crtx_get_first_item(a->dict);
			ib = crtx_get_first_item(b->dict);
			
			while (ia && ib) {
				res = crtx_cmp_item(ia, ib);
				if (res)
					return res;
				
				ia = crtx_get_next_item(a->dict, ia);
				ib = crtx_get_next_item(b->dict, ib);
			}
			if (!ia && !ib)
				return 0;
			if (!ia)
				return -1;
			if (!ib)
				return 1;
		default: ERROR("TODO");
			return 1;
	}
}

char crtx_dict_calc_payload_size(struct crtx_dict *orig, size_t *size) {
	struct crtx_dict_item *di;
	size_t lsize;
	char ret, result;
	
	di = crtx_get_first_item(orig);
	
	result = 1;
	lsize = 0;
	while (di) {
		if (di->type == 'D') {
			ret = crtx_dict_calc_payload_size(di->dict, &lsize);
			if (!ret)
				result = 0;
		} else
		if (di->type == 'p' || di->type == 's') {
			if (di->size == 0)
				DBG("calc_payload_size: size of item \"%s\" (t %c) = 0\n", di->key, di->type);
			lsize += di->size;
		}
		
		di = crtx_get_next_item(orig, di);
	}
	
	*size += lsize;
	
	return result;
}

// void crtx_dict_copy_item_data(struct crtx_dict_item *dst, struct crtx_dict_item *src) {
// 	struct crtx_dict_item tmp;
// 	
// 	
// 	dst->type = src->type;
// 	dst->size = src->size;
// 	
// 	if (src->type == 'D') {
// 		dst->dict = crtx_dict_copy(src->dict);
// 	} else
// 	if (src->type == 's') {
// 		if ( !(src->flags & CRTX_DIF_DONT_FREE_DATA) )
// 			dst->string = stracpy(src->string, 0);
// 	} else
// 	if (src->type == 'p') {
// 		if ( !(src->flags & CRTX_DIF_DONT_FREE_DATA) ) {
// 			dst->pointer = malloc(src->size);
// 			memcpy(dst->pointer, src->pointer, src->size);
// 			
// // 			if (src->copy_raw) {
// // 				dst->pointer = src->copy_raw(&event->response);
// // 			} else {
// // 				dst->pointer = src->pointer;
// // 				dst->flags |= CRTX_DIF_DONT_FREE_DATA;
// // 			}
// 			dst->size = src->size;
// 		}
// 	} else {
// 		memcpy(&tmp, dst, sizeof(struct crtx_dict_item));
// 		memcpy(dst, src, sizeof(struct crtx_dict_item));
// 		dst->key = tmp.key;
// 		dst->flags = tmp.flags;
// 	}
// }

void crtx_dict_copy_item(struct crtx_dict_item *dst, struct crtx_dict_item *src, char data_only) {
// 	dst->flags = 0;
	
	switch (src->type) {
		case 'D':
// 			dst->dict = crtx_dict_copy(src->dict);
			crtx_dict_ref(src->dict);
			dst->dict = src->dict;
			break;
		case 's':
			dst->string = crtx_stracpy(src->string, 0);
			break;
		case 'p':
			if (src->size) {
				dst->pointer = malloc(src->size);
				memcpy(dst->pointer, src->pointer, src->size);
			} else {
				ERROR("copying reference only\n");
				dst->pointer = src->pointer;
			}
			
// 			dst->size = src->size;
			break;
		default:
			{
				// TODO assumption that uint64 is one of the largest types
				dst->uint64 = src->uint64;
				
// 				char *key;
// 				unsigned char flags;
// 				
// 				key = dst->key;
// 				flags = dst->flags;
// 				
// 				// copy complete struct as we can't copy the anonymous union
// 				memcpy(dst, src, sizeof(struct crtx_dict_item));
// 				
// 				dst->key = key;
// 				dst->flags = flags;
			}
			return;
	}
	
	if (!data_only) {
// 		dst->flags = src->flags;
		
		if (src->key) {
			if (src->flags & CRTX_DIF_ALLOCATED_KEY) {
				dst->flags |= CRTX_DIF_ALLOCATED_KEY;
				dst->key = crtx_stracpy(src->key, 0);
			} else {
				dst->key = src->key;
			}
		}
	}
	
	dst->type = src->type;
	dst->size = src->size;
}

// struct crtx_dict *crtx_dict_copy(struct crtx_dict *orig) {
// 	size_t psize;
// 	char ret;
// 	struct crtx_dict *dict;
// 	struct crtx_dict_item *oitem, *nitem;
// 	
// 	printf("coipy\n");
// 	
// 	psize = 0;
// 	ret = crtx_dict_calc_payload_size(orig, &psize);
// 	if (!ret)
// 		ERROR("error while calculating dict payload size (sign \"%s\")\n", orig->signature);
// 	
// 	dict = crtx_init_dict(orig->signature, orig->signature_length, psize);
// 	
// 	oitem = crtx_get_first_item(orig);
// 	nitem = crtx_get_first_item(dict);
// 	while (oitem) {
// 		crtx_dict_copy_item(nitem, oitem, 0);
// 		
// 		oitem = crtx_get_next_item(orig, oitem);
// 		nitem = crtx_get_next_item(dict, nitem);
// 	}
// 	
// 	return dict;
// }

// void crtx_copy_item(struct crtx_dict_item *dst, struct crtx_dict_item *src) {
// 	if (src->key && src->flags & CRTX_DIF_ALLOCATED_KEY) {
// 		size_t slen;
// 		
// 		slen = strlen(src->key);
// 		dst->key = (char*) malloc(slen+1);
// 		sprint(dst->key, "%s", src->key);
// 		
// 		dst->flags = CRTX_DIF_ALLOCATED_KEY;
// 	} else {
// 		dst->key = 0;
// 		dst->flags = 0;
// 	}
// 	
// 	dst->size = src->size;
// 	dst->type = src->type;
// 	
// 	switch (src->type) {
// 		case 'p':
// 			if (src->flags & CRTX_DIF_DONT_FREE_DATA) {
// 				dst->pointer = src->pointer;
// 			} else {
// 				memcpy(dst->pointer, src->pointer, sizeof(struct crtx_dict_item));
// 			}
// 			break;
// 		case 's':
// 			
// 	}
// }

void crtx_dict_ref(struct crtx_dict *dict) {
	LOCK(dict->mutex);
	dict->ref_count++;
	UNLOCK(dict->mutex);
}

void crtx_dict_unref(struct crtx_dict *dict) {
	LOCK(dict->mutex);
	dict->ref_count--;
	
	if (dict->ref_count == 0) {
// 		printf("unref free dict %p\n", dict);
		crtx_free_dict(dict);
	} else {
		UNLOCK(dict->mutex);
	}
}

struct crtx_dict_item * crtx_dict_locate(struct crtx_dict *dict, char *path) {
	char *p, *sep, *buf;
	struct crtx_dict_item *di;
	size_t buflen;
	
	buflen = strlen(path)+1;
	buf = (char*) malloc(buflen);
	
	p = path;
	while (1) {
		sep = strchr(p, '/');
		
		if (sep) {
			memcpy(buf, p, sep-p);
			buf[sep-p] = 0;
		} else {
			strcpy(buf, p);
		}
		
		di = crtx_get_item(dict, buf);
		if (!di)
			break;
		if (sep == 0)
			break;
		
		if (di->type != 'D') {
			di = 0;
			break;
		}
		
		dict = di->dict;
		p = sep+1;
	}
	
	free(buf);
	return di;
}

char crtx_dict_locate_value(struct crtx_dict *dict, char *path, char type, void *buffer, size_t buffer_size) {
	struct crtx_dict_item * di;
	char r;
	
	di = crtx_dict_locate(dict, path);
	if (!di) {
		DBG("could not find \"%s\" in dict\n", path);
		return 1;
	}
	
	r = crtx_get_item_value(di, type,  buffer, buffer_size);
	return !r;
}

void crtx_dict_remove_item(struct crtx_dict *dict, char *key) {
	struct crtx_dict_item *di;
	
	di = crtx_get_item(dict, key);
	
	crtx_free_dict_item(di);
	memset(di, 0, sizeof(struct crtx_dict_item));
}
