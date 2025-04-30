/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <time.h>
#include <linux/limits.h>

#include "intern.h"
#include "core.h"
#include "cache.h"
#include "threads.h"
#ifdef CRTX_WITH_JSON
#include "dict_inout_json.h"
#endif


struct crtx_dict_item * crtx_cache_match_cb_t_strcmp(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_dict_item *ditem;
	
	
	ditem = crtx_get_first_item(cache->entries);
	
	while (ditem) {
		if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
			return ditem;
		
// 		rkey = rcache_get_key(ditem);
// 		
// 		if (rkey && !crtx_cmp_item(key, ditem->key)) {
// 			return ditem;
// 		}
		
		ditem = crtx_get_next_item(cache->entries, ditem);
	}
	
	return 0;
}

struct crtx_dict_item * crtx_cache_match_cb_t_regex(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event) {
	int ret;
	struct crtx_dict_item *ditem;
	struct crtx_dict_item *rkey, *regex;
	
	
	ditem = crtx_get_first_item(cache->entries);
	
	while (ditem) {
		regex = crtx_get_item(ditem->dict, "regexp");
		
// 		rkey = rcache_get_key(ditem);
		
		if (ditem->type != 'D') {
			CRTX_ERROR("wrong dictionary structure for complex cache layout: %c != %c\n", ditem->type, 'D');
			ditem = crtx_get_next_item(cache->entries, ditem);
			continue;
		}
		rkey = crtx_get_item(ditem->dict, "key");
		
		if (!regex) {
			if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
				return ditem;
			
			if (rkey && !crtx_cmp_item(key, rkey))
				return ditem;
			
			ditem = crtx_get_next_item(cache->entries, ditem);
			continue;
		}
		
		if (regex->type == 'I' && regex->uint32 == 1) {
			regex->type = 'p';
			regex->pointer = (regex_t*) malloc(sizeof(regex_t));
			
			ret = regcomp(regex->pointer, rkey->string, 0);
			if (ret) {
				CRTX_ERROR("Could not compile regex %s\n", rkey->string);
				exit(1);
			}
		}
		
		ret = regexec( (regex_t*) regex->pointer, key->string, 0, NULL, 0);
		if (!ret) {
			return ditem;
		} else
		if (ret == REG_NOMATCH) {
// 			return 0;
		} else {
			char msgbuf[128];
			
			regerror(ret, (regex_t*) regex->pointer, msgbuf, sizeof(msgbuf));
			CRTX_ERROR("Regex match failed: %s\n", msgbuf);
			
// 			return 0;
		}
		
		ditem = crtx_get_next_item(cache->entries, ditem);
	}
	
	return 0;
}

int crtx_cache_on_hit(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if ( !(cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) )
		c_entry = crtx_get_item(c_entry->dict, "value");
	
	// if there is no value or type == 0, this cache entry is ignored
	if (!c_entry || c_entry->type == 0) {
		if (crtx_verbosity >= CRTX_VLEVEL_DBG) {
			CRTX_DBG("ignoring key ");
			crtx_print_dict_item(key, 0);
		}
		
		return 0;
	}
	
	if (crtx_verbosity >= CRTX_VLEVEL_DBG) {
		CRTX_INFO("cache hit for key ");
		crtx_print_dict_item(key, 0);
		CRTX_INFO(" ");
		crtx_print_dict_item(c_entry, 0);
		CRTX_INFO("\n");
	}
	
	crtx_dict_copy_item(&event->response, c_entry, 0);
	
	return 1;
}

struct cache_sessiondata {
	struct crtx_dict_item key;
	struct crtx_dict_item *cache_entry;
};

int crtx_cache_task_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache *cache = (struct crtx_cache*) userdata;
	struct crtx_dict_item *ditem, *passthrough;
	char ret;
	struct cache_sessiondata *sd;
	
	
	sd = (struct cache_sessiondata*) calloc(1, sizeof(struct cache_sessiondata));
	*sessiondata = sd;
	
	ret = cache->create_key(cache, &sd->key, event);
	if (ret) {
		if (event->description)
			CRTX_DBG("no key created for \"%s\", ignoring\n", event->description);
		else
			CRTX_DBG("no key created for \"%d\", ignoring\n", event->type);
		sd->key.string = 0;
		return 1;
	}
	
	if (cache->flags & CRTX_CACHE_SIMPLE_LAYOUT)
		cache->match_event = &crtx_cache_match_cb_t_strcmp;
	else
		cache->match_event = &crtx_cache_match_cb_t_regex;
	
	pthread_mutex_lock(&cache->mutex);
	
	ditem = cache->match_event(cache, &sd->key, event);
	
	// update stats
	if (ditem && !(cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) && !(cache->flags & CRTX_CACHE_NO_EXT_FIELDS)) {
		struct crtx_dict_item *di, now;
		uint64_t timeout;
		
		timeout = 0;
		ret = crtx_get_value(cache->config, "timeout", 'U', &timeout, sizeof(timeout));
		if (ret && timeout > 0) {
			cache->get_time(&now);
			
			di = crtx_get_item(ditem->dict, "creation");
			
			if (di && now.uint64 > di->uint64 + timeout) {
				struct crtx_dict_item *key_item;
				
				CRTX_DBG("dropping cache entry\n");
				
				key_item = crtx_get_item(ditem->dict, "key");
				
				crtx_free_dict_item_data(key_item);
				key_item->type = 0;
				
				ditem = 0;
			}
		}
		
		if (ditem) {
			// update
			di = crtx_get_item(ditem->dict, "last_match");
			if (!di) {
				di = crtx_dict_alloc_next_item(ditem->dict);
				
				di->type = 'U';
				di->key = "last_match";
				cache->get_time(di);
			} else {
				cache->get_time(di);
			}
			
			di = crtx_get_item(ditem->dict, "n_hits");
			if (!di) {
				di = crtx_dict_alloc_next_item(ditem->dict);
				
				di->type = 'U';
				di->key = "n_hits";
				di->uint64 = 0;
			} else {
				if (di->uint64 == UINT64_MAX)
					CRTX_ERROR("cache uint64 value overflow\n");
				di->uint64++;
			}
			
			passthrough = crtx_get_item(ditem->dict, "passthrough");
		}
	} else
		passthrough = 0;
	
	if (ditem) {
		char response_available;
		
		response_available = cache->on_hit(cache, &sd->key, event, ditem);
		
		sd->cache_entry = ditem;
		
		pthread_mutex_unlock(&cache->mutex);
		
// 		crtx_print_dict(ct->cache->entries);
		
		if (response_available && (!passthrough || CRTX_DICT_GET_NUMBER(passthrough) == 0))
			return 0;
		else
			return 1;
	} else
		sd->cache_entry = 0;
	
	if (cache->on_miss)
		cache->on_miss(cache, &sd->key, event, 0);
	
	pthread_mutex_unlock(&cache->mutex);
	
	return 1;
}

struct crtx_dict_item * crtx_cache_add_entry(struct crtx_cache *cache, struct crtx_dict_item *key,
											 struct crtx_event *event, struct crtx_dict_item *entry_data)
{
	struct crtx_cache *dc = cache;
	struct crtx_dict_item *c_entry;
	struct crtx_dict_item *value_item;
	
// 	CRTX_DBG("new cache entry "); crtx_print_dict_item(key, 0); CRTX_DBG("\n");
	
// 	crtx_print_dict_item(key, 0);
	
	if (cache->on_add)
		if (!cache->on_add(cache, key, event))
			return 0;
	
	if (!dc->entries) {
		struct crtx_dict_item *e;
		
		// for dc->config
		e = crtx_dict_alloc_next_item(dc->dict);
		e->type = 'D';
		e->dict = 0;
		e->key = 0;
		
		e = crtx_dict_alloc_next_item(dc->dict);
		e->type = 'D';
		e->key = 0;
		e->dict = crtx_init_dict(0, 0, 0);
		e->dict->id = "entries";
		
		dc->entries = e->dict;
	}
	
	if (dc->flags & CRTX_CACHE_SIMPLE_LAYOUT) {
		if (key->type != 's')
			CRTX_ERROR("wrong dictionary structure for simple cache layout\n");
		
		c_entry = crtx_dict_alloc_next_item(dc->entries);
		
		c_entry->key = key->string;
		c_entry->flags = CRTX_DIF_ALLOCATED_KEY;
		c_entry->type = 'p';
		key->string = 0;
		
		value_item = c_entry;
// 			if (event->response.copy_raw) {
// 				c_entry->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				c_entry->pointer = event->response.raw.pointer;
// 				c_entry->flags |= CRTX_DIF_DONT_FREE_DATA;
// 			}
// 			c_entry->size = event->response.raw.size;
	} else {
		unsigned char n_items;
		
		// check if there is an empty cache entry
		c_entry = crtx_get_first_item(dc->entries);
		while (c_entry) {
			struct crtx_dict_item *it;
			
			if (c_entry->type != 'D')
				CRTX_ERROR("cache item type %c != D\n", c_entry->type);
			
			it = crtx_get_item(c_entry->dict, "key");
			if (!it || it->type == 0)
				break;
			
			c_entry = crtx_get_next_item(dc->entries, c_entry);
		}
		if (!c_entry)
			c_entry = crtx_dict_alloc_next_item(dc->entries);
		
		
		if (key->type == 's') {
			c_entry->key = key->string;
			if (key->flags & CRTX_DIF_ALLOCATED_KEY)
				c_entry->flags |= CRTX_DIF_ALLOCATED_KEY;
			key->string = 0;
		}
		
		c_entry->type = 'D';
		n_items = 2;
		
		if (!(dc->flags & CRTX_CACHE_NO_EXT_FIELDS))
			n_items += 3;
		
		if (!c_entry->dict) {
			c_entry->dict = crtx_init_dict(0, n_items, 0);
		}
		
		memcpy(crtx_get_item_by_idx(c_entry->dict, 0), key, sizeof(struct crtx_dict_item));
		crtx_get_item_by_idx(c_entry->dict, 0)->key = "key";
		crtx_get_item_by_idx(c_entry->dict, 0)->flags &= ~CRTX_DIF_ALLOCATED_KEY;
		
		crtx_fill_data_item(crtx_get_item_by_idx(c_entry->dict, 1), 'p', "value", 0, 0, 0); // event->response.raw.size
		
		if (!(dc->flags & CRTX_CACHE_NO_EXT_FIELDS)) {
			struct crtx_dict_item *last_match, *creation, *it;
			
			creation = crtx_get_item_by_idx(c_entry->dict, 2);
			creation->type = 'U';
			creation->key = "creation";
			dc->get_time(creation);
			
			last_match = crtx_get_item_by_idx(c_entry->dict, 3);
			last_match->type = 'U';
			last_match->key = "last_match";
			last_match->uint64 = creation->uint64;
			
			it = crtx_get_item_by_idx(c_entry->dict, 4);
			it->type = 'U';
			it->key = "n_hits";
			it->uint64 = 0;
		}
		
		value_item = crtx_get_item_by_idx(c_entry->dict, 1);
		
// 			if (event->response.copy_raw) {
// 				crtx_get_item_by_idx(c_entry->dict, 1)->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				crtx_get_item_by_idx(c_entry->dict, 1)->pointer = event->response.raw.pointer;
// 				crtx_get_item_by_idx(c_entry->dict, 1)->flags |= CRTX_DIF_DONT_FREE_DATA;
// 			}
	}
	
	if (entry_data) {
		value_item->type = entry_data->type;
// 		value_item->uint64 = entry_data->uint64;
// 		crtx_print_dict_item(entry_data, 0);
		crtx_dict_copy_item(value_item, entry_data, 1);
	} else {
		if (event->response.type == 'D') {
			value_item->type = 'D';
// 			value_item->dict = crtx_dict_copy(event->response.dict);
			crtx_dict_ref(event->response.dict);
			value_item->dict = event->response.dict;
		} else {
			value_item->type = event->response.type;
			
			crtx_dict_copy_item(value_item, &event->response, 1);
		}
	}
	
// 		crtx_print_dict(dc->entries);
	
	return c_entry;
}

static int crtx_cache_task_cleanup(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache *cache = (struct crtx_cache*) userdata;
	struct crtx_dict_item *key;
	struct cache_sessiondata *sd;
	
	sd = *sessiondata;
	
	key = &sd->key;
	
	if (!sd->key.string) {
		free(sd);
		return 1;
	}
	
	if (!sd->cache_entry && (event->response.pointer || event->response.dict)) {
		pthread_mutex_lock(&cache->mutex);
		
		crtx_cache_add_entry(cache, key, event, 0);
		
		pthread_mutex_unlock(&cache->mutex);
	}
	
	crtx_free_dict_item_data(key);
	
	free(sd);
	
	return 1;
}

void crtx_cache_remove_entry(struct crtx_cache *cache, struct crtx_dict_item *key) {
	struct crtx_cache *dc = cache;
	struct crtx_dict_item *c_entry;
	
// 	CRTX_DBG("new cache entry "); crtx_print_dict_item(key, 0); CRTX_DBG("\n");
	
	if (!dc->entries)
		return;
	
	c_entry = crtx_get_item(dc->entries, key->string);
	if (!c_entry)
		return;
	
	crtx_dict_remove_item(dc->entries, key->string);
}

int crtx_cache_no_add(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event) {
	return 0;
}

int crtx_cache_update_on_hit(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if (c_entry->type != 'D') {
		CRTX_ERROR("crtx_cache_update_on_hit: %c != D\n", c_entry->type);
		return 0;
	}
	
	crtx_dict_unref(c_entry->dict);
	
	crtx_dict_ref(event->data.dict);
	c_entry->dict = event->data.dict;
	
	// do not set entry as response
	return 0;
}

int crtx_cache_add_on_miss(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	struct crtx_dict_item di;
	
	di.type = 'D';
	crtx_event_get_payload(event, 0, 0, &di.dict);
	
	crtx_cache_add_entry(cache, key, event, &di);
	
	return 0;
}


// char * crtx_get_trim_copy(char * str, unsigned int maxlen) {
// 	char * s, *ret;
// 	unsigned int i, len;
// 	
// 	if (!str || maxlen == 0)
// 		return 0;
// 	
// 	i=0;
// 	// get start and reduce maxlen
// 	while (isspace(*str) && i < maxlen) {str++;i++;}
// 	maxlen -= i;
// 	
// 	// goto end, and move backwards
// 	s = str + (strlen(str) < maxlen? strlen(str) : maxlen) - 1;
// 	while (s > str && isspace(*s)) {s--;}
// 	
// 	len = (s-str + 1 < maxlen?s-str + 1:maxlen);
// 	if (len == 0)
// 		return 0;
// 	
// 	ret = (char*) dls_malloc( len + 1 );
// 	strncpy(ret, str, len);
// 	ret[len] = 0;
// 	
// 	return ret;
// }

void crtx_cache_gettime_rt(struct crtx_dict_item *item) {
	int ret;
	struct timespec tp;
	
	ret = clock_gettime(CLOCK_REALTIME, &tp);
	if (ret) {
		CRTX_ERROR("clock_gettime failed: %d\n", ret);
		return;
	}
	
	item->type = 'U';
	item->uint64 = tp.tv_sec * 1000000000 + tp.tv_nsec;
}

struct crtx_task *create_cache_task(char *id, create_key_cb_t create_key) {
	struct crtx_cache *dc;
	struct crtx_task *task;
	int ret;
	
	task = new_task();
	
	dc = (struct crtx_cache*) calloc(1, sizeof(struct crtx_cache));
	
	ret = pthread_mutex_init(&dc->mutex, 0); CRTX_ASSERT(ret >= 0);
	
	dc->dict = crtx_init_dict(0, 0, 0);
	dc->dict->id = id;
	dc->get_time = &crtx_cache_gettime_rt;
	
// 	dc->config = crtx_get_item_by_idx(dc->dict, 0)->dict;
// 	dc->entries = crtx_get_item_by_idx(dc->dict, 1)->dict;
	
// 	ct = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	dc->create_key = create_key;
	dc->match_event = &crtx_cache_match_cb_t_strcmp;
	dc->on_hit = &crtx_cache_on_hit;
	dc->on_miss = 0;
	
	dc->task = task;
	
	task->id = id;
	task->position = 90;
	task->userdata = dc;
	task->handle = &crtx_cache_task_handler;
	task->cleanup = &crtx_cache_task_cleanup;
	
	return task;
}

int crtx_create_cache_task(struct crtx_task **task, char *id, create_key_cb_t create_key) {
	*task = create_cache_task(id, create_key);
	if (!*task)
		return -1;
	
	return 0;
}

void crtx_free_cache(struct crtx_cache *cache) {
	crtx_dict_unref(cache->dict);
	free(cache);
}

void crtx_free_cache_task(struct crtx_task *task) {
	struct crtx_cache *cache;
	
	cache = (struct crtx_cache *) task->userdata;
	crtx_free_cache(cache);
	
	crtx_free_task(task);
}

char crtx_load_cache(struct crtx_cache *cache, char *path) {
#ifdef CRTX_WITH_JSON
	char ret;
	struct crtx_dict_item *cfg, *entries;
	
	ret = crtx_dict_json_from_file(&cache->dict, path, cache->dict->id);
	if (!ret) {
		CRTX_DBG("loading cache dict %s failed\n", cache->dict->id);
		return 0;
	}
	
	cfg = crtx_get_item(cache->dict, "config");
	if (cfg)
		cache->config = cfg->dict;
	
	entries = crtx_get_item(cache->dict, "entries");
	if (entries)
		cache->entries = entries->dict;
	
// 	crtx_print_dict(cache->dict);
	
	return 1;
#else
	return 0;
#endif
}
