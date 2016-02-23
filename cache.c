
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

void response_cache_on_hit(struct crtx_cache_task *rc, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if ( !(rc->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) )
		c_entry = crtx_get_item(c_entry->ds, "value");
	
	DBG("cache hit for key ");
	crtx_print_dict_item(c_entry, 0);
	
	crtx_dict_copy_item(&event->response.raw, c_entry, 0);
}

void response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *ditem;
	char ret;
	
	
// 	if (event->data.raw.type == 'p' && !event->data.dict)
// 		event->data.raw_to_dict(&event->data);
	
	ret = ct->create_key(event, &ct->key);
	if (!ret) {
		DBG("no key created for \"%s\", ignoring\n", event->type);
		ct->key.string = 0;
		return;
	}
	
	
	if (ct->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT)
		ct->match_event = &rcache_match_cb_t_strcmp;
	else
		ct->match_event = &rcache_match_cb_t_regex;
	
	
	pthread_mutex_lock(&ct->cache->mutex);
	
	ditem = ct->match_event(ct->cache, &ct->key, event);
	
	if (ditem && !(ct->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) && !(ct->cache->flags & CRTX_CACHE_NO_TIMES)) {
		struct crtx_dict_item *creation, *last_match, now;
		uint64_t timeout;
		
		ret = crtx_get_value(ct->cache->config, "timeout", 'U', &timeout, sizeof(timeout));
		if (ret) {
			// check if 
			ct->get_time(&now);
			
			creation = crtx_get_item(ditem->ds, "creation");
			
// 			printf("%zu \n", now.uint64 - creation->uint64 + timeout);
			if (now.uint64 > creation->uint64 + timeout) {
				struct crtx_dict_item *key_item;
				DBG("dropping cache entry\n");
				
				key_item = crtx_get_item(ditem->ds, "key");
				
				crtx_free_dict_item(key_item);
				key_item->type = 0;
				
				ditem = 0;
			}
			
			if (ditem) {
				// update
				last_match = crtx_get_item(ditem->ds, "last_match");
				ct->get_time(last_match);
			}
		}
	}
	
	if (ditem) {
		ct->on_hit(ct, &ct->key, event, ditem);
		
		ct->cache_entry = ditem;
		
		pthread_mutex_unlock(&ct->cache->mutex);
		
// 		crtx_print_dict(ct->cache->entries);
		
		return;
	} else
		ct->cache_entry = 0;
	
	pthread_mutex_unlock(&ct->cache->mutex);
	
	if (ct->on_miss)
		ct->on_miss(ct, &ct->key, event);
}

void crtx_cache_add_entry(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_cache *dc = ct->cache;
	struct crtx_dict_item *cache_item;
	char do_add;
	
	pthread_mutex_lock(&dc->mutex);
	
	do_add = 1;
	if (ct->on_add)
		do_add = ct->on_add(ct, &ct->key, event);
	
	if (do_add) {
		struct crtx_dict_item *ditem;
		
		DBG("new cache entry ");
		crtx_print_dict_item(key, 0);
		
		if (!dc->entries) {
			struct crtx_dict_item *e;
			
			crtx_alloc_item(dc->dict);
			
// 			e = crtx_get_item_by_idx(dc->dict, 1);
			e = crtx_alloc_item(dc->dict);
			e->type = 'D';
			e->ds = crtx_init_dict(0, 0, 0);
			dc->entries = e->ds;
			
// 			e = crtx_get_item_by_idx(dc->entries, 0);
// 			e->type = 'D';
// 			e->ds = 0;
		}
		
		if (dc->flags & CRTX_CACHE_SIMPLE_LAYOUT) {
			if (key->type != 's')
				ERROR("wrong dictionary structure for simple cache layout\n");
			
			ditem = crtx_alloc_item(dc->entries);
			
			ditem->key = key->string;
			ditem->flags |= DIF_KEY_ALLOCATED;
			ditem->type = 'p';
			key->string = 0;
			
			cache_item = ditem;
// 			if (event->response.copy_raw) {
// 				ditem->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				ditem->pointer = event->response.raw.pointer;
// 				ditem->flags |= DIF_DATA_UNALLOCATED;
// 			}
// 			ditem->size = event->response.raw.size;
		} else {
			unsigned char n_items;
			
			// check if there is an empty cache entry
			ditem = crtx_get_first_item(dc->entries);
			while (ditem) {
				struct crtx_dict_item *it;
				
				if (ditem->type != 'D')
					ERROR("cache item type %c != D\n", ditem->type);
				
				it = crtx_get_item(ditem->ds, "key");
				if (!it || it->type == 0)
					break;
				
				ditem = crtx_get_next_item(dc->entries, ditem);
			}
			if (!ditem)
				ditem = crtx_alloc_item(dc->entries);
			
			
			ditem->type = 'D';
			n_items = 2;
			
			if (!(ct->cache->flags & CRTX_CACHE_NO_TIMES))
				n_items += 2;
			
			if (!ditem->ds)
				ditem->ds = crtx_init_dict(0, n_items, 0);
			
			memcpy(crtx_get_item_by_idx(ditem->ds, 0), key, sizeof(struct crtx_dict_item));
			crtx_get_item_by_idx(ditem->ds, 0)->key = "key";
			crtx_get_item_by_idx(ditem->ds, 0)->flags &= ~DIF_KEY_ALLOCATED;
			crtx_fill_data_item(crtx_get_item_by_idx(ditem->ds, 1), 'p', "value", 0, event->response.raw.size, 0);
			
			if (key->type == 's') {
				ditem->key = key->string;
				key->string = 0;
			}
			
			if (!(ct->cache->flags & CRTX_CACHE_NO_TIMES)) {
				struct crtx_dict_item *last_match, *creation;
				
				creation = crtx_get_item_by_idx(ditem->ds, 2);
				creation->type = 'U';
				creation->key = "creation";
				ct->get_time(creation);
				
				last_match = crtx_get_item_by_idx(ditem->ds, 3);
				last_match->type = 'U';
				last_match->key = "last_match";
				last_match->uint64 = creation->uint64;
			}
			
			cache_item = crtx_get_item_by_idx(ditem->ds, 1);
			
// 			if (event->response.copy_raw) {
// 				crtx_get_item_by_idx(ditem->ds, 1)->pointer = event->response.copy_raw(&event->response);
// 			} else {
// 				crtx_get_item_by_idx(ditem->ds, 1)->pointer = event->response.raw.pointer;
// 				crtx_get_item_by_idx(ditem->ds, 1)->flags |= DIF_DATA_UNALLOCATED;
// 			}
		}
		
		if (event->response.dict) {
			cache_item->type = 'D';
			cache_item->ds = crtx_dict_copy(event->response.dict);
		} else {
			cache_item->type = event->response.raw.type;
			
			crtx_dict_copy_item(cache_item, &event->response.raw, 1);
		}
		
// 		crtx_print_dict(dc->entries);
	}
	
	pthread_mutex_unlock(&dc->mutex);
}

void response_cache_task_cleanup(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *key;
	
	key = &ct->key;
	
	if (!ct->key.string)
		return;
	
	if (!ct->cache_entry && event->response.raw.pointer) {
		crtx_cache_add_entry(ct, key, event);
	}
	
	crtx_free_dict_item(key);
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
	ct->on_hit = &response_cache_on_hit;
	ct->on_miss = 0;
	ct->cache = dc;
	ct->get_time = &crtx_cache_gettime_rt;
	
	task->id = "response_cache";
	task->position = 90;
	task->userdata = ct;
	task->handle = &response_cache_task;
	task->cleanup = &response_cache_task_cleanup;
	
	return task;
}

void free_response_cache(struct crtx_cache *dc) {
	free_dict(dc->entries);
	
	free(dc);
}

char crtx_load_cache(struct crtx_cache *cache, char *path) {
	char ret;
	struct crtx_dict_item *cfg, *entries;
// 	char *conf_id;
// 	size_t id_len;
// 	
// 	id_len = strlen(cache->entries->id);
// 	conf_id = (char*) malloc(id_len);
// 	sprintf(conf_id, "%s.config", 
	
	ret = crtx_load_dict(&cache->dict, path, cache->dict->id);
	if (!ret) {
		DBG("loading cache dict %s failed\n", cache->dict->id);
		return 0;
	}
	
	crtx_print_dict(cache->dict);
	
	cfg = crtx_get_item(cache->dict, "config");
	if (cfg)
		cache->config = cfg->ds;
	
	entries = crtx_get_item(cache->dict, "entries");
	if (entries)
		cache->entries = entries->ds;
	
// 	ret = crtx_load_dict(cache->entries, path, cache->entries->id);
// 	if (!ret)
// 		ERROR("loading cache data failed\n", cache->entries->id);
	
	return 1;
}
