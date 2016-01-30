
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "core.h"
#include "dict.h"

static char parse_fmt(struct crtx_dict *ds, char *format, char **inkeys, char **result, size_t *rlen) {
	char *f, *e;
	char **ink, *s;
	size_t len, alloc;
	char *fmt;
	char buf[32]; // assume primitive data types fit in 32 chars
	unsigned char min;
	struct crtx_dict_item *di;
	
	*rlen = 0;
	alloc = 24;
	*result = (char*) malloc(alloc);
	
	printf("fmt %s %s\n", ds->signature, format);
	
	f = format;
	ink = inkeys;
	di = crtx_get_first_item(ds);
	while (*f) {
		if (*f == '%' && *(f+1) != '%') {
			e = f;
			if (*(f+1) == '(') {
				while (*e != ')')
				{
					e++;
				}
				min = e - (f+2);
				
				fmt = (char*) malloc(min+1);
				strncpy(fmt, f+2, min);
				fmt[min] = 0;
				printf("sub %s\n", fmt);
				
				if (ink && *ink)
					di = crtx_get_item(ds, *ink);
				else
					di = crtx_get_next_item(di);
				
				char ret;
				ret = parse_fmt(di->ds, fmt, (char**) *ink, result, rlen);
				if (!ret)
					return 0;
			} else {
				while (*e != 's' &&
					*e != 'd' &&
					*e != 'D' &&
					*e != 'u')
				{
					e++;
				}
				min = e - f + 1;
				
				fmt = (char*) malloc(min+1);
				strncpy(fmt, f, min);
				fmt[min] = 0;
				printf("fmt %c %c\n", fmt[1], *e);
				f = e;
				switch (*e) {
					case 's':
						do {
							if (ink)
								di = crtx_get_item(ds, *ink);
							else
								di = crtx_get_next_item(di);
							printf("%c\n", di->type);
							if (!di || di->type != 's') {
								if (ink)
									ERROR("get_value failed, requested key %s\n", *ink);
								return 0;
							}
							printf("%c\n", di->type);
							
							s = di->string;
							len = strlen(s);
							
							alloc += len;
							*result = (char*) realloc(*result, alloc);
							
							*rlen += sprintf( &((*result)[*rlen]), fmt, s);
						} while (fmt[1] == '*');
						
						break;
					case 'd':
					case 'u':
						if (ink)
							di = crtx_get_item(ds, *ink);
						else
							di = crtx_get_next_item(di);
						
						if (!di || (di->type != 's' && di->type != 'u')) {
							if (ink)
								ERROR("get_value failed, requested key %s\n", *ink);
							return 0;
						}
						
						snprintf(buf, 32, fmt, di->uint32);
						len = strlen(buf);
						
						alloc += len;
						*result = (char*) realloc(*result, alloc);
						
						*rlen += sprintf( &((*result)[*rlen]), "%s", buf);
						
						break;
					default:
						ERROR("%c unknown", *e);
						return 0;
				}
			}
			
			if (ink)
				ink++;
		} else {
			if (*rlen >= alloc-1) {
				alloc += 24;
				*result = (char*) realloc(*result, alloc);
			}
			
			(*result)[*rlen] = *f;
			
			(*rlen)++;
			
			if (*f == '%' && *(f+1) == '%')
				f++;
		}
		f++;
	}
	
	if (*rlen >= alloc-1) {
		alloc += 1;
		*result = (char*) realloc(*result, alloc);
	}
	(*result)[*rlen] = 0;
	
	return 1;
}

struct crtx_dict * crtx_dict_transform(struct crtx_dict *dict, char *signature, struct crtx_dict_transformation *transf) {
	struct crtx_dict_transformation *pit;
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	
	ds = crtx_init_dict(signature, 0);
	
	di = ds->items;
	for (pit = transf; pit && pit->outtype; pit++) {
		di->type = pit->outtype;
		di->key = pit->outkey;
		
		if (pit->outtype == 's') {
			parse_fmt(dict, pit->format, pit->inkeys, &di->string, &di->size);
			di->size++; // add space for \0 delimiter
		} else
			if (pit->outtype == 'D') {
				
			} else {
				
			}
			
			di->flags = pit->outflag;
		
		di++;
		ds->signature_length++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	return ds;
}


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

struct crtx_dict * crtx_init_dict(char *signature, size_t payload_size) {
	struct crtx_dict *ds;
	size_t size;
	
	size = strlen(signature) * sizeof(struct crtx_dict_item) + sizeof(struct crtx_dict) + payload_size;
	ds = (struct crtx_dict*) malloc(size);
	
	ds->signature = signature;
	ds->size = size;
	ds->signature_length = 0;
	
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
	char *s;
	
	if (!ds)
		return;
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
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
		s++;
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
	
	for (j=0;j<level;j++) printf("  ");
	INFO("sign: %s (%zu==%u)\n", ds->signature, strlen(ds->signature), ds->signature_length);
	
	i=0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		for (j=0;j<level;j++) INFO("  "); // indent
		INFO("%u: %s = ", i, di->key);
		
		if (*s != di->type)
			INFO("error type %c != %c\n", *s, di->type);
		
		switch (di->type) {
			case 'u': INFO("(uint32_t) %d\n", di->uint32); break;
			case 'i': INFO("(int32_t) %d\n", di->int32); break;
			case 's': INFO("(char*) %s\n", di->string); break;
			case 'D':
				INFO("(dict) \n");
				if (di->ds)
					crtx_print_dict_rec(di->ds, level+1);
				break;
		}
		
		s++;
		di++;
		i++;
	}
}

void crtx_print_dict(struct crtx_dict *ds) {
	crtx_print_dict_rec(ds, 0);
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
	return ds->items;
}

struct crtx_dict_item *crtx_get_next_item(struct crtx_dict_item *di) {
	return di+1;
}

struct crtx_dict_item * crtx_get_item(struct crtx_dict *ds, char *key) {
	char *s;
	struct crtx_dict_item *di;
	
	if (!ds)
		return 0;
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
		if (!strcmp(di->key, key)) {
			return di;
		}
		
		s++;
		di++;
	}
	
	return 0;
}

char crtx_get_value(struct crtx_dict *ds, char *key, void *buffer, size_t buffer_size) {
	struct crtx_dict_item *di;
	
	di = crtx_get_item(ds, key);
	if (!di)
		return 0;
	
	return crtx_copy_value(di, buffer, buffer_size);
}
