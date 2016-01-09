
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#include "core.h"
#include "controls.h"

#ifndef CONTROLDIR
#define CONTROLDIR "/usr/lib/"
#endif

struct crtx_control {
	char *path;
	char *basename;
	
	void *handle;
	void (*init)();
	void (*finish)();
};

struct crtx_control *controls = 0;
unsigned int n_controls = 0;

static void load_control(char *path, char *basename) {
	struct crtx_control *p;
	
	printf("load control \"%s\"\n", path);
	
	n_controls++;
	controls = (struct crtx_control*) realloc(controls, sizeof(struct crtx_control)*n_controls);
	p = &controls[n_controls-1];
	
	p->path = path;
	p->basename = basename;
	p->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	
	p->init = dlsym(p->handle, "init");
	p->finish = dlsym(p->handle, "finish");
	
	if (p->init)
		p->init();
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
				
				load_control(fullname, dent->d_name);
			}
		}
		closedir (dhandle);
	} else {
		printf("cannot open control directory \"%s\"\n", directory);
	}
	
	return;
}

void crtx_controls_init() {
// 	main_cbs.add_task = &add_task;
// 	main_cbs.create_listener = &create_listener;
// 	main_cbs.wait_on_event = &wait_on_event;
// 	main_cbs.add_event = &add_event;
// 	main_cbs.new_event = &new_event;
// 	main_cbs.get_graph_for_event_type = &get_graph_for_event_type;
	
	load_dir(PLUGINDIR);
}

void crtx_controls_finish() {
	size_t i;
	
	for (i=0; i<n_controls; i++) {
		controls[i].finish();
	}
}