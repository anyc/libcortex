
struct response_cache_entry {
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
};

struct response_cache;
struct response_cache_task;
typedef char *(*create_key_cb_t)(struct event *event);
typedef char (*match_cb_t)(struct response_cache *rc, void *key, struct event *event, struct response_cache_entry *c_entry);
typedef void (*hit_cb_t)(struct response_cache_task *ct, void **key, struct event *event, struct response_cache_entry *c_entry);
typedef void (*miss_cb_t)(struct response_cache_task *ct, void **key, struct event *event);

struct response_cache {
	char *signature;
	
	struct response_cache_entry *entries;
	size_t n_entries;
	
	pthread_mutex_t mutex;
};

struct response_cache_task {
	struct response_cache *cache;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
	hit_cb_t on_hit;
	miss_cb_t on_miss;
};

void response_cache_task(struct event *event, void *userdata, void **sessiondata);
struct event_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void free_response_cache(struct response_cache *dc);
void prefill_rcache(struct response_cache *rcache, char *file);
char rcache_match_cb_t_regex(struct response_cache *rc, void *key, struct event *event, struct response_cache_entry *c_entry);
char rcache_match_cb_t_strcmp(struct response_cache *rc, void *key, struct event *event, struct response_cache_entry *c_entry);