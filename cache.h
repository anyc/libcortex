
#include "dict.h"

struct crtx_cache;
struct crtx_cache_entry;
struct crtx_cache_task;

typedef char (*create_key_cb_t)(struct crtx_event *event, struct crtx_dict_item *key);
typedef struct crtx_dict_item * (*match_cb_t)(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
typedef void (*hit_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);
typedef void (*miss_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event);
typedef char (*add_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event);
typedef void (*get_time_cb_t)(struct crtx_dict_item *item);

struct crtx_cache_regex {
	regex_t regex;
	char key_is_regex;
	char initialized;
	
	struct crtx_dict_item *dict_item;
};

#define CRTX_CACHE_SIMPLE_LAYOUT 1<<0
#define CRTX_CACHE_NO_TIMES 1<<1
#define CRTX_CACHE_REGEXP 1<<2

struct crtx_cache {
	char flags;
	
	struct crtx_dict *entries;
	
	struct crtx_dict *config;
	
	struct crtx_cache_regex *regexps;
	size_t n_regexps;
	
	pthread_mutex_t mutex;
};

struct crtx_cache_task {
	struct crtx_cache *cache;
	
	// temporary data
	struct crtx_dict_item key;
	struct crtx_dict_item *cache_entry;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
	hit_cb_t on_hit;
	miss_cb_t on_miss;
	add_cb_t on_add; // can prevent adding entries to the cache
	
	get_time_cb_t get_time;
};

void response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata);
struct crtx_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void free_response_cache(struct crtx_cache *dc);
// void prefill_rcache(struct crtx_cache *rcache, char *file);
struct crtx_dict_item * rcache_match_cb_t_regex(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
struct crtx_dict_item * rcache_match_cb_t_strcmp(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
void crtx_cache_add_entry(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event);