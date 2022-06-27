#ifndef _CRTX_CACHE_H
#define _CRTX_CACHE_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "dict.h"

struct crtx_cache;
struct crtx_cache_entry;
struct crtx_cache_task;

typedef int (*create_key_cb_t)(struct crtx_event *event, struct crtx_dict_item *key);
typedef struct crtx_dict_item * (*match_cb_t)(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
typedef int (*hit_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);
typedef int (*miss_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);
typedef int (*add_cb_t)(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event);
typedef void (*get_time_cb_t)(struct crtx_dict_item *item);


// An entry in the simple layout cache contains only the value.
// In a non-simple layout the entry contains a dictionary which may contain
// additional fields like number of hits or time since last hist
#define CRTX_CACHE_SIMPLE_LAYOUT 1<<0
#define CRTX_CACHE_NO_EXT_FIELDS 1<<1
// #define CRTX_CACHE_REGEXP 1<<2

struct crtx_cache {
	char flags;
	
	struct crtx_dict *dict;
	
	// pointers into $dict
	struct crtx_dict *config;
	struct crtx_dict *entries;
	
	get_time_cb_t get_time;
	
	pthread_mutex_t mutex;
	
	struct crtx_cache_task *ct;
	
	#ifndef CRTX_REDUCED_SIZE
	void *reserved1;
	#endif
};

struct crtx_cache_task {
	struct crtx_cache *cache;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
	hit_cb_t on_hit;
	miss_cb_t on_miss;
	add_cb_t on_add; // can prevent adding entries to the cache
	
	void *userdata;
	
	#ifndef CRTX_REDUCED_SIZE
	void *reserved1;
	#endif
};

typedef char (*create_key_action_cb_t)(struct crtx_event *event, struct crtx_dict_item *key, char *add);
struct crtx_presence_cache_task {
	struct crtx_cache *cache;
	
	create_key_action_cb_t create_key_action;
	match_cb_t match_event;
	
	get_time_cb_t get_time;
};

char response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata);
struct crtx_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void free_response_cache(struct crtx_cache *dc);
void free_response_cache_task(struct crtx_cache_task *ct);
struct crtx_dict_item * rcache_match_cb_t_regex(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
struct crtx_dict_item * rcache_match_cb_t_strcmp(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event);
struct crtx_dict_item * crtx_cache_add_entry(struct crtx_cache *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *entry_data);
char crtx_load_cache(struct crtx_cache *cache, char *path);
int crtx_cache_no_add(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event);
int crtx_cache_on_hit(struct crtx_cache_task *rc, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);
// void crtx_flush_entries(struct crtx_cache *dc);

int crtx_cache_add_on_miss(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);
int crtx_cache_update_on_hit(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry);

struct crtx_task *crtx_create_presence_cache_task(char *id, create_key_action_cb_t create_key);
void crtx_free_presence_cache_task(struct crtx_task *task);

int crtx_create_response_cache_task(struct crtx_task **task, char *id, create_key_cb_t create_key);
void crtx_free_response_cache_task(struct crtx_task *task);

void crtx_cache_remove_entry(struct crtx_cache *cache, struct crtx_dict_item *key);

#endif
