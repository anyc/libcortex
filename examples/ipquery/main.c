
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <crtx/core.h>
#include <crtx/dict.h>

#include "ipquery.h"

static char *default_format = "%[action]s %[interface]s %[address]s\n";
static char *format = 0;

#define VERSION "0.1"

void help(char **argv) {
	fprintf(stdout, "Usage: %s [args] action\n", argv[0]);
	fprintf(stdout, "\n");
	fprintf(stdout, "%s (v" VERSION ") \n", argv[0]);
	fprintf(stdout, "\n");
	fprintf(stdout, "Actions\n");
	fprintf(stdout, "  get_ips\n");
	fprintf(stdout, "  monitor_ips\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Args:\n");
	fprintf(stdout, "  -h             show help\n");
	fprintf(stdout, "  -f <format>    Format string, possible ids: action scope interface family address\n");
	fprintf(stdout, "                 Default: \"%%{action} %%{interface} %%{address}\\n\"\n");
}

static void ipq_on_update(struct crtx_dict *dict) {
	char *s;
	
	crtx_dict_snaprintf(&s, 0, 0, format, dict);
	printf("%s", s);
}

int main(int argc, char **argv) {
	int rc, opt;
	struct crtx_dict *ips;
	struct crtx_dict_item *iter, *value;
	char *uformat, *action;
	
	
	action = "get_ips";
	uformat = 0;
	while ((opt = getopt (argc, argv, "hf:")) != -1) {
		switch (opt) {
			case 'h':
				help(argv);
				return 0;
			case 'f':
				uformat = strdup(optarg);
				break;
		}
	}
	if (optind < argc)
		action = argv[optind];
	
	if (uformat) {
		regex_t regex;
		regmatch_t rmatch;
		
		char *start = uformat;
		char *cformat = 0;
		size_t cformat_size = 0;
		size_t cpy_size = 0;
		int add;
		char buf[128];
		
		rc = regcomp(&regex, "%\\{[[:alnum:]]+\\}", REG_EXTENDED);
		if (rc) {
			regerror(rc, &regex, buf, sizeof(buf));
			fprintf(stderr, "regcomp() failed: %s\n", buf);
			exit(1);
		}
		
		while (1) {
			rc = regexec(&regex, start, 1, &rmatch, 0);
			if (!rc) {
				cpy_size = rmatch.rm_so;
				add = rmatch.rm_eo - rmatch.rm_so - 3;
			} else if (rc == REG_NOMATCH) {
				cpy_size = strlen(start);
				add = 0;
			} else {
				regerror(rc, &regex, buf, sizeof(buf));
				fprintf(stderr, "regexec(%s) failed: %s\n", start, buf);
				exit(1);
			}
			
			cformat = (char*) realloc(cformat, cformat_size + cpy_size + 2 + add + 2 + 1);
			memcpy(&cformat[cformat_size], start, cpy_size);
			cformat_size += cpy_size;
			
			if (!rc) {
				cformat[cformat_size] = '%';
				cformat[cformat_size+1] = '[';
				memcpy(&cformat[cformat_size+2], start + rmatch.rm_so + 2, add);
				cformat[cformat_size+2+add] = ']';
				cformat[cformat_size+2+add+1] = 's';
				cformat_size += add + 4;
				
				start = start + rmatch.rm_eo;
			} else if (rc == REG_NOMATCH)
				break;
		}
		cformat[cformat_size-1] = 0;
		
		regfree(&regex);
		
		format = cformat;
	} else {
		format = default_format;
	}
	
	ipq_init();
	crtx_handle_std_signals();
	
	if (!strcmp(action, "get_ips")) {
		rc = ipq_get_ips(&ips);
		for (iter = crtx_get_first_item(ips); iter; iter = crtx_get_next_item(ips, iter)) {
			value = crtx_get_item(iter->dict, "value");
			
			ipq_on_update(value->dict);
		}
	} else
	if (!strcmp(action, "monitor_ips")) {
		int rc;
		struct ipq_monitor *monitor;
		
		rc = ipq_ips_setup_monitor(&monitor);
		if (rc) {
			ipq_ips_finish_monitor(monitor);
			return rc;
		}
		
		monitor->on_update = &ipq_on_update;
		
		rc = ipq_ips_start_monitor(monitor);
		if (rc) {
			ipq_ips_finish_monitor(monitor);
			return rc;
		}
		
		// 	crtx_print_dict(((struct crtx_cache_task*) monitor->cache_task->userdata)->cache->entries);
		
// 		*ips = crtx_dict_create_copy(((struct crtx_cache_task*) monitor->cache_task->userdata)->cache->entries);
		ipq_loop();
		
		rc = ipq_ips_stop_monitor(monitor);
		if (rc) {
			ipq_ips_finish_monitor(monitor);
			return rc;
		}
		
		rc = ipq_ips_finish_monitor(monitor);
		if (rc)
			return rc;
	}
	
	ipq_finish();
	
	return 0;
}
