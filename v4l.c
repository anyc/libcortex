
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

#include "core.h"
#include "intern.h"
#include "v4l.h"

char crtx_v4l2_event_ctrl2dict(struct v4l2_event_ctrl *ptr, struct crtx_dict **dict_ptr) {
        struct crtx_dict *dict;
        struct crtx_dict_item *di;

        if (! *dict_ptr)
                *dict_ptr = crtx_init_dict(0, 0, 0);
        dict = *dict_ptr;

        crtx_dict_new_item(dict, 'u', "changes", ptr->changes, sizeof(ptr->changes), 0);

        crtx_dict_new_item(dict, 'u', "type", ptr->type, sizeof(ptr->type), 0);

        /* TODO union: 
        if (ptr) {
                di->dict = crtx_init_dict(0, 0, 0);
                struct crtx_dict *dict = di->dict;
                struct crtx_dict_item *di2;

                crtx_dict_new_item(dict, 'i', "value", ptr->value, sizeof(ptr->value), 0);

                crtx_dict_new_item(dict, 'I', "value64", ptr->value64, sizeof(ptr->value64), 0);
        }
        */

        crtx_dict_new_item(dict, 'u', "flags", ptr->flags, sizeof(ptr->flags), 0);

        crtx_dict_new_item(dict, 'i', "minimum", ptr->minimum, sizeof(ptr->minimum), 0);

        crtx_dict_new_item(dict, 'i', "maximum", ptr->maximum, sizeof(ptr->maximum), 0);

        crtx_dict_new_item(dict, 'i', "step", ptr->step, sizeof(ptr->step), 0);

        crtx_dict_new_item(dict, 'i', "default_value", ptr->default_value, sizeof(ptr->default_value), 0);

        return 0;
}

char crtx_v4l2_event2dict(struct v4l2_event *ptr, struct crtx_dict **dict_ptr)
{
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *di2;
	
	if (! *dict_ptr)
		*dict_ptr = crtx_init_dict(0, 0, 0);
	dict = *dict_ptr;
	
	crtx_dict_new_item(dict, 'u', "etype_id", ptr->type, sizeof(ptr->type), 0);
	
	switch(ptr->type) {
		case V4L2_EVENT_ALL:
			crtx_dict_new_item(dict, 'u', "etype", "all", 0, CRTX_DIF_DONT_FREE_DATA);
			break;
		case V4L2_EVENT_VSYNC:
			crtx_dict_new_item(dict, 'u', "etype", "vsync", 0, CRTX_DIF_DONT_FREE_DATA);
			
// 			di2 = crtx_alloc_item(dict);
// 			crtx_fill_data_item(di2, 'D', "vsync", 0, 0, 0);
			// crtx_v4l2_event_vsync2dict(&ptr->vsync, di2->dict);
			break;
		case V4L2_EVENT_EOS:
			crtx_dict_new_item(dict, 'u', "etype", "eos", 0, CRTX_DIF_DONT_FREE_DATA);
			break;
		case V4L2_EVENT_CTRL:
			crtx_dict_new_item(dict, 'u', "etype", "ctrl", 0, CRTX_DIF_DONT_FREE_DATA);
			
			di2 = crtx_alloc_item(dict);
			crtx_fill_data_item(di2, 'D', "ctrl", 0, 0, 0);
			crtx_v4l2_event_ctrl2dict(&ptr->u.ctrl, &di2->dict);
			break;
		case V4L2_EVENT_FRAME_SYNC:
			crtx_dict_new_item(dict, 'u', "etype", "frame sync", 0, CRTX_DIF_DONT_FREE_DATA);
			
// 			di2 = crtx_alloc_item(dict);
// 			crtx_fill_data_item(di2, 'D', "frame_sync", 0, 0, 0);
			// crtx_v4l2_event_frame_sync2dict(&ptr->frame_sync, di2->dict);
			break;
		case V4L2_EVENT_SOURCE_CHANGE:
			crtx_dict_new_item(dict, 'u', "etype", "source change", 0, CRTX_DIF_DONT_FREE_DATA);
			
// 			di2 = crtx_alloc_item(dict);
// 			crtx_fill_data_item(di2, 'D', "src_change", 0, 0, 0);
			// crtx_v4l2_event_src_change2dict(&ptr->src_change, di2->dict);
			break;
		case V4L2_EVENT_MOTION_DET:
			crtx_dict_new_item(dict, 'u', "etype", "motion det", 0, CRTX_DIF_DONT_FREE_DATA);
			
// 			di2 = crtx_alloc_item(dict);
// 			crtx_fill_data_item(di2, 'D', "motion_det", 0, 0, 0);
			// crtx_v4l2_event_motion_det2dict(&ptr->motion_det, di2->dict);
			break;
		case V4L2_EVENT_PRIVATE_START:
			crtx_dict_new_item(dict, 'u', "etype", "private start", 0, CRTX_DIF_DONT_FREE_DATA);
			
			crtx_dict_new_item(dict, 'p', "data", ptr->u.data, sizeof(ptr->u.data[0])*64, CRTX_DIF_DONT_FREE_DATA);
			break;
	}
	
// 	di = crtx_alloc_item(dict);
// 	crtx_fill_data_item(di, 'D', "u", 0, 0, 0);
	// crtx_union v4l2_event::(anonymous at /usr/include/linux/videodev2.h:2089:2)2dict(&ptr->u, di->dict);
	
	crtx_dict_new_item(dict, 'u', "pending", ptr->pending, sizeof(ptr->pending), 0);
	
	crtx_dict_new_item(dict, 'u', "sequence", ptr->sequence, sizeof(ptr->sequence), 0);
	
// 	di = crtx_alloc_item(dict);
// 	crtx_fill_data_item(di, 'D', "timestamp", 0, 0, 0);
	// crtx_timespec2dict(&ptr->timestamp, di->dict);
	
	crtx_dict_new_item(dict, 'u', "id", ptr->id, sizeof(ptr->id), 0);
	
// 	di = crtx_alloc_item(dict);
// 	crtx_fill_data_item(di, 'D', "reserved", 0, 0, 0);
// 	
// 	// TypeKind.UINT
// 	di->dict = crtx_init_dict("uuuuuuuu", 8, 0);
// 	{
// 		crtx_fill_data_item(&di->dict->items[0], 'u', 0, ptr->reserved[0], sizeof(ptr->reserved[0]), 0);
// 		crtx_fill_data_item(&di->dict->items[1], 'u', 0, ptr->reserved[1], sizeof(ptr->reserved[1]), 0);
// 		crtx_fill_data_item(&di->dict->items[2], 'u', 0, ptr->reserved[2], sizeof(ptr->reserved[2]), 0);
// 		crtx_fill_data_item(&di->dict->items[3], 'u', 0, ptr->reserved[3], sizeof(ptr->reserved[3]), 0);
// 		crtx_fill_data_item(&di->dict->items[4], 'u', 0, ptr->reserved[4], sizeof(ptr->reserved[4]), 0);
// 		crtx_fill_data_item(&di->dict->items[5], 'u', 0, ptr->reserved[5], sizeof(ptr->reserved[5]), 0);
// 		crtx_fill_data_item(&di->dict->items[6], 'u', 0, ptr->reserved[6], sizeof(ptr->reserved[6]), 0);
// 		crtx_fill_data_item(&di->dict->items[7], 'u', 0, ptr->reserved[7], sizeof(ptr->reserved[7]), 0);
// 	}
	
	return 0;
}

static char v4l_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_event *nevent;
	struct crtx_v4l_listener *clist;
	struct v4l2_event *v4l_event;
	struct crtx_dict *dict;
	int r;
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	clist = (struct crtx_v4l_listener *) payload->data;
	
	r = ioctl(clist->fd, VIDIOC_DQEVENT, &v4l_event);
	if (r < 0) {
		ERROR("VIDIOC_DQEVENT failed\n");
		return -1;
	}
	
	dict = 0;
	crtx_v4l2_event2dict(v4l_event, &dict);
	
	crtx_print_dict(dict);
	
// 	frame = (struct can_frame *) malloc(sizeof(struct can_frame));
// 	r = read(clist->sockfd, frame, sizeof(struct can_frame));
// 	if (r != sizeof(struct can_frame)) {
// 		fprintf(stderr, "wrong can frame size: %d\n", r);
// 		return 1;
// 	}
// 	
// 	// 	printf("0x%04x ", frame->can_id);
// 	
// 	nevent = crtx_create_event("can"); //, frame, sizeof(frame));
// 	crtx_event_set_raw_data(nevent, 'p', frame, sizeof(frame), 0);
// 	// 	nevent->cb_before_release = &can_event_before_release_cb;
// 	
// 	// 	memcpy(&nevent->data.uint64, &frame.data, sizeof(uint64_t));
// 	
// 	crtx_add_event(clist->parent.graph, nevent);
// 	
// 	// 	exit(1);
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_v4l_listener *lstnr;
	unsigned int i;
	int r;
	
	lstnr = (struct crtx_v4l_listener *) listener;
	
	printf("format settings: %u %c%c%c%c %u %u\n",
		lstnr->format.type,
		((char *)&lstnr->format.fmt.pix.pixelformat)[0],
		((char *)&lstnr->format.fmt.pix.pixelformat)[1],
		((char *)&lstnr->format.fmt.pix.pixelformat)[2],
		((char *)&lstnr->format.fmt.pix.pixelformat)[3],
		lstnr->format.fmt.pix.width, lstnr->format.fmt.pix.height);
	
	if(ioctl(lstnr->fd, VIDIOC_S_FMT, &lstnr->format) < 0){
		ERROR("VIDIOC_S_FMT failed\n");
		return -1;
	}
	
	for (i=0; i < lstnr->subscription_length; i++) {
		r = ioctl(lstnr->fd, VIDIOC_SUBSCRIBE_EVENT, &lstnr->subscriptions);
		if (r < 0) {
			ERROR("VIDIOC_SUBSCRIBE_EVENT failed\n");
			return -1;
		}
	}
	
	return 1;
}

struct crtx_listener_base *crtx_new_v4l_listener(void *options) {
	struct crtx_v4l_listener *lstnr;
	int r;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_frmsizeenum frmsize;
	
	
	lstnr = (struct crtx_v4l_listener *) options;
	
	lstnr->fd = open(lstnr->device_path, O_RDWR);
	if (lstnr->fd < 0) {
		ERROR("opening v4l device failed: %s\n", strerror(errno));
		return 0;
	}
	
	
	lstnr->formats = crtx_init_dict(0, 0, 0);
	
	memset(&fmtdesc,0,sizeof(fmtdesc));
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(lstnr->fd,VIDIOC_ENUM_FMT,&fmtdesc) == 0) {
		size_t slen;
		struct crtx_dict *desc_dict, *size_dict;
		struct crtx_dict_item *di;
		char fourcc[5];
		
// 		printf("%s\n", fmtdesc.description);
		
// 		di = crtx_alloc_item(dict);
// 		crtx_fill_data_item(di, 's', "description", fmtdesc.description, 0, CRTX_DIF_CREATE_DATA_COPY);
		
		di = crtx_alloc_item(lstnr->formats);
		di->type = 'D';
		
		slen = strlen((char*)fmtdesc.description);
		di->key = malloc(slen+1);
		snprintf(di->key, slen+1, "%s", fmtdesc.description);
		di->flags |= CRTX_DIF_ALLOCATED_KEY;
		di->dict = crtx_init_dict(0, 0, 0);
		desc_dict = di->dict;
		
		crtx_dict_new_item(desc_dict, 'u', "type_id", fmtdesc.type, 0, CRTX_DIF_DONT_FREE_DATA); \
		switch (fmtdesc.type) {
		#define SWITCHTYPE(dict, type, id, s) \
			case id: \
				crtx_dict_new_item(dict, 's', type, s, 0, CRTX_DIF_DONT_FREE_DATA); \
				break;
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_CAPTURE, "video capture");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_OUTPUT, "video output");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_OVERLAY, "video overlay");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VBI_CAPTURE, "vbi capture");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VBI_OUTPUT, "vbi output");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_SLICED_VBI_CAPTURE, "sliced vbi capture");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_SLICED_VBI_OUTPUT, "sliced vbi output");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY, "video output overlay");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, "video capture mplane");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, "video output mplane");
				SWITCHTYPE(desc_dict, "type", V4L2_BUF_TYPE_SDR_CAPTURE, "sdr capture");
		}
		
		switch (fmtdesc.flags) {
			SWITCHTYPE(desc_dict, "flags", V4L2_FMT_FLAG_COMPRESSED, "compressed");
			SWITCHTYPE(desc_dict, "flags", V4L2_FMT_FLAG_EMULATED, "emulated");
		}
		
		memcpy(fourcc, &fmtdesc.pixelformat, 4);
		fourcc[4] = 0;
		crtx_dict_new_item(desc_dict, 's', "fourcc", fourcc, 0, CRTX_DIF_CREATE_DATA_COPY);
		
		
		
		frmsize.pixel_format = fmtdesc.pixelformat;
		frmsize.index = 0;
		while (ioctl(lstnr->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
			di = crtx_alloc_item(desc_dict);
			di->type = 'D';
			di->dict = crtx_init_dict(0, 0, 0);
			size_dict = di->dict;
			
			switch (frmsize.type) {
				SWITCHTYPE(size_dict, "type", V4L2_FRMSIZE_TYPE_DISCRETE, "discrete");
				SWITCHTYPE(size_dict, "type", V4L2_FRMSIZE_TYPE_CONTINUOUS, "continuous");
				SWITCHTYPE(size_dict, "type", V4L2_FRMSIZE_TYPE_STEPWISE, "stepwise");
			}
			
			if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				crtx_dict_new_item(size_dict, 'u', "width", frmsize.discrete.width, 0, CRTX_DIF_CREATE_DATA_COPY);
				crtx_dict_new_item(size_dict, 'u', "height", frmsize.discrete.height, 0, CRTX_DIF_CREATE_DATA_COPY);
// 				printf("%dx%d\n", frmsize.discrete.width, frmsize.discrete.height);
				
			} else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
				/* TODO:
				__u32 min_width
				__u32	max_width
				__u32	step_width
				__u32	min_height
				__u32	max_height
				__u32	step_height */
				
				crtx_dict_new_item(size_dict, 'u', "max_width", frmsize.stepwise.max_width, 0, CRTX_DIF_CREATE_DATA_COPY);
				crtx_dict_new_item(size_dict, 'u', "max_height", frmsize.stepwise.max_height, 0, CRTX_DIF_CREATE_DATA_COPY);
// 				printf("%dx%d\n", frmsize.stepwise.max_width, frmsize.stepwise.max_height);
			}
			frmsize.index++;
		}
		
		fmtdesc.index++;
	}
	
	lstnr->parent.el_payload.fd = lstnr->fd;
	lstnr->parent.el_payload.data = lstnr;
	lstnr->parent.el_payload.event_handler = &v4l_fd_event_handler;
	lstnr->parent.el_payload.event_handler_name = "v4l fd handler";
// 	lstnr->parent.el_payload.error_cb = &on_error_cb;
// 	lstnr->parent.el_payload.error_cb_data = lstnr;
	
// 	lstnr->parent.shutdown = &crtx_shutdown_can_listener;
	
	lstnr->parent.start_listener = &start_listener;
	
	return &lstnr->parent;
}

void crtx_v4l_init() {
}

void crtx_v4l_finish() {
}

#ifdef CRTX_TEST
static char can_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct can_frame *frame;
	
	frame = (struct can_frame*) event->data.pointer;
	
// 	printf("CAN: %d\n", frame->can_id);
	
	return 0;
}

int v4l_main(int argc, char **argv) {
	struct crtx_v4l_listener clist;
	struct crtx_listener_base *lbase;
	int ret;
	struct crtx_dict_item *di;
	struct crtx_dict *pix_format, *fsize;
	char *fourcc;
	
	
	memset(&clist, 0, sizeof(struct crtx_v4l_listener));
// 	clist.protocol = CAN_RAW;
// 	clist.bitrate = 250000;
	clist.device_path = "/dev/video1";
	
	lbase = create_listener("v4l", &clist);
	if (!lbase) {
		ERROR("create_listener(v4l) failed\n");
		return 0;
	}
	
	crtx_print_dict(clist.formats);
	
	if (clist.formats->signature_length == 0) {
		ERROR("no video formats found\n");
		return 0;
	}
	
	di = crtx_get_item_by_idx(clist.formats, 0);
	pix_format = di->dict;
	
	crtx_get_value(pix_format, "type_id", 'u', &clist.format.type, sizeof(clist.format.type));
	
	fourcc = crtx_get_string(pix_format, "fourcc");
// 	crtx_get_value(pix_format, "fourcc", 'u', &clist.format.fmt.pix.pixelformat, sizeof(clist.format.fmt.pix.pixelformat));
	memcpy(&clist.format.fmt.pix.pixelformat, fourcc, 4);
	
	di = crtx_get_item_by_idx(pix_format, 3);
	fsize = di->dict;
	crtx_get_value(fsize, "width", 'u', &clist.format.fmt.pix.width, sizeof(clist.format.fmt.pix.width));
	crtx_get_value(fsize, "height", 'u', &clist.format.fmt.pix.height, sizeof(clist.format.fmt.pix.height));
	
	clist.subscription_length = 1;
	clist.subscriptions = (struct v4l2_event_subscription*) calloc(1, sizeof(struct v4l2_event_subscription)*clist.subscription_length);
	clist.subscriptions[0].type = V4L2_EVENT_FRAME_SYNC;
	clist.subscriptions[0].flags = V4L2_EVENT_SUB_FL_SEND_INITIAL;
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting v4l listener failed\n");
		return 0;
	}
	
	
// 	crtx_create_task(lbase->graph, 0, "can_test", &can_event_handler, 0);
	
// 	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(v4l_main);
#endif
