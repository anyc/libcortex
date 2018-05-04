#ifndef _CRTX_V4L_H
#define _CRTX_V4L_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

struct crtx_v4l_listener {
	struct crtx_listener_base parent;
	
	char * device_path;
	int fd;
	
	struct crtx_dict *formats;
	struct v4l2_format format;
	
	struct crtx_dict *controls;
	
	struct v4l2_event_subscription *subscriptions;
	unsigned int subscription_length;
};

struct crtx_listener_base *crtx_new_v4l_listener(void *options);

void crtx_v4l_init();
void crtx_v4l_finish();

char crtx_v4l2_event2dict(struct v4l2_event *ptr, struct crtx_dict **dict_ptr, struct crtx_dict *controls);

#endif
