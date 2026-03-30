
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include <core.h>
#include <popen.h>
#include <netconf.h>
#include <uevents.h>

struct crtx_uevents_listener *uevents_lstnr;
struct crtx_netconf_listener *netconf_lstnr;

struct crtx_popen_listener *popen_lstnr;
char ** popen_args = (char*[]) {
	0,
	0
};

struct usd_event {
	char *stype;
	struct crtx_dict *dict;
	
	char ** env;
	unsigned int env_length;
	char *script;
	
	struct usd_event *next;
};

struct usd_event *events;

int process_event(struct usd_event *event);

void release_head_event() {
	int i;
	struct usd_event *tmp;
	
	if (events->env) {
		for (i=0; i<events->env_length; i++) {
			if (events->env[i])
				free(events->env[i]);
		}
		free(events->env);
	}
	
	if (events->script)
		free(events->script);
	events->script = 0;
	
	crtx_dict_unref(events->dict);
	
	tmp = events;
	events = events->next;
	free(tmp);
}

// callback that is executed after the child process terminates
static int popen_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED || event->type == CRTX_FORK_ET_CHILD_KILLED) {
		printf("event %p done with rc %d\n", events, event->data.int32);
		
		release_head_event();
		
		if (events)
			process_event(events);
	} else {
		printf("unexpected event\n");
	}
	
	return 0;
}

// start execution of the script using the popen listener
int start_popen(char *script, struct usd_event *event) {
	int rv;
	
	
	crtx_popen_clear_lstnr(popen_lstnr);
	
	popen_lstnr->filepath = script;
	popen_lstnr->envp = event->env;
	popen_lstnr->argv = popen_args;
	popen_args[0] = script;
	
	printf("executing \"%s\" for %p\n", script, event);
	
	rv = crtx_setup_listener("popen", popen_lstnr);
	if (rv) {
		fprintf(stderr, "create_listener(popen) failed\n");
	}
	
	if (!rv) {
		crtx_create_listener_task(&popen_lstnr->base, &popen_event_handler, 0);
		
		rv = crtx_start_listener(&popen_lstnr->base);
		if (rv) {
			fprintf(stderr, "starting popen listener failed\n");
		}
	}
	
	return 0;
}

// find a script for this event and execute it
int process_event(struct usd_event *event) {
	struct stat sb;
	DIR *dhandle;
	struct dirent *dent;
	int rv;
	
	
	printf("processing %p\n", event);
	
	if (getenv("USD_SCRIPT")) {
		start_popen(getenv("USD_SCRIPT"), event);
	} else
	if (access(SYSCONFDIR "/usdev", F_OK) == 0 && stat(SYSCONFDIR "/usdev", &sb) == 0 && sb.st_mode & S_IXUSR) {
		start_popen(SYSCONFDIR "/usdev", event);
	} else
	if (access(LIBEXECDIR "/usdev", F_OK) == 0 && stat(LIBEXECDIR "/usdev", &sb) == 0 && sb.st_mode & S_IXUSR) {
		start_popen(LIBEXECDIR "/usdev", event);
	} else {
		char directory[4096];
		char found;
		
		if (event->stype)
			snprintf(directory, sizeof(directory), SYSCONFDIR "/usdev.d/%s", event->stype);
		else
			snprintf(directory, sizeof(directory), SYSCONFDIR "/usdev.d/generic");
		
		dhandle = opendir(directory);
		if (dhandle != NULL) {
			found = 0;
			while ((dent = readdir(dhandle)) != NULL) {
				char * fpath;
				
				
				fpath = (char*) malloc(strlen(directory)+strlen(dent->d_name)+2);
				if (!fpath) {
					fprintf(stderr, "malloc failed\n");
					continue;
				}
				sprintf(fpath, "%s/%s", directory, dent->d_name);
				event->script = fpath;
				
				rv = lstat(fpath, &sb);
				if (rv) {
					free(fpath);
					continue;
				}
				
				if (
					(sb.st_mode & S_IXUSR)
					&& (S_ISLNK(sb.st_mode) || S_ISREG(sb.st_mode))
					)
				{
					start_popen(fpath, event);
					found = 1;
				} else {
					free(fpath);
					
				}
			}
			closedir(dhandle);
			
			if (!found) {
				release_head_event();
				if (events)
					process_event(events);
			}
		} else {
			fprintf(stderr, "no usdev executable found\n");
			release_head_event();
			if (events)
				process_event(events);
		}
	}
	
	return 0;
}

// create a event structure and append it to the queue
static int create_event(char *stype, char **env, unsigned int env_length, struct crtx_dict *dict) {
	struct usd_event *eit;
	char exec_now;
	
	
	if (events) {
		for (eit = events; eit && eit->next; eit = eit->next) {}
		eit->next = (struct usd_event*) calloc(1, sizeof(struct usd_event));
		eit = eit->next;
		
		exec_now = 0;
	} else {
		events = (struct usd_event*) calloc(1, sizeof(struct usd_event));
		eit = events;
		
		exec_now = 1;
	}
	
	eit->stype = stype;
	eit->dict = dict;
	eit->env = env;
	eit->env_length = env_length;
	
	crtx_dict_ref(dict);
	// crtx_print_dict(dict);
	
	printf("added event %p to queue\n", eit);
	
	if (exec_now)
		process_event(eit);
	
	return 0;
}

#define ADDENV(fmt, key, value) { \
	rv = asprintf(&env[env_idx], fmt, key, value); \
	if (rv > 0) { \
		env_idx++; \
	} else { \
		fprintf(stderr, "asprintf() failed: %s\n", strerror(errno)); \
	} \
}

// handle netconf events
static int netconf_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STOPPED) {
		crtx_init_shutdown();
	} else
	if (event->data.type == 'D') {
		char *stype;
		unsigned int env_length, env_idx;
		char **env;
		unsigned int i, slen;
		struct crtx_dict *flags_dict;
		struct crtx_dict_item *item;
		char *s;
		uint32_t uint32;
		int32_t int32;
		int rv;
		
		
		crtx_event_get_payload(event, 0, 0, &dict);
		
		env_length = 2;
		env_idx = 0;
		env = (char**) calloc(1, sizeof(char*) * env_length);
		
		ADDENV("%s=%s", "USD_ACTION", crtx_dict_get_string(dict, "action"));
		
		switch (event->type) {
			case CRTX_NETCONF_ET_INTF:
				stype = "net_interface";
				
				env_length += 8;
				env = (char**) realloc(env, sizeof(char*) * env_length);
				
				ADDENV("%s=%s", "USD_EV_TYPE", "net_interface");
				
				crtx_get_value(dict, "index", 'i', &int32, sizeof(int32));
				ADDENV("%s=%d", "USD_INDEX", int32);
				ADDENV("%s=%s", "USD_INTERFACE", crtx_dict_get_string(dict, "interface"));
				crtx_get_value(dict, "mtu", 'u', &uint32, sizeof(uint32));
				ADDENV("%s=%d", "USD_MTU", uint32);
				ADDENV("%s=%s", "USD_ADDRESS", crtx_dict_get_string(dict, "address"));
				ADDENV("%s=%s", "USD_BROADCAST", crtx_dict_get_string(dict, "broadcast"));
				ADDENV("%s=%s", "USD_QDISC", crtx_dict_get_string(dict, "qdisc"));
				
				s = 0;
				slen = 0;
				flags_dict = crtx_get_item(dict, "flags")->dict;
				for (i=0; i < flags_dict->signature_length; i++) {
					item = crtx_get_item_by_idx(flags_dict, i);
					s = (char*) realloc(s, strlen(item->string) + 1 + slen + 1);
					if (slen > 0) {
						s[slen] = ',';
						slen += 1;
					}
					sprintf(&s[slen], "%s", item->string);
					slen += strlen(item->string);
				}
				s[slen] = 0;
				ADDENV("%s=%s", "USD_FLAGS", s);
				free(s);
				break;
			case CRTX_NETCONF_ET_IP_ADDR:
				stype = "ip_address";
				
				env_length += 8;
				env = (char**) realloc(env, sizeof(char*) * env_length);
				
				ADDENV("%s=%s", "USD_EV_TYPE", "ip_address");
				
				ADDENV("%s=%s", "USD_SCOPE", crtx_dict_get_string(dict, "scope"));
				ADDENV("%s=%s", "USD_FAMILY", crtx_dict_get_string(dict, "family"));
				crtx_get_value(dict, "prefixlen", 'u', &uint32, sizeof(uint32));
				ADDENV("%s=%d", "USD_PREFIXLEN", uint32);
				ADDENV("%s=%s", "USD_ADDRESS", crtx_dict_get_string(dict, "address"));
				ADDENV("%s=%s", "USD_LOCAL", crtx_dict_get_string(dict, "local"));
				ADDENV("%s=%s", "USD_INTERFACE", crtx_dict_get_string(dict, "interface"));
				crtx_get_value(dict, "interface_index", 'i', &int32, sizeof(int32));
				ADDENV("%s=%d", "USD_INTERFACE_INDEX", int32);
				break;
			default:
				fprintf(stderr, "unexpected event type %d\n", event->type);
				return 1;
		}
		
		env[env_idx] = 0;
		
		create_event(stype, env, env_length, dict);
	}
	
	return 0;
}

// handle uevents from kernel
static int uevents_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	int i, rv;
	struct crtx_dict *dict;
	unsigned int env_length, env_idx;
	char **env;
	struct crtx_dict_item *item;
	char key[1024];
	char *subsystem;
	
	
	dict = crtx_uevents_raw2dict(event);
	
	env_length = (dict->signature_length + 3);
	env_idx = 0;
	env = (char**) calloc(1, sizeof(char*) * env_length);
	
	ADDENV("%s=%s", "USD_ACTION", crtx_dict_get_string(dict, "ACTION"));
	
	subsystem = crtx_dict_get_string(dict, "SUBSYSTEM");
	if (subsystem)
		ADDENV("%s=%s", "USD_EV_TYPE", subsystem);
	
	for (i=0; i<dict->signature_length; i++) {
		item = crtx_get_item_by_idx(dict, i);
		
		snprintf(key, sizeof(key), "USD_%s", item->key);
		switch (item->type) {
			case 's':
				ADDENV("%s=%s", key, item->string);
				break;
		}
	}
	
	env[env_idx] = 0;
	
	create_event(subsystem, env, env_length, dict);
	
	// dict is not associated with event
	crtx_dict_unref(dict);
	
	return 0;
}

int main(int argc, char **argv) {
	int rv;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	popen_lstnr = crtx_alloc_popen_listener();
	
	{
		netconf_lstnr = crtx_alloc_netconf_listener();
		netconf_lstnr->query_existing = 1;
		
		rv = crtx_setup_listener("netconf", netconf_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(netconf) failed\n");
			exit(1);
		}
		
		// we also want to receive status events of the listener in this graph
		netconf_lstnr->base.state_graph = netconf_lstnr->base.graph;
		netconf_lstnr->monitor_types = CRTX_NETCONF_INTF | CRTX_NETCONF_IPV4_ADDR | CRTX_NETCONF_IPV6_ADDR;
		
		crtx_create_task(netconf_lstnr->base.graph, 0, "netconf_event_handler", netconf_event_handler, 0);
		
		rv = crtx_start_listener(&netconf_lstnr->base);
		if (rv) {
			CRTX_ERROR("starting netconf listener failed: %s (%d)\n", strerror(-rv), rv);
			return 1;
		}
	}
	
	{
		uevents_lstnr = crtx_alloc_uevents_listener();
		
		rv = crtx_setup_listener("uevents", uevents_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(uevents) failed: %s\n", strerror(-rv));
			return rv;
		}
		
		crtx_create_task(uevents_lstnr->base.graph, 0, "uevents_test", uevents_test_handler, 0);
		
		rv = crtx_start_listener(&uevents_lstnr->base);
		if (rv) {
			CRTX_ERROR("starting uevents listener failed\n");
			return rv;
		}
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
