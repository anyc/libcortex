
#ifndef _CRTX_V4L_H
#define _CRTX_V4L_H

struct crtx_v4l_listener {
	struct crtx_listener_base parent;
	
	char * device_path;
	int fd;
	
	struct crtx_dict *formats;
	struct v4l2_format format;
};

struct crtx_listener_base *crtx_new_v4l_listener(void *options);

void crtx_v4l_init();
void crtx_v4l_finish();

#endif
