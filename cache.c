
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
	struct crtx_cache_regex *rex;
	struct crtx_dict_item *ditem;
	struct crtx_dict_item *rkey;
	
	if (!rc->regexps)
		return 0;
	
	ditem = crtx_get_first_item(rc->entries);
	rex = &rc->regexps[0];
	
	while (ditem) {
		if (!rex->key_is_regex && key->type == 's' && ditem->key && !strcmp(ditem->key, key->string))
			return ditem;
		
		rkey = rcache_get_key(ditem);
		
		if (rex->key_is_regex) {
			if (!rex->initialized) {
				ret = regcomp(&rex->regex, rkey->string, 0);
				if (ret) {
					ERROR("Could not compile regex %s\n", rkey->string);
					exit(1);
				}
				rex->initialized = 1;
			}
			
			ret = regexec( &rex->regex, (char *) key, 0, NULL, 0);
			if (!ret) {
				return ditem;
			} else
			if (ret == REG_NOMATCH) {
				return 0;
			} else {
				char msgbuf[128];
				
				regerror(ret, &rex->regex, msgbuf, sizeof(msgbuf));
				ERROR("Regex match failed: %s\n", msgbuf);
				
				return 0;
			}
		}
		
		rex++;
		ditem = crtx_get_next_item(rc->entries, ditem);
	}
	
	return 0;
}

void response_cache_on_hit(struct crtx_cache_task *rc, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	if (!rc->cache->simple)
		c_entry = crtx_get_item(c_entry->ds, "value");
	
	if (c_entry->type != 'p') {
		ERROR("cache type mismatch: %c != %c\n", c_entry->type, 'p');
		return;
	}
	
	DBG("cache hit for key ");
	crtx_print_dict_item(key, 0);
	
	event->response.raw = c_entry->pointer;
	event->response.raw_size = c_entry->size;
}

void response_cache_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *ditem;
	char ret;
	
	ret = ct->create_key(event, &ct->key);
	if (!ret) {
		ERROR("error creating key\n");
		return;
	}
	
	pthread_mutex_lock(&ct->cache->mutex);
	
	ditem = ct->match_event(ct->cache, &ct->key, event);
	if (ditem) {
		ct->on_hit(ct, &ct->key, event, ditem);
		
		ct->cache_entry = ditem;
		
		pthread_mutex_unlock(&ct->cache->mutex);
		
		return;
	} else
		ct->cache_entry = 0;
	
	pthread_mutex_unlock(&ct->cache->mutex);
	
	if (ct->on_miss)
		ct->on_miss(ct, &ct->key, event);
}

void crtx_cache_add_entry(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_cache *dc = ct->cache;
	char do_add;
	
	pthread_mutex_lock(&dc->mutex);
	
	do_add = 1;
	if (ct->on_add)
		do_add = ct->on_add(ct, &ct->key, event);
	
	if (do_add) {
		struct crtx_dict_item *ditem;
		
		dc->n_regexps++;
		dc->regexps = (struct crtx_cache_regex *) realloc(dc->regexps, sizeof(struct crtx_cache_regex)*dc->n_regexps);
		memset(&dc->regexps[dc->n_regexps-1], 0, sizeof(struct crtx_cache_regex));
		
		ditem = crtx_alloc_item(dc->entries);
		
		if (dc->simple) {
			if (key->type != 's')
				ERROR("wrong dictionary structure for simple cache layout\n");
			
			ditem->key = key->string;
			ditem->flags |= DIF_KEY_ALLOCATED;
			ditem->type = 'p';
			key->string = 0;
			
			if (event->response.copy_raw) {
				ditem->pointer = event->response.copy_raw(&event->response);
			} else {
				ditem->pointer = event->response.raw;
				ditem->flags |= DIF_DATA_UNALLOCATED;
			}
			ditem->size = event->response.raw_size;
		} else {
			char sign[3];
			ditem->type = 'D';
			
			sign[0] = key->type;
			sign[1] = 'p';
			sign[2] = 0;
			
			ditem->ds = crtx_init_dict(sign, 0);
			memcpy(crtx_get_item_by_idx(ditem->ds, 0), key, sizeof(struct crtx_dict_item));
			crtx_get_item_by_idx(ditem->ds, 0)->key = "key";
			crtx_fill_data_item(crtx_get_item_by_idx(ditem->ds, 1), 'p', "value", 0, event->response.raw_size, 0);
			
			if (key->type == 's') {
				ditem->key = key->string;
				key->string = 0;
			}
			
			if (event->response.copy_raw) {
				crtx_get_item_by_idx(ditem->ds, 1)->pointer = event->response.copy_raw(&event->response);
			} else {
				crtx_get_item_by_idx(ditem->ds, 1)->pointer = event->response.raw;
				crtx_get_item_by_idx(ditem->ds, 1)->flags |= DIF_DATA_UNALLOCATED;
			}
		}
		
		crtx_print_dict(dc->entries);
	}
	
	pthread_mutex_unlock(&dc->mutex);
}

void response_cache_task_cleanup(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct = (struct crtx_cache_task*) userdata;
	struct crtx_dict_item *key;
	
	key = &ct->key;
	
	if (!ct->cache_entry && event->response.raw) {
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


struct crtx_task *create_response_cache_task(char *signature, create_key_cb_t create_key) {
	struct crtx_cache *dc;
	struct crtx_cache_task *ct;
	struct crtx_task *task;
	int ret;
	
	task = new_task();
	
	dc = (struct crtx_cache*) calloc(1, sizeof(struct crtx_cache));
	
	ret = pthread_mutex_init(&dc->mutex, 0); ASSERT(ret >= 0);
	
	dc->entries = crtx_init_dict(0, 0);
	
	ct = (struct crtx_cache_task*) calloc(1, sizeof(struct crtx_cache_task));
	ct->create_key = create_key;
	ct->match_event = &rcache_match_cb_t_strcmp;
	ct->on_hit = &response_cache_on_hit;
	ct->on_miss = 0;
	ct->cache = dc;
	
	task->id = "response_cache";
	task->position = 90;
	task->userdata = ct;
	task->handle = &response_cache_task;
	task->cleanup = &response_cache_task_cleanup;
	
	return task;
}

void free_response_cache(struct crtx_cache *dc) {
	size_t i;
	
	free_dict(dc->entries);
	
	for (i=0; i<dc->n_regexps; i++) {
		if (dc->regexps[i].initialized)
			regfree(&dc->regexps[i].regex);
	}
	
	free(dc->regexps);
	free(dc);
}
