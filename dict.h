
#define DIF_KEY_ALLOCATED 1<<0
#define DIF_DATA_UNALLOCATED 1<<1
#define DIF_LAST 1<<7

#define DIF_IS_LAST(di) ( ((di)->flags & DIF_LAST) != 0 )

struct crtx_dict_item {
	char *key;
	unsigned long size;
	
	char type;
	char flags;
	
	union {
		char *string; // s
		uint32_t uint32; // u
		int32_t int32; // i
		uint64_t uint64; // z
		void *pointer; // p
		struct crtx_dict *ds; // D
// 		char payload[0]; // P
	};
};

struct crtx_dict {
	char *signature;
	unsigned char signature_length;
	unsigned long size;
	
// 	struct crtx_dict_item items[0];
	struct crtx_dict_item *items;
};

struct crtx_dict_transformation {
	char *outkey;
	char outtype;
	char outflag;
	
	char *format;
// 	char **inkeys;
};

struct crtx_dict_item * crtx_alloc_item(struct crtx_dict *dict);
struct crtx_dict * crtx_dict_transform(struct crtx_dict *dict, char *signature, struct crtx_dict_transformation *transf);
struct crtx_dict_item * crtx_get_item(struct crtx_dict *ds, char *key);
struct crtx_dict * crtx_init_dict(char *signature, size_t payload_size);
struct crtx_dict * crtx_create_dict(char *signature, ...);
char  crtx_fill_data_item(struct crtx_dict_item *di, char type, ...);
void free_dict(struct crtx_dict *ds);
char crtx_get_value(struct crtx_dict *ds, char *key, void *buffer, size_t buffer_size);
char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size);
void crtx_print_dict(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_next_item(struct crtx_dict_item *di);