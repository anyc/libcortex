
#ifndef CRTX_DICT_H
#define CRTX_DICT_H

#define CRTX_DICT_GET_NUMBER(dict_item) ( \
	(dict_item)->type == 'u' ? (dict_item)->uint32 : \
	(dict_item)->type == 'i' ? (dict_item)->int32 : \
	(dict_item)->type == 'U' ? (dict_item)->uint64 : \
	(dict_item)->type == 'I' ? (dict_item)->int64 : \
	(dict_item)->type == 'd' ? (dict_item)->double_fp : \
	( crtx_printf(CRTX_ERR, "invalid type %c for get_int32\n", (dict_item)->type), 0) \
	)

#define CRTX_DIF_ALLOCATED_KEY 1<<0 // key was allocated for this dict_item
#define CRTX_DIF_DONT_FREE_DATA 1<<1 // data was not allocated for the dict_item
#define CRTX_DIF_CREATE_DATA_COPY 1<<2 // only used while creating a dict_item
#define CRTX_DIF_LAST_ITEM 1<<7

#define CRTX_DIF_IS_LAST(di) ( ((di)->flags & CRTX_DIF_LAST_ITEM) != 0 )

#include <stdint.h>

#include "threads.h"

struct crtx_dict_item {
	char type;
// 	char key_type;
	unsigned char flags;
	
	char *key;
	size_t size;
	
	union {
// 		uint8_t uint8; // c
// 		int8_t int8; // C
		uint32_t uint32; // u
		int32_t int32; // i
		uint64_t uint64; // U
		int64_t int64; // I
		double double_fp; // d
		size_t sizet; // z
		ssize_t ssizet; // Z
		
		char *string; // s
		void *pointer; // p		
		struct crtx_dict *dict; // D
// 		char payload[0]; // P
	};
};

struct crtx_dict {
	char *id;
	
	char *signature;
	uint32_t signature_length;
	unsigned long size;
	
// 	struct crtx_dict_item items[0];
	struct crtx_dict_item *items;
	
	MUTEX_TYPE mutex;
	unsigned int ref_count;
};

struct crtx_dict_transformation {
	char *key;
	char type;
	char flag;
	
	char *format;
};

struct crtx_transform_dict_handler {
	char *signature;
	struct crtx_dict_transformation *transformation;
	
	struct crtx_graph *graph;
	char *type;
};


struct crtx_dict_item * crtx_alloc_item(struct crtx_dict *dict);
struct crtx_dict * crtx_dict_transform(struct crtx_dict *dict, char *signature, struct crtx_dict_transformation *transf);
struct crtx_dict_item * crtx_get_item(struct crtx_dict *ds, char *key);
struct crtx_dict * crtx_init_dict(char *signature, uint32_t sign_length, size_t payload_size);
struct crtx_dict * crtx_create_dict(char *signature, ...);
char  crtx_fill_data_item(struct crtx_dict_item *di, unsigned char type, char *key, ...);
char crtx_fill_data_item_va2(struct crtx_dict_item *di, unsigned char type, char *key, va_list va);
char crtx_fill_data_item_va(struct crtx_dict_item *di, unsigned char type, va_list va);
// void crtx_free_dict(struct crtx_dict *ds);
char crtx_get_value(struct crtx_dict *ds, char *key, char type, void *buffer, size_t buffer_size);
char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size);
void crtx_print_dict(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_next_item(struct crtx_dict *ds, struct crtx_dict_item *di);
// char crtx_add_item(struct crtx_dict **dict, char type, ...);

char *crtx_get_string(struct crtx_dict *ds, char *key);
struct crtx_dict *crtx_get_dict(struct crtx_dict *ds, char *key);

struct crtx_task *crtx_create_transform_task(struct crtx_graph *in_graph, char *name, struct crtx_transform_dict_handler *trans);

struct crtx_dict_item * crtx_get_item_by_idx(struct crtx_dict *ds, size_t idx);

char crtx_cmp_item(struct crtx_dict_item *a, struct crtx_dict_item *b);
void crtx_print_dict_item(struct crtx_dict_item *di, unsigned char level);
void crtx_free_dict_item(struct crtx_dict_item *di);
struct crtx_dict *crtx_dict_copy(struct crtx_dict *orig);
void crtx_dict_copy_item(struct crtx_dict_item *dst, struct crtx_dict_item *src, char data_only);
char crtx_get_item_value(struct crtx_dict_item *di, char type,  void *buffer, size_t buffer_size);

char crtx_resize_dict(struct crtx_dict *dict, size_t n_items);
int crx_append_to_dict(struct crtx_dict **dictionary, char *signature, ...);

void crtx_dict_ref(struct crtx_dict *dict);
void crtx_dict_unref(struct crtx_dict *dict);
struct crtx_dict_item * crtx_dict_locate(struct crtx_dict *dict, char *path);
char crtx_dict_locate_value(struct crtx_dict *dict, char *path, char type, void *buffer, size_t buffer_size);
char *crtx_dict_locate_string(struct crtx_dict *dict, char *path);
void crtx_dict_remove_item(struct crtx_dict *dict, char *key);

#endif
