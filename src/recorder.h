#ifndef _CRTX_RECORDER_H
#define _CRTX_RECORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <librecorder.h>

#include "core.h"

#define CRTX_RECORDER_EVT_NEW_ENTRY "recorder.new_entry"
#define CRTX_RECORDER_EVT_NEW_ENTRY_ID (1 << 0)

struct crtx_recorder_listener {
	struct crtx_listener_base base;

	const char *path;
	const char *private_key;
	RecorderPlayer *reader;
	int seek;

	int (*event_cb)(struct crtx_recorder_listener *listener, void *userdata);
	void *event_cb_data;
};

struct crtx_listener_base *crtx_setup_recorder_listener(void *options);
void crtx_recorder_clear_listener(struct crtx_recorder_listener *listener);
CRTX_DECLARE_CALLOC_FUNCTION(recorder)

void crtx_recorder_init();
void crtx_recorder_finish();

int crtx_recorder_gen_event_dict(struct crtx_recorder_listener *listener, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
