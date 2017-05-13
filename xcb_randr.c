
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "xcb_randr.h"

#define QRY(cmd, conn, error, ...) cmd ## _reply(conn, cmd(conn, __VA_ARGS__), & (error))

// char *xcb_randr_msg_etype[] = { "xcb_randr/crtc_change", "xcb_randr/output_change", 0 };

struct edid_monitor_descriptor {
	uint16_t zero0;
	uint8_t  zero1;
	uint8_t  desc_type;
	uint8_t  zero2;
	uint8_t  data[13];
};

char crtx_xcb_randr_get_edid(struct crtx_xcb_randr_listener *xrlist, xcb_randr_output_t output, unsigned char **edid, int *length, void **to_free) {
	xcb_generic_error_t* error;
	xcb_randr_get_output_property_reply_t *out_prop;
	
	
	xcb_intern_atom_reply_t *atom = QRY(xcb_intern_atom, xrlist->conn, error,
									1,
									4,
									"EDID");
	if (error) {
		ERROR("xcb_intern_atom returned: %d\n", error->error_code);
		free(error);
		return 1;
	}
	
	out_prop = QRY(xcb_randr_get_output_property, xrlist->conn, error,
				output,
				atom->atom,
				XCB_GET_PROPERTY_TYPE_ANY,
				0,
				256,
				0,
				0
			);
	free(atom);
	if (error) {
		ERROR("xcb_randr_get_output_property returned: %d\n", error->error_code);
		free(error);
		free(out_prop);
		return 1;
	}
	
	*to_free = out_prop;
	*edid = xcb_randr_get_output_property_data(out_prop);
	*length = xcb_randr_get_output_property_data_length(out_prop);
	
	if ((*edid == NULL) || (*length < 1)) {
		ERROR("xcb_randr_get_output_property_data returned no data\n");
		free(out_prop);
		return 1;
	}
	
// 	FILE *f = fopen("test.edid", "w");
// 	fwrite(*edid, *length, 1, f);
// 	fclose(f);
	
	unsigned char sum, i;
	sum = 0;
	for (i=0; i<128; i++)
		sum += (*edid)[i];
	
	if (sum) {
		ERROR("received EDID has wrong checksum\n");
		free(out_prop);
		return 1;
	}
	
	unsigned char magic[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
	if (memcmp(magic, *edid, 8)) {
		ERROR("received EDID has wrong header\n");
		free(out_prop);
		return 1;
	}
	
	return 0;
}

char crtx_xcb_randr_crtc_change_t2dict(struct xcb_randr_crtc_change_t *ptr, struct crtx_dict **dict_ptr) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "mode", ptr->mode, sizeof(ptr->mode), 0);
	
	di = crtx_alloc_item(dict);
	if (ptr->mode == XCB_NONE)
		crtx_fill_data_item(di, 's', "state", "off", 0, CRTX_DIF_DONT_FREE_DATA);
	else
		crtx_fill_data_item(di, 's', "state", "on", 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "timestamp", ptr->timestamp, sizeof(ptr->timestamp), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "window", ptr->window, sizeof(ptr->window), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "crtc", ptr->crtc, sizeof(ptr->crtc), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "rotation", ptr->rotation, sizeof(ptr->rotation), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "pad0", 0, 0, 0);
	
	// TypeKind.UCHAR
	di->dict = crtx_init_dict("uu", 2, 0);
	{
		crtx_fill_data_item(&di->dict->items[0], 'u', 0, ptr->pad0[0], sizeof(ptr->pad0[0]), 0);
		crtx_fill_data_item(&di->dict->items[1], 'u', 0, ptr->pad0[1], sizeof(ptr->pad0[1]), 0);
	}
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "x", ptr->x, sizeof(ptr->x), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'i', "y", ptr->y, sizeof(ptr->y), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "width", ptr->width, sizeof(ptr->width), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "height", ptr->height, sizeof(ptr->height), 0);
	
	return 0;
}

char crtx_xcb_randr_output_change_t2dict(struct xcb_randr_output_change_t *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "timestamp", ptr->timestamp, sizeof(ptr->timestamp), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "config_timestamp", ptr->config_timestamp, sizeof(ptr->config_timestamp), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "window", ptr->window, sizeof(ptr->window), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "output", ptr->output, sizeof(ptr->output), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "crtc", ptr->crtc, sizeof(ptr->crtc), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "mode", ptr->mode, sizeof(ptr->mode), 0);
	
	di = crtx_alloc_item(dict);
	if (ptr->mode == XCB_NONE)
		crtx_fill_data_item(di, 's', "state", "off", 0, CRTX_DIF_DONT_FREE_DATA);
	else
		crtx_fill_data_item(di, 's', "state", "on", 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "rotation", ptr->rotation, sizeof(ptr->rotation), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "connection", ptr->connection, sizeof(ptr->connection), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "subpixel_order", ptr->subpixel_order, sizeof(ptr->subpixel_order), 0);
	
	return 0;
}

char crtx_xcb_randr_get_output_info_reply_t2dict(struct xcb_randr_get_output_info_reply_t *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "response_type", ptr->response_type, sizeof(ptr->response_type), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "status", ptr->status, sizeof(ptr->status), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "sequence", ptr->sequence, sizeof(ptr->sequence), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "length", ptr->length, sizeof(ptr->length), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "timestamp", ptr->timestamp, sizeof(ptr->timestamp), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "crtc", ptr->crtc, sizeof(ptr->crtc), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "mm_width", ptr->mm_width, sizeof(ptr->mm_width), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "mm_height", ptr->mm_height, sizeof(ptr->mm_height), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "connection", ptr->connection, sizeof(ptr->connection), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "subpixel_order", ptr->subpixel_order, sizeof(ptr->subpixel_order), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "num_crtcs", ptr->num_crtcs, sizeof(ptr->num_crtcs), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "num_modes", ptr->num_modes, sizeof(ptr->num_modes), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "num_preferred", ptr->num_preferred, sizeof(ptr->num_preferred), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "num_clones", ptr->num_clones, sizeof(ptr->num_clones), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "name_len", ptr->name_len, sizeof(ptr->name_len), 0);
	
	return 0;
}

struct crtx_dict * crtx_output_change2dict(struct crtx_xcb_randr_listener *xrlist, xcb_randr_output_change_t *oevent) {
	xcb_randr_get_output_info_reply_t *output_info;
	xcb_generic_error_t* error;
	int i;
	struct crtx_dict *dict, *oi_dict;
	struct crtx_dict_item *di, *edi;
	char *name;
	size_t length;
	
	
	dict = crtx_init_dict(0, 0, 0);
	
	crtx_xcb_randr_output_change_t2dict(oevent, &dict);
	
	if (oevent->crtc != XCB_NONE) {
		output_info = QRY(xcb_randr_get_output_info, xrlist->conn, error, oevent->output, XCB_CURRENT_TIME);
		if (error) {
			ERROR("xcb_randr_get_output_info returned: %d\n", error->error_code);
			free(error);
			return 0;
		}
		
		di = crtx_alloc_item(dict);
		di->type = 'D';
		di->key = "output_info";
		
		crtx_xcb_randr_get_output_info_reply_t2dict(output_info, &di->dict);
		oi_dict = di->dict;
		
		
		length = xcb_randr_get_output_info_name_length(output_info);
		// name is not null-terminated!
		name = strndup((char *) xcb_randr_get_output_info_name(output_info), length);
		
		di = crtx_alloc_item(oi_dict);
		crtx_fill_data_item(di, 's', "output_name", name, length, 0);
		
		
		xcb_randr_get_crtc_info_reply_t *crtc_info;
		xcb_randr_crtc_t *crtc = xcb_randr_get_output_info_crtcs(output_info);
		for (i=0; i < output_info->num_crtcs; i++) {
			crtc_info = QRY(xcb_randr_get_crtc_info, xrlist->conn, error, crtc[i], XCB_CURRENT_TIME);
			if (error) {
				ERROR("xcb_randr_get_crtc_info returned: %d\n", error->error_code);
				free(error);
				continue;
			}
// 			printf("crtc %d: 0x%08x %d,%d %d,%d\n", i, crtc[i], crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height);
			
			if (crtc[i] == oevent->crtc) {
				di = crtx_alloc_item(oi_dict);
				di->type = 'D';
// 				di->key = (char*) malloc(5+floor(log10(abs(crtc[i])))+1);
// 				sprintf(di->key, "crtc_%u", crtc[i]);
// 				di->flags |= CRTX_DIF_ALLOCATED_KEY;
				di->key = "crtc_dict";
				
				di->dict = crtx_create_dict("uuuuu",
										"crtc", oevent->crtc, sizeof(oevent->crtc), 0,
										"x", (uint32_t) crtc_info->x, sizeof(uint32_t), 0,
										"y", (uint32_t) crtc_info->y, sizeof(uint32_t), 0,
										"width_px", (uint32_t) crtc_info->width, sizeof(uint32_t), 0,
										"height_px", (uint32_t) crtc_info->height, sizeof(uint32_t), 0
						);
			}
			free(crtc_info);
		}
		
		free(output_info);
		
		/*
		 * get EDID data
		 */
		
		unsigned char *edid;
		int edid_size;
		struct edid_monitor_descriptor *mon_desc;
		void *to_free;
		
		
		i = crtx_xcb_randr_get_edid(xrlist, oevent->output, &edid, &edid_size, &to_free);
		if (i)
			return dict;
		
		di = crtx_alloc_item(dict);
		di->type = 'D';
		
		di->key = "edid";
		di->dict = crtx_init_dict(0, 0, 0);
		
// 		printf("\tVendorName \"%c%c%c\"\n", (edid[8] >> 2 & 0x1f) + 'A' - 1, (((edid[8] & 0x3) << 3) | ((edid[9] & 0xe0) >> 5)) + 'A' - 1, (edid[9] & 0x1f) + 'A' - 1 );
		name = (char*) malloc(4);
		sprintf(name, "%c%c%c", (edid[8] >> 2 & 0x1f) + 'A' - 1, (((edid[8] & 0x3) << 3) | ((edid[9] & 0xe0) >> 5)) + 'A' - 1, (edid[9] & 0x1f) + 'A' - 1 );
		
		edi = crtx_alloc_item(di->dict);
		crtx_fill_data_item(edi, 's', "vendor_name", name, 4, 0);
		
		if (edid[21] && edid[22]) {
			edi = crtx_alloc_item(di->dict);
			crtx_fill_data_item(edi, 'u', "width_cm", edid[21], 4, 0);
			edi = crtx_alloc_item(di->dict);
			crtx_fill_data_item(edi, 'u', "height_cm", edid[22], 4, 0);
		}
		
		mon_desc = (struct edid_monitor_descriptor*) &edid[54];
		
		for (i=0;i<4;i++) {
			if (mon_desc->zero0 == 0 && mon_desc->zero1 == 0 && mon_desc->zero2 == 0) {
				int j;
				
				if (mon_desc->desc_type == 0xFC) {
					for (j=0;j<13;j++)
						if (mon_desc->data[j] == 0x0a)
							break;
					
					name = (char*) malloc(j+1);
					memcpy(name, mon_desc->data, j);
					name[j] = 0;
					
					edi = crtx_alloc_item(di->dict);
					crtx_fill_data_item(edi, 's', "monitor_name", name, j, 0);
				} else
				if (mon_desc->desc_type == 0xFF) {
					for (j=0;j<13;j++)
						if (mon_desc->data[j] == 0x0a)
							break;
					
					name = (char*) malloc(j+1);
					memcpy(name, mon_desc->data, j);
					
					edi = crtx_alloc_item(di->dict);
					crtx_fill_data_item(edi, 's', "monitor_serial", name, j, 0);
				}
			}
			mon_desc++;
		}
		
		free(to_free);
	}
	
	return dict;
}

char crtx_xcb_randr_output_property_t2dict(struct crtx_xcb_randr_listener *xrlist, struct xcb_randr_output_property_t *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	xcb_get_atom_name_reply_t *atom_reply;
	xcb_generic_error_t* error;
	
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "window", ptr->window, sizeof(ptr->window), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "output", ptr->output, sizeof(ptr->output), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "atom", ptr->atom, sizeof(ptr->atom), 0);
	
	atom_reply = QRY(xcb_get_atom_name, xrlist->conn, error, ptr->atom);
	if (error) {
		ERROR("xcb_get_atom_name returned: %d\n", error->error_code);
		free(error);
	} else {
		char *prop;
		
		prop = xcb_get_atom_name_name(atom_reply);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "property", prop, 0, 0);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "property_status", ptr->status?"DELETE":"NEW", 0, CRTX_DIF_DONT_FREE_DATA);
	}
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "timestamp", ptr->timestamp, sizeof(ptr->timestamp), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "status", ptr->status, sizeof(ptr->status), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "pad0", 0, 0, 0);
	
	// TypeKind.UCHAR
	di->dict = crtx_init_dict("uuuuuuuuuuu", 11, 0);
	{
		crtx_fill_data_item(&di->dict->items[0], 'u', 0, ptr->pad0[0], sizeof(ptr->pad0[0]), 0);
		crtx_fill_data_item(&di->dict->items[1], 'u', 0, ptr->pad0[1], sizeof(ptr->pad0[1]), 0);
		crtx_fill_data_item(&di->dict->items[2], 'u', 0, ptr->pad0[2], sizeof(ptr->pad0[2]), 0);
		crtx_fill_data_item(&di->dict->items[3], 'u', 0, ptr->pad0[3], sizeof(ptr->pad0[3]), 0);
		crtx_fill_data_item(&di->dict->items[4], 'u', 0, ptr->pad0[4], sizeof(ptr->pad0[4]), 0);
		crtx_fill_data_item(&di->dict->items[5], 'u', 0, ptr->pad0[5], sizeof(ptr->pad0[5]), 0);
		crtx_fill_data_item(&di->dict->items[6], 'u', 0, ptr->pad0[6], sizeof(ptr->pad0[6]), 0);
		crtx_fill_data_item(&di->dict->items[7], 'u', 0, ptr->pad0[7], sizeof(ptr->pad0[7]), 0);
		crtx_fill_data_item(&di->dict->items[8], 'u', 0, ptr->pad0[8], sizeof(ptr->pad0[8]), 0);
		crtx_fill_data_item(&di->dict->items[9], 'u', 0, ptr->pad0[9], sizeof(ptr->pad0[9]), 0);
		crtx_fill_data_item(&di->dict->items[10], 'u', 0, ptr->pad0[10], sizeof(ptr->pad0[10]), 0);
	}
	
	return 0;
}

void xcb_event_before_release_cb(struct crtx_event *event, void *userdata) {
	free(userdata);
}

static void xcb_handle_event(struct crtx_xcb_randr_listener *xrlist, xcb_generic_event_t *ev) {
	struct crtx_event *crtx_event;
	const xcb_query_extension_reply_t *randr;
	xcb_randr_notify_event_t *xcb_event;
	char *event_type;
	uint8_t type;
	int sent;
	void *data_ptr;
	
	
	type = ev->response_type & 0x7f;
	sent = !!(ev->response_type & 0x80);
	
	randr = xcb_get_extension_data(xrlist->conn, &xcb_randr_id);
	if (type == randr->first_event + XCB_RANDR_NOTIFY) {
		xcb_event = (xcb_randr_notify_event_t *) ev;
		
		event_type = 0;
		switch (xcb_event->subCode) {
			case XCB_RANDR_NOTIFY_CRTC_CHANGE:
				event_type = "xcb_randr/crtc_change";
				data_ptr = &xcb_event->u.cc;
				break;
			case XCB_RANDR_NOTIFY_OUTPUT_CHANGE:
				event_type = "xcb_randr/output_change";
				data_ptr = &xcb_event->u.oc;
				break;
			case XCB_RANDR_NOTIFY_OUTPUT_PROPERTY:
				event_type = "xcb_randr/output_property";
				data_ptr = &xcb_event->u.op;
				break;
			default:
				{
					char *s;
					#define XCBRANDR2STR(id) case id: s = #id; break;
					switch (xcb_event->subCode) {
// 						XCBRANDR2STR(XCB_RANDR_NOTIFY_CRTC_CHANGE)
// 						XCBRANDR2STR(XCB_RANDR_NOTIFY_OUTPUT_CHANGE)
// 						XCBRANDR2STR(XCB_RANDR_NOTIFY_OUTPUT_PROPERTY)
						XCBRANDR2STR(XCB_RANDR_NOTIFY_PROVIDER_CHANGE)
						XCBRANDR2STR(XCB_RANDR_NOTIFY_PROVIDER_PROPERTY)
						XCBRANDR2STR(XCB_RANDR_NOTIFY_RESOURCE_CHANGE)
					}
					INFO("unhandled xcb_randr notify event with subcode %d: %s\n", xcb_event->subCode, s);
				}
		}
		if (event_type) {
			crtx_event = crtx_create_event(event_type, data_ptr, 0);
			crtx_event->data.flags = CRTX_DIF_DONT_FREE_DATA;
			crtx_event->cb_before_release = &xcb_event_before_release_cb;
			crtx_event->cb_before_release_data = xcb_event;
			
			crtx_add_event(xrlist->parent.graph, crtx_event);
		} else {
			free(xcb_event);
		}
	} else {
		INFO("unhandled event of type %d (randr starts at: %d) %s\n", type, randr->first_event, sent ? " (generated)" : "");
		free(ev);
	}
}

static void *xcb_randr_tmain(void *data) {
	struct crtx_xcb_randr_listener *xrlist;
	xcb_generic_event_t *ev;
	
	xrlist = (struct crtx_xcb_randr_listener*) data;
	
	while ((ev = xcb_wait_for_event(xrlist->conn)) != NULL) {
		xcb_handle_event(xrlist, ev);
	}
	
	return 0;
}

static char xcb_randr_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_xcb_randr_listener *xrlist;
	xcb_generic_event_t *ev;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	xrlist = (struct crtx_xcb_randr_listener *) payload->data;
	
	ev = xcb_poll_for_event(xrlist->conn);
	
	xcb_handle_event(xrlist, ev);
	
	return 0;
}

static void crtx_free_xcb_randr_listener(struct crtx_listener_base *listener, void *userdata) {
	struct crtx_xcb_randr_listener *xrlist;
	
	xrlist = (struct crtx_xcb_randr_listener*) listener;
	
	xcb_disconnect(xrlist->conn);
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_xcb_randr_listener *xrlist;
	
	xrlist = (struct crtx_xcb_randr_listener*) listener;
	
	xcb_randr_select_input(xrlist->conn, xrlist->screen->root, XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);
	xcb_flush(xrlist->conn);
	
	return 0;
}

struct crtx_listener_base *crtx_new_xcb_randr_listener(void *options) {
	struct crtx_xcb_randr_listener *xrlist;
	const xcb_query_extension_reply_t *query;
	xcb_randr_query_version_reply_t *version;
	int ret;
	xcb_generic_error_t* error;
	
	
	xrlist = (struct crtx_xcb_randr_listener *) options;
	
	xrlist->conn = xcb_connect(NULL, NULL);
	ret = xcb_connection_has_error(xrlist->conn);
	if (ret) {
		ERROR("xcb_connect failed: %d\n", ret);
		return 0;
	}
	
	xrlist->screen = xcb_setup_roots_iterator(xcb_get_setup(xrlist->conn)).data;
	
	query = xcb_get_extension_data(xrlist->conn, &xcb_randr_id);
	if (!query || !query->present) {
		ERROR("randr extension not present\n");
		return 0;
	}
	
	version = QRY(xcb_randr_query_version, xrlist->conn, error, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
	if (!version || version->major_version < 1 || (version->major_version == 1 && version->minor_version < 2)) {
		ERROR("randr extension too old\n");
		return 0;
	}
	free(version);
	
	xrlist->parent.start_listener = &start_listener;
	xrlist->parent.free = &crtx_free_xcb_randr_listener;
	
	xrlist->parent.thread_job.fct = &xcb_randr_tmain;
	xrlist->parent.thread_job.fct_data = xrlist;
	
	xrlist->parent.el_payload.fd = xcb_get_file_descriptor(xrlist->conn);
	xrlist->parent.el_payload.event_flags = EPOLLIN;
	xrlist->parent.el_payload.data = xrlist;
	xrlist->parent.el_payload.event_handler = &xcb_randr_fd_event_handler;
	xrlist->parent.el_payload.event_handler_name = "xcb-randr fd handler";
	
	return &xrlist->parent;
}

void crtx_xcb_randr_init() {
}

void crtx_xcb_randr_finish() {
}


#ifdef CRTX_TEST

struct crtx_xcb_randr_listener xrlist;

static char xcb_randr_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	void *ptr;
	
	
	ptr = crtx_event_get_ptr(event);
	
	dict = 0;
	if (!strcmp(event->type, "xcb_randr/crtc_change")) {
		crtx_xcb_randr_crtc_change_t2dict(ptr, &dict);
	} else
	if (!strcmp(event->type, "xcb_randr/output_change")) {
		dict = crtx_output_change2dict(&xrlist, ptr);
	} else
	if (!strcmp(event->type, "xcb_randr/output_property")) {
		crtx_xcb_randr_output_property_t2dict(&xrlist, ptr, &dict);
	}
	
	
	if (dict) {
		crtx_print_dict(dict);
		crtx_dict_unref(dict);
	}
	
	return 0;
}

int xcb_randr_main(int argc, char **argv) {
	struct crtx_listener_base *lbase;
	char ret;
	
	
	memset(&xrlist, 0, sizeof(struct crtx_xcb_randr_listener));
	
	lbase = create_listener("xcb_randr", &xrlist);
	if (!lbase) {
		ERROR("create_listener(xcb_randr) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "xcb_randr_test", xcb_randr_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting xcb_randr listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(xcb_randr_main);

#endif
