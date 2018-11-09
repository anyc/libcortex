
extern char crtx_verbosity;

struct crtx_event;
typedef char (*crtx_handle_task_t)(struct crtx_event *event, void *userdata, void **sessiondata);
