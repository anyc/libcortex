
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <time.h>

#include "core.h"
#include "cache.h"
#include "threads.h"

struct crtx_dict_item * rcache_get_key(struct crtx_dict_item *ditem) {
	struct crtx_dict_item *key_item;
	
	if (ditem->type != 'D') {
		ERROR("wrong dictionary structure for simple cache layout: %c != %c\n", ditem->type, 'D');
		return 0;
	}
	
	key_item = crtx_get_item(ditem->ds, "key");
	
	return key_item;
}

struct crtx_dict_item * rcache_match_cb_t_strcmp(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_dict_item *ditem;
	struct crtx_dict_item *rkey;
	
	ditem = crtx_get_first_item(rc->entries);
	
	while (ditem) {
		if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
			return ditem;
		
		rkey = rcache_get_key(ditem);
		
		if (rkey && !crtx_cmp_item(key, rkey)) {
			return ditem;
		}
		
		ditem = crtx_get_next_item(rc->entries, ditem);
	}
	
	return 0;
}

struct crtx_dict_item * rcache_match_cb_t_regex(struct crtx_cache *rc, struct crtx_dict_item *key, struct crtx_event *event) {
	int ret;
// 	struct crtx_cache_regex *rex;
	struct crtx_dict_item *ditem;
	struct crtx_dict_item *rkey, *regex;
	
// 	if (!rc->regexps)
// 		return 0;
	
	ditem = crtx_get_first_item(rc->entries);
// 	rex = &rc->regexps[0];
	
	while (ditem) {
// 		if (!rex->key_is_regex && key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
// 			return ditem;
		
		regex = crtx_get_item(ditem->ds, "regexp");
		
		rkey = rcache_get_key(ditem);
		
		if (!regex) {
			if (key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
				return ditem;
			
			if (rkey && !crtx_cmp_item(key, rkey))
				return ditem;
			
			continue;
		}
		
// 		if (!rex->initialized) {
// 			ret = regcomp(&rex->regex, rkey->string, 0);
// 			if (ret) {
// 				ERROR("Could not compile regex %s\n", rkey->string);
// 				exit(1);
// 			}
// 			rex->initialized = 1;
// 		}
		
// 		ret = regexec( &rex->regex, (char *) key, 0, NULL, 0);
		
		ret = regexec( (regex_t*) regex->pointer, (char *) key, 0, NULL, 0);
		if (!ret) {
			return ditem;
		} else
		if (ret == REG_NOMATCH) {
			return 0;
		} else {
			char msgbuf[128];
			
			regerror(ret, (regex_t*) regex->pointer, msgbuf, sizeof(msgbuf));
			ERROR("Regex match failed: %s\n", msgbuf);
			
			return 0;
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
	crtx_print_dict_item(key, 0);
	
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
	
	pthread_mutex_lock(&ct->cache->mutex);
	
	ditem = ct->match_event(ct->cache, &ct->key, event);
	
	if (ditem && !(ct->cache->flags & CRTX_CACHE_SIMPLE_LAYOUT) && !(ct->cache->flags & CRTX_CACHE_NO_TIMES)) {
		struct crtx_dict_item *creation, *last_match, now;
		uint64_t timeout;
		
		ret = crtx_get_value(ct->cache->config, "timeout", 'z', &timeout, sizeof(timeout));
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
		
// 		if (dc->flags & CRTX_CACHE_REGEXP) {
// 			dc->n_regexps++;
// 			dc->regexps = (struct crtx_cache_regex *) realloc(dc->regexps, sizeof(struct crtx_cache_regex)*dc->n_regexps);
// 			memset(&dc->regexps[dc->n_regexps-1], 0, sizeof(struct crtx_cache_regex));
// 		}
		
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
				
				it = crtx_get_item(ditem->ds, "key");
				if (it && it->type == 0)
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
				creation->type = 'z';
				creation->key = "creation";
				ct->get_time(creation);
				
				last_match = crtx_get_item_by_idx(ditem->ds, 3);
				last_match->type = 'z';
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

// void prefill_rcache(struct crtx_cache *rcache, char *file) {
// 	FILE *f;
// 	char buf[1024];
// 	char *indent;
// 	size_t slen;
// 	
// 	// 	if (strcmp(rcache->signature, "ss")) {
// 	// 		printf("rcache signature %s != ss\n", rcache->signature);
// 	// 		return;
// 	// 	}
// 	
// 	f = fopen(file, "r");
// 	
// 	if (!f)
// 		return;
// 	
// 	while (fgets(buf, sizeof(buf), f)) {
// 		if (buf[0] == '#')
// 			continue;
// 		
// 		indent = strchr(buf, '\t');
// 		
// 		if (!indent)
// 			continue;
// 		
// 		rcache->n_entries++;
// 		rcache->entries = (struct crtx_cache_entry *) realloc(rcache->entries, sizeof(struct crtx_cache_entry)*rcache->n_entries);
// 		memset(&rcache->entries[rcache->n_entries-1], 0, sizeof(struct crtx_cache_entry));
// 		
// 		slen = indent - buf;
// 		rcache->entries[rcache->n_entries-1].key = stracpy(buf, &slen);
// 		// 		printf("asd %s\n", (char*)rcache->entries[rcache->n_entries-1].key);
// 		if (rcache->signature[1] == 's') {
// 			slen = strlen(buf) - slen - 1;
// 			if (buf[strlen(buf)-1] == '\n')
// 				slen--;
// 			rcache->entries[rcache->n_entries-1].value = stracpy(indent+1, &slen);;
// 			rcache->entries[rcache->n_entries-1].value_size = slen;
// 			// 			printf("asd %s\n", (char*)rcache->entries[rcache->n_entries-1].response);
// 		} else
// 			if (rcache->signature[1] == 'u') {
// 				uint32_t uint;
// 				rcache->entries[rcache->n_entries-1].value_size = strlen(buf) - slen - 2;
// 				
// 				sscanf(indent+1, "%"PRIu32,  &uint);
// 				// 			printf("%"PRIu32"\n", uint);
// 				rcache->entries[rcache->n_entries-1].value = (void*)(uintptr_t) uint;
// 			}
// 	}
// 	
// 	if (ferror(stdin)) {
// 		fprintf(stderr,"Oops, error reading stdin\n");
// 		exit(1);
// 	}
// 	
// 	fclose(f);
// }

void crtx_cache_gettime_rt(struct crtx_dict_item *item) {
	int ret;
	struct timespec tp;
	
	ret = clock_gettime(CLOCK_REALTIME, &tp);
	if (ret) {
		ERROR("clock_gettime failed: %d\n", ret);
		return;
	}
	
	item->type = 'z';
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
	
	dc->entries = crtx_init_dict(0, 0, 0);
	
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
// 	size_t i;
	
	free_dict(dc->entries);
	
// 	for (i=0; i<dc->n_regexps; i++) {
// 		if (dc->regexps[i].initialized)
// 			regfree(&dc->regexps[i].regex);
// 	}
	
// 	free(dc->regexps);
	free(dc);
}
