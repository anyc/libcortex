#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "recorder.h"

static int recorder_event_handler(struct crtx_event *event, void *userdata,
								  void **sessiondata)
{
	(void) userdata;
	(void) sessiondata;

	if (event->data.type == 'u')
		return 0;

	crtx_print_event(event, 0);
	putchar('\n');
	return 0;
}

int main(int argc, char **argv)
{
	struct crtx_recorder_listener listener;
	int rv;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <recorder-log-directory>\n", argv[0]);
		return 1;
	}

	crtx_init();
	crtx_handle_std_signals();

	crtx_recorder_clear_listener(&listener);
	listener.path = argv[1];
	listener.seek = -10;
	listener.event_cb = &crtx_recorder_gen_event_dict;

	rv = crtx_setup_listener("recorder", &listener);
	if (rv) {
		CRTX_ERROR("create_listener(recorder) failed: %s\n", strerror(-rv));
		return 1;
	}

	crtx_create_task(listener.base.graph, 0, "recorder_event",
					 &recorder_event_handler, 0);

	rv = crtx_start_listener(&listener.base);
	if (rv) {
		CRTX_ERROR("starting recorder listener failed: %s\n", strerror(-rv));
		return 1;
	}

	crtx_loop();
	crtx_finish();
	return 0;
}
