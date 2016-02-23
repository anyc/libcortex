
#ifndef CRTX_DICT_H
#define CRTX_DICT_H

#define DIF_KEY_ALLOCATED 1<<0
#define DIF_DATA_UNALLOCATED 1<<1
#define DIF_LAST 1<<7

#define DIF_IS_LAST(di) ( ((di)->flags & DIF_LAST) != 0 )

struct crtx_dict_item {
	char *key;
	
	unsigned long size;
	unsigned char flags;
	char type;
	
	union {
		uint32_t uint32; // u
		int32_t int32; // i
		uint64_t uint64; // U
		int64_t int64; // I
		double double_fp; // d
		
		char *string; // s
		void *pointer; // p		
		struct crtx_dict *ds; // D
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
char  crtx_fill_data_item(struct crtx_dict_item *di, char type, ...);
void free_dict(struct crtx_dict *ds);
char crtx_get_value(struct crtx_dict *ds, char *key, char type, void *buffer, size_t buffer_size);
char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size);
void crtx_print_dict(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_next_item(struct crtx_dict *ds, struct crtx_dict_item *di);
char crtx_add_item(struct crtx_dict **dict, char type, ...);

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
#endif
