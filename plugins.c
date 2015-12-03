
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#include "core.h"
#include "plugins.h"

struct crtx_plugin {
	char *path;
	char *basename;
	
	void *handle;
	void (*init)(struct main_cb *main_cbs);
	void (*finish)();
};

struct main_cb main_cbs;

struct crtx_plugin *plugins = 0;
unsigned int n_plugins = 0;

static void load_plugin(char *path, char *basename) {
	struct crtx_plugin *p;
	
	printf("load plugin \"%s\n", basename);
	
	n_plugins++;
	plugins = (struct crtx_plugin*) realloc(plugins, sizeof(struct crtx_plugin)*n_plugins);
	p = &plugins[n_plugins-1];
	
	p->path = path;
	p->basename = basename;
	p->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	
	p->init = dlsym(p->handle, "init");
	p->finish = dlsym(p->handle, "finish");
	
	if (p->init)
		p->init(&main_cbs);
}

static void load_dir(char * directory) {
	DIR *dhandle;
	struct dirent *dent;
	
	if (!directory)
		return;
	
	dhandle = opendir (directory);
	if (dhandle != NULL) {
		while ((dent = readdir (dhandle)) != NULL) {
			if (!strncmp(dent->d_name, "libcrtx_", 8)) {
				char * fullname = (char*) malloc(strlen(directory)+strlen(dent->d_name)+2);
				sprintf(fullname, "%s/%s", directory, dent->d_name);
				
				load_plugin(fullname, dent->d_name);
			}
		}
		closedir (dhandle);
	} else {
		printf("cannot open plugin directory \"%s\"\n", directory);
	}
	
	return;
}

void plugins_init() {
	main_cbs.add_task = &add_task;
	main_cbs.create_listener = &create_listener;
	main_cbs.wait_on_event = &wait_on_event;
	main_cbs.add_event = &add_event;
	main_cbs.new_event = &new_event;
	main_cbs.get_graph_for_event_type = &get_graph_for_event_type;
	
	load_dir("/home/anyc/Projekte/cortexd/");
}

void plugins_finish() {
	
}