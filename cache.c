
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <time.h>
#include <linux/limits.h>

#include "core.h"
#include "cache.h"
#include "threads.h"
#include "dict_inout.h"


struct crtx_dict_item * rcache_match_cb_t_strcmp(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_dict_item *ditem;
// 	struct crtx_dict_item *rkey;
	
	
	ditem = crtx_get_first_item(rc->entries);
	
	while (ditem) {
		if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
			return ditem;
		
// 		rkey = rcache_get_key(ditem);
// 		
// 		if (rkey && !crtx_cmp_item(key, ditem->key)) {
// 			return ditem;
// 		}
		
		ditem = crtx_get_next_item(rc->entries, ditem);
	}
	
	return 0;
}

struct crtx_dict_item * rcache_match_cb_t_regex(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event) {
	int ret;
	struct crtx_dict_item *ditem;
	struct crtx_dict_item *rkey, *regex;
	
	
	ditem = crtx_get_first_item(rc->entries);
	
	while (ditem) {
		regex = crtx_get_item(ditem->ds, "regexp");
		
// 		rkey = rcache_get_key(ditem);
		
		if (ditem->type != 'D') {
			ERROR("wrong dictionary structure for complex cache layout: %c != %c\n", ditem->type, 'D');
			ditem = crtx_get_next_item(rc->entries, ditem);
			continue;
		}
		rkey = crtx_get_item(ditem->ds, "key");
		
		if (!regex) {
			if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
				return ditem;
			
			if (rkey && !crtx_cmp_item(key, rkey))
				return ditem;
			
			ditem = crtx_get_next_item(rc->entries, ditem);
			continue;
		}
		
		if (regex->type == 'I' && regex->uint32 == 1) {
			regex->type = 'p';
			regex->pointer = (regex_t*) malloc(sizeof(regex_t));
			
			ret = regcomp(regex->pointer, rkey->string, 0);
			if (ret) {
				ERROR("Could not compile regex %s\n", rkey->string);
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
			ERROR("Regex match failed: %s\n", msgbuf);
			
// 			return 0;
		}
		
// 		rex++;
		ditem = crtx_get_next_item(rc->entries, ditem);
	}
	
	return 0;
}

char crtx_cache_on_hit(struct crtx_cache_task *rc, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if ( !(rc->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) )
		c_entry = crtx_get_item(c_entry->ds, "value");
	
	// if there is no value or type == 0, this cache entry is ignored
	if (!c_entry || c_entry->type == 0) {
		DBG("ignoring key ");
		crtx_print_dict_item(key, 0);
		
		return 0;
	}
	
	INFO("cache hit for key ");
	crtx_print_dict_item(key, 0);
	INFO(" ");
	crtx_print_dict_item(c_entry, 0);
	INFO("\n");
	
	crtx_dict_copy_item(&event->response.raw, c_entry, 0);
	
	return 1;
}

char response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *ditem, *passthrough;
	char ret;
	
	
// 	if (event->data.raw.type == 'p' && !event->data.dict)
// 		event->data.raw_to_dict(&event->data);
	
	ret = ct->create_key(event, &ct->key);
	if (!ret) {
		DBG("no key created for \"%s\", ignoring\n", event->type);
		ct->key.string = 0;
		return 1;
	}
	
	
	if (ct->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT)
		ct->match_event = &rcache_match_cb_t_strcmp;
	else
		ct->match_event = &rcache_match_cb_t_regex;
	
	
	pthread_mutex_lock(&ct->cache->mutex);
	
	ditem = ct->match_event(ct->cache, &ct->key, event);
	
	if (ditem && !(ct->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) && !(ct->cache->flags & CRTX_CACHE_NO_EXT_FIELDS)) {
		struct crtx_dict_item *di, now;
		uint64_t timeout;
		
		timeout = 0;
		ret = crtx_get_value(ct->cache->config, "timeout", 'U', &timeout, sizeof(timeout));
		if (ret && timeout > 0) {
			ct->get_time(&now);
			
			di = crtx_get_item(ditem->ds, "creation");
			
// 			printf("%zu \n", now.uint64 - creation->uint64 + timeout);
			if (di && now.uint64 > di->uint64 + timeout) {
				struct crtx_dict_item *key_item;
				DBG("dropping cache entry\n");
				
				key_item = crtx_get_item(ditem->ds, "key");
				
				crtx_free_dict_item(key_item);
				key_item->type = 0;
				
				ditem = 0;
			}
		}
		
		if (ditem) {
			// update
			di = crtx_get_item(ditem->ds, "last_match");
			if (!di) {
				di = crtx_alloc_item(ditem->ds);
				
				di->type = 'U';
				di->key = "last_match";
				ct->get_time(di);
			} else {
				ct->get_time(di);
			}
			
			di = crtx_get_item(ditem->ds, "n_hits");
			if (!di) {
				di = crtx_alloc_item(ditem->ds);
				
				di->type = 'U';
				di->key = "n_hits";
				di->uint64 = 0;
			} else {
				if (di->uint64 == UINT64_MAX)
					ERROR("cache uint64 value overflow\n");
				di->uint64++;
			}
			
			passthrough = crtx_get_item(ditem->ds, "passthrough");
		}
	} else
		passthrough = 0;
	
	if (ditem) {
		char response_available;
		
		response_available = ct->on_hit(ct, &ct->key, event, ditem);
		
		ct->cache_entry = ditem;
		
		pthread_mutex_unlock(&ct->cache->mutex);
		
// 		crtx_print_dict(ct->cache->entries);
		
		if (response_available && (!passthrough || CRTX_DICT_GET_NUMBER(passthrough) == 0))
			return 0;
		else
			return 1;
	} else
		ct->cache_entry = 0;
	
	if (ct->on_miss)
		ct->on_miss(ct, &ct->key, event);
	
	pthread_mutex_unlock(&ct->cache->mutex);
	
	return 1;
}

struct crtx_dict_item * crtx_cache_add_entry(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *entry_data) {
	struct crtx_cache *dc = ct->cache;
	struct crtx_dict_item *c_entry;
	struct crtx_dict_item *value_item;
	
	pthread_mutex_lock(&dc->mutex);
	
	DBG("new cache entry "); crtx_print_dict_item(key, 0); DBG("\n");
	
// 	crtx_print_dict_item(key, 0);
	
	if (!dc->entries) {
		struct crtx_dict_item *e;
		
		// for dc->config
		e = crtx_alloc_item(dc->dict);
		e->type = 'D';
		e->ds = 0;
		
		e = crtx_alloc_item(dc->dict);
		e->type = 'D';
		e->ds = crtx_init_dict(0, 0, 0);
		dc->entries = e->ds;
	}
	
	if (dc->flags & CRTX_CACHE_SIMPLE_LAYOUT) {
		if (key->type != 's')
			ERROR("wrong dictionary structure for simple cache layout\n");
		
		c_entry = crtx_alloc_item(dc->entries);
		
		c_entry->key = key->string;
		c_entry->flags |= DIF_KEY_ALLOCATED;
		c_entry->type = 'p';
		key->string = 0;
		
		value_item = c_entry;
// 			if (event->response.copy_raw) {
// 				c_entry->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				c_entry->pointer = event->response.raw.pointer;
// 				c_entry->flags |= DIF_DATA_UNALLOCATED;
// 			}
// 			c_entry->size = event->response.raw.size;
	} else {
		unsigned char n_items;
		
		// check if there is an empty cache entry
		c_entry = crtx_get_first_item(dc->entries);
		while (c_entry) {
			struct crtx_dict_item *it;
			
			if (c_entry->type != 'D')
				ERROR("cache item type %c != D\n", c_entry->type);
			
			it = crtx_get_item(c_entry->ds, "key");
			if (!it || it->type == 0)
				break;
			
			c_entry = crtx_get_next_item(dc->entries, c_entry);
		}
		if (!c_entry)
			c_entry = crtx_alloc_item(dc->entries);
		
		
		c_entry->type = 'D';
		n_items = 2;
		
		if (!(ct->cache->flags & CRTX_CACHE_NO_EXT_FIELDS))
			n_items += 3;
		
		if (!c_entry->ds)
			c_entry->ds = crtx_init_dict(0, n_items, 0);
		
		memcpy(crtx_get_item_by_idx(c_entry->ds, 0), key, sizeof(struct crtx_dict_item));
		crtx_get_item_by_idx(c_entry->ds, 0)->key = "key";
		crtx_get_item_by_idx(c_entry->ds, 0)->flags &= ~DIF_KEY_ALLOCATED;
		crtx_fill_data_item(crtx_get_item_by_idx(c_entry->ds, 1), 'p', "value", 0, 0, 0); // event->response.raw.size
		
		if (key->type == 's') {
			c_entry->key = key->string;
			key->string = 0;
		}
		
		if (!(ct->cache->flags & CRTX_CACHE_NO_EXT_FIELDS)) {
			struct crtx_dict_item *last_match, *creation, *it;
			
			creation = crtx_get_item_by_idx(c_entry->ds, 2);
			creation->type = 'U';
			creation->key = "creation";
			ct->get_time(creation);
			
			last_match = crtx_get_item_by_idx(c_entry->ds, 3);
			last_match->type = 'U';
			last_match->key = "last_match";
			last_match->uint64 = creation->uint64;
			
			it = crtx_get_item_by_idx(c_entry->ds, 4);
			it->type = 'U';
			it->key = "n_hits";
			it->uint64 = 0;
		}
		
		value_item = crtx_get_item_by_idx(c_entry->ds, 1);
		
// 			if (event->response.copy_raw) {
// 				crtx_get_item_by_idx(c_entry->ds, 1)->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				crtx_get_item_by_idx(c_entry->ds, 1)->pointer = event->response.raw.pointer;
// 				crtx_get_item_by_idx(c_entry->ds, 1)->flags |= DIF_DATA_UNALLOCATED;
// 			}
	}
	
	if (entry_data) {
		value_item->type = entry_data->type;
		
		crtx_dict_copy_item(value_item, entry_data, 1);
	} else {
		if (event->response.dict) {
			value_item->type = 'D';
			value_item->ds = crtx_dict_copy(event->response.dict);
		} else {
			value_item->type = event->response.raw.type;
			
			crtx_dict_copy_item(value_item, &event->response.raw, 1);
		}
	}
	
// 		crtx_print_dict(dc->entries);
	
	pthread_mutex_unlock(&dc->mutex);
	
	return c_entry;
}

char response_cache_task_cleanup(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *key;
	char do_add;
	
	key = &ct->key;
	
	if (!ct->key.string)
		return 1;
	
	if (!ct->cache_entry && (event->response.raw.pointer || event->response.dict)) {
		do_add = 1;
		if (ct->on_add)
			do_add = ct->on_add(ct, &ct->key, event);
		
		if (do_add)
			crtx_cache_add_entry(ct, key, event, 0);
	}
	
	crtx_free_dict_item(key);
	
	return 1;
}

char crtx_cache_no_add(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	return 0;
}

char crtx_cache_update_on_hit(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if (c_entry->type != 'D') {
		ERROR("crtx_cache_update_on_hit: %c != D\n", c_entry->type);
		return 0;
	}
	
	free_dict(c_entry->ds);
	
	c_entry->ds = crtx_dict_copy(event->data.dict);
	
	// do not set entry as response
	return 0;
}

void crtx_cache_add_on_miss(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	crtx_cache_add_entry(ct, key, event, 0);
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
		ERROR("clock_gettime failed: %d\n", ret);
		return;
	}
	
	item->type = 'U';
	item->uint64 = tp.tv_sec * 1000000000 + tp.tv_nsec;
}

struct crtx_task *create_response_cache_task(char *id, create_key_cb_t create_key) {
	struct crtx_cache *dc;
	struct crtx_cache_task *ct;
	struct crtx_task *task;
	int ret;
	
	task = new_task();
	
	dc = (struct crtx_cache*) calloc(1, sizeof(struct crtx_cache));
	
	ret = pthread_mutex_init(&dc->mutex, 0); ASSERT(ret >= 0);
	
	dc->dict = crtx_init_dict(0, 0, 0);
	dc->dict->id = id;
	
// 	dc->config = crtx_get_item_by_idx(dc->dict, 0)->ds;
// 	dc->entries = crtx_get_item_by_idx(dc->dict, 1)->ds;
	
	ct = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct->create_key = create_key;
	ct->match_event = &rcache_match_cb_t_strcmp;
	ct->on_hit = &crtx_cache_on_hit;
	ct->on_miss = 0;
	ct->cache = dc;
	ct->get_time = &crtx_cache_gettime_rt;
	
	task->id = id;
	task->position = 90;
	task->userdata = ct;
	task->handle = &response_cache_task;
	task->cleanup = &response_cache_task_cleanup;
	
	return task;
}

void crtx_flush_entries(struct crtx_cache *dc) {
	free_dict(dc->entries);
}

void free_response_cache(struct crtx_cache *dc) {
	crtx_flush_entries(dc);
	free(dc);
}

char crtx_load_cache(struct crtx_cache *cache, char *path) {
	char ret;
	struct crtx_dict_item *cfg, *entries;
	
	ret = crtx_load_dict(&cache->dict, path, cache->dict->id);
	if (!ret) {
		DBG("loading cache dict %s failed\n", cache->dict->id);
		return 0;
	}
	
	cfg = crtx_get_item(cache->dict, "config");
	if (cfg)
		cache->config = cfg->ds;
	
	entries = crtx_get_item(cache->dict, "entries");
	if (entries)
		cache->entries = entries->ds;
	
	crtx_print_dict(cache->dict);
	
	return 1;
}
