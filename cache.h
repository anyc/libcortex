

struct crtx_cache;
struct crtx_cache_entry;
struct crtx_cache_task;

typedef char *(*create_key_cb_t)(struct crtx_event *event);
typedef char (*match_cb_t)(struct crtx_cache *rc, void *key, struct crtx_event *event, struct crtx_cache_entry *c_entry);
typedef void (*hit_cb_t)(struct crtx_cache_task *ct, void **key, struct crtx_event *event, struct crtx_cache_entry *c_entry);
typedef void (*miss_cb_t)(struct crtx_cache_task *ct, void **key, struct crtx_event *event);


struct crtx_cache_entry {
	void *key;
	union {
		regex_t key_regex;
	};
	union {
		char regex_initialized;
	};
	char type;
	
	void *value;
	size_t value_size;
	char copied;
};

struct crtx_cache {
	char *signature;
	
	struct crtx_cache_entry *entries;
	size_t n_entries;
	
	pthread_mutex_t mutex;
};

struct crtx_cache_task {
	struct crtx_cache *cache;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
	hit_cb_t on_hit;
	miss_cb_t on_miss;
};

void response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata);
struct crtx_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void free_response_cache(struct crtx_cache *dc);
void prefill_rcache(struct crtx_cache *rcache, char *file);
char rcache_match_cb_t_regex(struct crtx_cache *rc, void *key, struct crtx_event *event, struct crtx_cache_entry *c_entry);
char rcache_match_cb_t_strcmp(struct crtx_cache *rc, void *key, struct crtx_event *event, struct crtx_cache_entry *c_entry);