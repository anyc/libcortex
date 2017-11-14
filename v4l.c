
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
#include "v4l.h"

// static char v4l_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_event_loop_payload *payload;
// 	struct can_frame *frame;
// 	struct crtx_event *nevent;
// 	struct crtx_can_listener *clist;
// 	int r;
// 	
// 	payload = (struct crtx_event_loop_payload*) event->data.pointer;
// 	
// 	clist = (struct crtx_can_listener *) payload->data;
// 	
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
// 	
// 	return 0;
// }

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_v4l_listener *lstnr;
	
	lstnr = (struct crtx_v4l_listener *) listener;
	
	printf("format settings: %u %c%c%c%c %u %u\n",
		lstnr->format.type,
		((char *)&lstnr->format.fmt.pix.pixelformat)[0],
		((char *)&lstnr->format.fmt.pix.pixelformat)[1],
		((char *)&lstnr->format.fmt.pix.pixelformat)[2],
		((char *)&lstnr->format.fmt.pix.pixelformat)[3],
		lstnr->format.fmt.pix.width, lstnr->format.fmt.pix.height);
	
	if(ioctl(lstnr->fd, VIDIOC_S_FMT, &lstnr->format) < 0){
		printf("VIDIOC_S_FMT failed\n");
		return -1;
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
// 	lstnr->parent.el_payload.event_handler = &v4l_fd_event_handler;
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
