/*
 * libcortex recorder integration
 */

#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <librecorder.h>

#include "core.h"
#include "epoll.h"
#include "recorder.h"

static int emit_entry(struct crtx_recorder_listener *listener,
					  const RecorderEntry *entry)
{
	struct crtx_event *event;
	struct crtx_dict *dict;
	int rv;

	rv = crtx_create_event(&event);
	if (rv)
		return rv;

	dict = crtx_init_dict(0, 0, 0);
	if (!dict) {
		crtx_free_event(event);
		return -ENOMEM;
	}

#define ADD_STRING(name, value) \
	do { \
		if ((value)) { \
			rv = crtx_dict_new_item(dict, 's', (name), (char *) (value), 0, \
							CRTX_DIF_CREATE_DATA_COPY); \
			if (rv) \
				goto error; \
		} \
	} while (0)
#define ADD_U32(name, value) \
	do { \
		uint32_t item_value = (value); \
		rv = crtx_dict_new_item(dict, 'u', (name), item_value, sizeof(item_value), 0); \
		if (rv) \
			goto error; \
	} while (0)
#define ADD_U64(name, value) \
	do { \
		uint64_t item_value = (value); \
		rv = crtx_dict_new_item(dict, 'U', (name), item_value, sizeof(item_value), 0); \
		if (rv) \
			goto error; \
	} while (0)
#define ADD_U32_IF_SET(name, value) \
	do { \
		if ((value) != 0) \
			ADD_U32((name), (value)); \
	} while (0)

	ADD_U32("boot_seq", entry->boot_seq);
	ADD_STRING("boot_id", entry->boot_id);
	ADD_U64("segment_seq", entry->segment_seq);
	ADD_U64("frame_offset", entry->frame_offset);
	ADD_U32("frame_entry_index", entry->frame_entry_index);
	ADD_U64("realtime_ts", entry->realtime_ts);
	ADD_U64("monotonic_ts", entry->monotonic_ts);
	ADD_U32_IF_SET("pid", entry->pid);
	ADD_U32_IF_SET("uid", entry->uid);
	ADD_U32_IF_SET("gid", entry->gid);
	ADD_U32("priority", entry->priority);
	ADD_U32_IF_SET("errno", entry->errno_value);
	ADD_STRING("hostname", entry->hostname);
	ADD_STRING("comm", entry->comm);
	ADD_STRING("unit", entry->unit);
	ADD_STRING("exe", entry->exe);
	ADD_STRING("message", entry->message);
	ADD_STRING("message_id", entry->message_id);

#undef ADD_STRING
#undef ADD_U32
#undef ADD_U64
#undef ADD_U32_IF_SET

	event->type = CRTX_RECORDER_EVT_NEW_ENTRY_ID;
	event->description = CRTX_RECORDER_EVT_NEW_ENTRY;
	rv = crtx_fill_data_item(&event->data, 'D', 0, dict, 0, 0);
	if (rv) {
		crtx_dict_unref(dict);
		crtx_free_event(event);
		return rv;
	}

	crtx_add_event(listener->base.graph, event);
	return 0;

error:
#undef ADD_STRING
#undef ADD_U32
#undef ADD_U64
#undef ADD_U32_IF_SET
	crtx_dict_unref(dict);
	crtx_free_event(event);
	return rv;
}

static int emit_entries(struct crtx_recorder_listener *listener)
{
	RecorderPlayer *reader;
	const RecorderEntry *entry;
	int rv;

	reader = listener->reader;

	while ((rv = rec_player_next(reader)) > 0) {
		rv = rec_player_get_entry(reader, &entry);
		if (rv)
			return rv;
		rv = emit_entry(listener, entry);
		if (rv)
			return rv;
	}

	return rv < 0 ? rv : 0;
}

static int emit_current_entry(struct crtx_recorder_listener *listener)
{
	const RecorderEntry *entry;
	int rv;

	rv = rec_player_get_entry(listener->reader, &entry);
	if (rv)
		return rv;

	return emit_entry(listener, entry);
}

int crtx_recorder_gen_event_dict(struct crtx_recorder_listener *listener, void *userdata)
{
	(void) userdata;
	return emit_entries(listener);
}

static int recorder_fd_event_handler(struct crtx_event *event, void *userdata,
									 void **sessiondata)
{
	struct crtx_recorder_listener *listener;
	int rv;

	(void) event;
	(void) sessiondata;
	listener = userdata;

	rv = rec_player_process(listener->reader);
	if (rv < 0) {
		CRTX_ERROR("rec_player_process() failed: %s\n", strerror(-rv));
		return rv;
	}

	if (rv != RECORDER_PROCESS_NOP) {
		if (listener->event_cb)
			return listener->event_cb(listener, listener->event_cb_data);
	}

	return 0;
}

static int start_listener(struct crtx_listener_base *base)
{
	struct crtx_recorder_listener *listener;
	RecorderPlayer *reader;
	int fd;
	int rv;
	int i;

	listener = (struct crtx_recorder_listener *) base;
	rv = rec_player_open(&reader, listener->path);
	if (rv) {
		CRTX_ERROR("rec_player_open() failed for \"%s\"\n", listener->path);
		return -ENOENT;
	}

	if (listener->private_key) {
		rv = rec_player_set_private_key(reader, listener->private_key);
		if (rv) {
			CRTX_ERROR("rec_player_set_private_key() failed\n");
			rec_player_close(reader);
			return rv;
		}
	}

	listener->reader = reader;
	fd = rec_player_get_fd(reader);
	if (fd < 0) {
		rec_player_close(reader);
		listener->reader = 0;
		return fd;
	}

	crtx_evloop_init_listener(&listener->base, 0, fd,
							  crtx_epoll_flags2crtx_event_flags(
								  rec_player_get_events(reader)),
							  0, &recorder_fd_event_handler,
							  listener, 0, 0);

	if (listener->seek > 0) {
		rv = rec_player_seek_head(reader);
		if (rv)
			return rv;
		for (i = 0; i < listener->seek; i++) {
			rv = rec_player_next(reader);
			if (rv <= 0)
				break;
		}
	} else if (listener->seek < 0) {
		int count = 0;

		rv = rec_player_seek_tail(reader);
		if (rv)
			return rv;
		for (i = 0; i < -listener->seek; i++) {
			rv = rec_player_previous(reader);
			if (rv <= 0)
				break;
			count++;
		}
		while (count-- > 0) {
			if (listener->event_cb == &crtx_recorder_gen_event_dict)
				rv = emit_current_entry(listener);
			else if (listener->event_cb)
				rv = listener->event_cb(listener, listener->event_cb_data);
			if (rv)
				return rv;
			rv = rec_player_next(reader);
			if (rv <= 0)
				break;
		}
	}

	return 0;
}

static int stop_listener(struct crtx_listener_base *base)
{
	struct crtx_recorder_listener *listener;

	listener = (struct crtx_recorder_listener *) base;
	if (listener->reader) {
		rec_player_close(listener->reader);
		listener->reader = 0;
	}

	return 0;
}

struct crtx_listener_base *crtx_setup_recorder_listener(void *options)
{
	struct crtx_recorder_listener *listener;

	listener = options;
	listener->base.start_listener = &start_listener;
	listener->base.stop_listener = &stop_listener;
	if (!listener->event_cb) {
		listener->event_cb = &crtx_recorder_gen_event_dict;
		listener->event_cb_data = listener;
	}

	return &listener->base;
}

void crtx_recorder_clear_listener(struct crtx_recorder_listener *listener)
{
	memset(listener, 0, sizeof(*listener));
}

CRTX_DEFINE_CALLOC_FUNCTION(recorder)

void crtx_recorder_init()
{
}

void crtx_recorder_finish()
{
}
