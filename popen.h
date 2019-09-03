#ifndef _CRTX_CURL_H
#define _CRTX_CURL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "core.h"

struct crtx_popen_listener;

struct crtx_popen_listener {
	struct crtx_listener_base base;
	
	pid_t pid;
	
	char *cmd;
};

struct crtx_listener_base *crtx_new_popen_listener(void *options);

void crtx_popen_init();
void crtx_popen_finish();

#endif
