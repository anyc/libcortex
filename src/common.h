#ifndef CRTX_COMMON_H
#define CRTX_COMMON_H

#define CRTX_DECLARE_ALLOC_FUNCTION(lstnr) \
	struct crtx_ ## lstnr ## _listener * crtx_alloc_ ## lstnr ## _listener();
#define CRTX_DECLARE_CALLOC_FUNCTION(lstnr) \
	struct crtx_ ## lstnr ## _listener * crtx_ ## lstnr ## _calloc_listener(void);

#define CRTX_RET_GEZ(r) { if (r < 0) {CRTX_ERROR("%s:%d: %d %s\n", __FILE__, __LINE__, r, strerror(-r)); return(r); } }

extern char crtx_verbosity;

struct crtx_event;

typedef int (*crtx_handle_task_t)(struct crtx_event *event, void *userdata, void **sessiondata);

#endif
