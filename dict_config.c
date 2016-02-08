
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>

#include "core.h"
#include "dict.h"
#include "dict_config.h"
#include "dict_inout_json.h"

char *crtx_readfile(char *path) {
	FILE *f;
	size_t size, read_size;
	char *s;
	
	f = fopen(path, "r");
	if (!f) {
		printf("error reading file \"%s\": %s\n", path, strerror(errno));
		return 0;
	}
	
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	s = (char*) malloc(size + 1);
	read_size = fread(s, 1, size, f);
	if (read_size != size) {
		printf("error while reading file %s\n", path);
		return 0;
	}
	
	fclose(f);
	
	return s;
}

void crtx_load_config(struct crtx_config *config) {
	char *s;
	char file_path[PATH_MAX];
	char *config_path;
	
	config_path = getenv("CRTX_CFG_PATH");
	
	snprintf(file_path, PATH_MAX, "%s%s.json", config_path?config_path:"", config->name);
	
// 	int f = open(file_path, O_RDONLY);
// 	if (f == -1) {
// 		printf("error %s\n", strerror(errno));
// 		return;
// 	}
	
	s = crtx_readfile(file_path);
	if (!s)
		return;
	
	config->data = crtx_init_dict(0, 0);
	
	#ifdef WITH_JSON
	crtx_load_json_config(config, s);
	#endif
	
// 	close(f);
	
	crtx_print_dict(config->data);
}

void crtx_store_config(struct crtx_config *config) {
	int f = open(config->name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
	if (f == -1) {
		printf("error %s\n", strerror(errno));
		return;
	}
	
	// 	send_dict(crtx_wrapper_write, &f, d);
	
	close(f);
}

