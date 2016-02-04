
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#include "core.h"
#include "controls.h"

#ifndef CONTROLDIR
#define CONTROLDIR "/usr/lib/cortexd/controls/"
#endif

struct crtx_control {
	char *path;
	char *basename;
	
	void *handle;
	
	char initialized;
	char (*init)();
	void (*finish)();
};

static struct crtx_control *controls = 0;
static unsigned int n_controls = 0;

static void load_control(char *path, char *basename) {
	struct crtx_control *p;
	
	printf("load control \"%s\"\n", path);
	
	n_controls++;
	controls = (struct crtx_control*) realloc(controls, sizeof(struct crtx_control)*n_controls);
	p = &controls[n_controls-1];
	
	memset(p, 0, sizeof(struct crtx_control));
	p->path = path;
	p->basename = basename;
	p->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	
	if (!p->handle)
		return;
	
	p->initialized = 1;
	p->init = dlsym(p->handle, "init");
	p->finish = dlsym(p->handle, "finish");
	
	if (p->init)
		p->initialized = p->init();
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
	load_dir(CONTROLDIR);
}

void crtx_controls_finish() {
	size_t i;
	
	for (i=0; i<n_controls; i++) {
		if (controls[i].initialized) {
			if (controls[i].finish)
				controls[i].finish();
			controls[i].initialized = 0;
		}
		
		if (controls[i].handle)
			dlclose(controls[i].handle);
		free(controls[i].path);
	}
	
	free(controls);
}