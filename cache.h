
#include "dict.h"

struct crtx_cache;
struct crtx_cache_entry;
struct crtx_cache_task;

typedef char *(*create_key_cb_t)(struct crtx_event *event);
typedef struct crtx_dict_item * (*match_cb_t)(struct crtx_cache *rc, void *key, struct crtx_event *event);
typedef void (*hit_cb_t)(struct crtx_cache_task *ct, void **key, struct crtx_event *event, struct crtx_dict_item *c_entry);
typedef void (*miss_cb_t)(struct crtx_cache_task *ct, void **key, struct crtx_event *event);
typedef char (*add_cb_t)(struct crtx_cache_task *ct, void *key, struct crtx_event *event);


struct crtx_cache_regex {
	regex_t regex;
	char key_is_regex;
	char initialized;
	
	struct crtx_dict_item *dict_item;
};

struct crtx_cache {
	char complex;
	
	struct crtx_dict *entries;
	
	struct crtx_cache_regex *regexps;
	size_t n_regexps;
	
	pthread_mutex_t mutex;
};

struct crtx_cache_task {
	struct crtx_cache *cache;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
	hit_cb_t on_hit;
	miss_cb_t on_miss;
	add_cb_t on_add; // can prevent adding entries to the cache
};

void response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata);
struct crtx_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void free_response_cache(struct crtx_cache *dc);
// void prefill_rcache(struct crtx_cache *rcache, char *file);
struct crtx_dict_item * rcache_match_cb_t_regex(struct crtx_cache *rc, void *key, struct crtx_event *event);
struct crtx_dict_item * rcache_match_cb_t_strcmp(struct crtx_cache *rc, void *key, struct crtx_event *event);