
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

char get_edid(struct crtx_xcb_randr_listener *xrlist, xcb_randr_output_t output, unsigned char **edid, int *length) {
	xcb_generic_error_t* error;
	
	xcb_intern_atom_reply_t *atom = QRY(xcb_intern_atom, xrlist->conn, error,
									0,
									4,
									"EDID");
	if (error)
		fprintf(stderr, "error xcb_intern_atom\n");
	
	xcb_randr_get_output_property_reply_t *out_prop;
	out_prop = QRY(xcb_randr_get_output_property, xrlist->conn, error,
				output,
				atom->atom,
				XCB_GET_PROPERTY_TYPE_ANY,
				0,
				256,
				0,
				0
	);
	if (error)
		fprintf(stderr, "error xcb_randr_get_output_property\n");
	
	*edid = xcb_randr_get_output_property_data(out_prop);
	*length = xcb_randr_get_output_property_data_length(out_prop);
	if ((*edid == NULL) || (*length < 1)) {
		fprintf(stderr, "error xcb_randr_get_output_property_data\n");
	}
	
	// 	FILE *f = fopen("test.edid", "w");
	// 	fwrite(*edid, *length, 1, f);
	// 	fclose(f);
	
	unsigned char sum, i;
	for (i=0; i<128; i++)
		sum += (*edid)[i];
	
	if (sum) {
		fprintf(stderr, "wrong checksum\n");
		return 1;
	}
	
	unsigned char magic[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
	if (memcmp(magic, *edid, 8)) {
		fprintf(stderr, "wrong edid header\n");
		return 1;
	}
	
	return 0;
}

struct crtx_dict * crtc_change2dict(struct crtx_xcb_randr_listener *xrlist, xcb_randr_crtc_change_t *cevent) {
	return crtx_create_dict("suuuuu",
							"mode", (cevent->mode == XCB_NONE)?"off":"on", 0, CRTX_DIF_DONT_FREE_DATA,
							"crtc", cevent->crtc, sizeof(cevent->crtc), 0,
							"x", (uint32_t) cevent->x, sizeof(uint32_t), 0,
							"y", (uint32_t) cevent->y, sizeof(uint32_t), 0,
							"width", (uint32_t) cevent->width, sizeof(uint32_t), 0,
							"height", (uint32_t) cevent->height, sizeof(uint32_t), 0
						);
}

struct crtx_dict * output_change2dict(struct crtx_xcb_randr_listener *xrlist, xcb_randr_output_change_t *oevent) {
	xcb_randr_get_output_info_reply_t *output_info;
	xcb_generic_error_t* error;
	int i;
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *edi;
	
	
	dict = crtx_init_dict(0, 0, 0);
	
	if (oevent->crtc == XCB_NONE) {
		printf("output 0x%08x disconnected\n", oevent->output);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "mode", "off", 3, CRTX_DIF_DONT_FREE_DATA);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 'u', "crtc", oevent->crtc, sizeof(oevent->crtc), 0);
		return 0;
	} else {
		output_info = QRY(xcb_randr_get_output_info, xrlist->conn, error, oevent->output, XCB_CURRENT_TIME);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "mode", "on", 3, CRTX_DIF_DONT_FREE_DATA);
		
// 		char *name = strndup((char *) xcb_randr_get_output_info_name(output_info), 
// 								xcb_randr_get_output_info_name_length(output_info));
// 		printf("output %d\n", output_info->num_crtcs);
		
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "output_name", (char *) xcb_randr_get_output_info_name(output_info),
							xcb_randr_get_output_info_name_length(output_info), CRTX_DIF_DONT_FREE_DATA);
		
		
		xcb_randr_get_crtc_info_reply_t *crtc_info;
		xcb_randr_crtc_t *crtc = xcb_randr_get_output_info_crtcs(output_info);
		for (i=0; i < output_info->num_crtcs; i++) {
			crtc_info = QRY(xcb_randr_get_crtc_info, xrlist->conn, error, crtc[i], XCB_CURRENT_TIME);
// 			printf("crtc %d: 0x%08x %d,%d %d,%d\n", i, crtc[i], crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height);
			
			if (crtc[i] == oevent->crtc) {
				di = crtx_alloc_item(dict);
				di->type = 'D';
// 				di->key = (char*) malloc(5+floor(log10(abs(crtc[i])))+1);
// 				sprintf(di->key, "crtc_%u", crtc[i]);
// 				di->flags |= CRTX_DIF_ALLOCATED_KEY;
				di->key = "crtc";
				
				di->dict = crtx_create_dict("uuuuu",
										"crtc", oevent->crtc, sizeof(oevent->crtc), 0,
										"x", (uint32_t) crtc_info->x, sizeof(uint32_t), 0,
										"y", (uint32_t) crtc_info->y, sizeof(uint32_t), 0,
										"width_px", (uint32_t) crtc_info->width, sizeof(uint32_t), 0,
										"height_px", (uint32_t) crtc_info->height, sizeof(uint32_t), 0
						);
			}
		}
		
		
		unsigned char *edid;
		int edid_size;
		struct edid_monitor_descriptor *mon_desc;
		
		i = get_edid(xrlist, oevent->output, &edid, &edid_size);
		if (i)
			return dict;
		
		di = crtx_alloc_item(dict);
		di->type = 'D';
		
		di->key = "edid";
// 		di->flags |= CRTX_DIF_ALLOCATED_KEY;
		di->dict = crtx_init_dict(0, 0, 0);
		
		// 		printf("\tVendorName \"%c%c%c\"\n", (edid[8] >> 2 & 0x1f) + 'A' - 1, (((edid[8] & 0x3) << 3) | ((edid[9] & 0xe0) >> 5)) + 'A' - 1, (edid[9] & 0x1f) + 'A' - 1 );
		char *name = (char*) malloc(4);
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
				char *name;
				int j;
				
				if (mon_desc->desc_type == 0xFC) {
					for (j=0;j<13;j++)
						if (mon_desc->data[j] == 0x0a)
							break;
					
					name = (char*) malloc(j+1);
					memcpy(name, mon_desc->data, j);
					
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
		
	}
	
// 			di = crtx_alloc_item(dict);
// 			value = udev_device_get_devnode(dev);
// 			crtx_fill_data_item(di, 's', "node", value, strlen(value), CRTX_DIF_DONT_FREE_DATA);
	
// 	printf("output %s (0x%08x) on CRTC 0x%08x changed\n", name, oevent->output, oevent->crtc);
	
	return dict;
}

void *xcb_randr_tmain(void *data) {
	struct crtx_xcb_randr_listener *xrlist;
	xcb_generic_event_t *ev;
	struct crtx_event *crtx_event;
	
	xrlist = (struct crtx_xcb_randr_listener*) data;
	
	while ((ev = xcb_wait_for_event(xrlist->conn)) != NULL) {
		const xcb_query_extension_reply_t *randr;
		uint8_t type = ev->response_type & 0x7f;
		int sent = !!(ev->response_type & 0x80);
		
		randr = xcb_get_extension_data(xrlist->conn, &xcb_randr_id);
		if (type == randr->first_event + XCB_RANDR_NOTIFY) {
			xcb_randr_notify_event_t *event = (xcb_randr_notify_event_t *) ev;
			switch (event->subCode) {
				case XCB_RANDR_NOTIFY_CRTC_CHANGE:
					{
// 						xcb_randr_crtc_change_t *cevent = &event->u.cc;
						
						
						crtx_event = crtx_create_event("xcb_randr/crtc_change", &event->u.cc, 0);
						crtx_event->data.flags = CRTX_DIF_DONT_FREE_DATA;
// 						crtx_event->origin = (struct crtx_listener_base *) xrlist;
						
						add_event(xrlist->parent.graph, crtx_event);
						
// 						if (cevent->mode == XCB_NONE) {
// 							printf("CRTC 0x%08x disabled\n", cevent->crtc);
// 						} else {
// 							printf("CRTC 0x%08x now has geometry (%d, %d), (%d, %d)\n",
// 								cevent->crtc, cevent->x, cevent->y, cevent->width, cevent->height);
// 						}
					}
					break;
				case XCB_RANDR_NOTIFY_OUTPUT_CHANGE:
// 					output_change(xrlist, &event->u.oc);
					
					crtx_event = crtx_create_event("xcb_randr/output_change", &event->u.oc, 0);
					crtx_event->data.flags = CRTX_DIF_DONT_FREE_DATA;
// 					crtx_event->origin = (struct crtx_listener_base *) xrlist;
					
					add_event(xrlist->parent.graph, crtx_event);
					
					break;
				default:
					fprintf(stderr, "Unhandled RandR Notify event of subcode %d\n", event->subCode);
			}
		} else {
			fprintf(stderr, "Unhandled event of type %d (%d) %s\n", type, randr->first_event, sent ? " (generated)" : "");
		}
		
		free(ev);
	}
	
	return 0;
}

void crtx_free_xcb_randr_listener(struct crtx_listener_base *data, void *userdata) {
	struct crtx_xcb_randr_listener *xrlist;
	
	xrlist = (struct crtx_xcb_randr_listener*) data;
	
	xcb_disconnect(xrlist->conn);
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
		fprintf(stderr, "xcb_connect failed: %d\n", ret);
		return 0;
	}
	
	xrlist->screen = xcb_setup_roots_iterator(xcb_get_setup(xrlist->conn)).data;
	
	query = xcb_get_extension_data(xrlist->conn, &xcb_randr_id);
	if (!query || !query->present) {
		fprintf(stderr, "error, randr not present\n");
		return 0;
	}
	
// 	version = xcb_randr_query_version_reply(xrlist->conn,
// 				xcb_randr_query_version(xrlist->conn, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION),
// 				NULL);
	version = QRY(xcb_randr_query_version, xrlist->conn, error, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
	if (!version || version->major_version < 1 || (version->major_version == 1 && version->minor_version < 2)) {
		fprintf(stderr, "error, randr too old\n");
		return 0;
	}
	
	xcb_randr_select_input(xrlist->conn, xrlist->screen->root, XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);
	xcb_flush(xrlist->conn);
	
	xrlist->parent.free = &crtx_free_xcb_randr_listener;
// 	new_eventgraph(&xrlist->parent.graph, "xcb_randr", xcb_randr_msg_etype);
	
	xrlist->parent.thread = get_thread(xcb_randr_tmain, xrlist, 0);
	
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
// 	struct crtx_xcb_randr_listener *xrlist;
	
	
// 	xrlist = (struct crtx_xcb_randr_listener *) event->origin;
	
	if (!strcmp(event->type, "xcb_randr/crtc_change")) {
		dict = crtc_change2dict(&xrlist, event->data.pointer);
		
// 		xcb_randr_crtc_change_t *cevent = event->data.pointer;
		
// 		if (cevent->mode == XCB_NONE) {
// 			printf("CRTC 0x%08x disabled\n", cevent->crtc);
// 		} else {
// 			printf("CRTC 0x%08x now has geometry (%d, %d), (%d, %d)\n",
// 				cevent->crtc, cevent->x, cevent->y, cevent->width, cevent->height);
// 		}
		
// 		dict = crtx_create_dict("suuuuu",
// 					"mode", (cevent->mode == XCB_NONE)?"off":"on", 0, CRTX_DIF_DONT_FREE_DATA,
// 					"crtc", cevent->crtc, sizeof(cevent->crtc), 0,
// 					"x", (uint32_t) cevent->x, sizeof(uint32_t), 0,
// 					"y", (uint32_t) cevent->y, sizeof(uint32_t), 0,
// 					"width", (uint32_t) cevent->width, sizeof(uint32_t), 0,
// 					"height", (uint32_t) cevent->height, sizeof(uint32_t), 0
// 			   );
		
	} else
	if (!strcmp(event->type, "xcb_randr/output_change")) {
		dict = output_change2dict(&xrlist, event->data.pointer);
	}
	
	crtx_print_dict(dict);
	
	crtx_free_dict(dict);
	
	return 0;
}

int xcb_randr_main(int argc, char **argv) {
	struct crtx_listener_base *lbase;
	char ret;
	
// 	crtx_root->no_threads = 1;
	
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
	
// 	crtx_free_listener(lbase);
	
	return 0;  
}

CRTX_TEST_MAIN(xcb_randr_main);

#endif
