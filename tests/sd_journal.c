
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "sd_journal.h"

char * topic = 0;

static int sd_journal_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_print_event(event, 0);
	printf("\n");
	
	return 0;
}

int main(int argc, char **argv) {
	struct crtx_sd_journal_listener jlist;
	int rv;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_sd_journal_clear_listener(&jlist);
	
	jlist.flags = SD_JOURNAL_SYSTEM | SD_JOURNAL_CURRENT_USER | SD_JOURNAL_LOCAL_ONLY;
	jlist.seek = -10;
	
	// 	jlist.event_cb = &crtx_sd_journal_print_event;
	jlist.event_cb = &crtx_sd_journal_gen_event_dict;
	
	rv = crtx_setup_listener("sd_journal", &jlist);
	if (rv) {
		CRTX_ERROR("create_listener(sd_journal) failed\n");
		return 0;
	}
	
	crtx_create_task(jlist.base.graph, 0, "sd_journal_event", &sd_journal_event_handler, &jlist);
	
	rv = crtx_start_listener(&jlist.base);
	if (rv) {
		CRTX_ERROR("starting sd_journal listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
