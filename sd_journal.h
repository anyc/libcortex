#ifndef _CRTX_SD_JOURNAL_H
#define _CRTX_SD_JOURNAL_H

#ifdef __cplusplus
extern "C" {
	#endif
	
	/*
	 * Mario Kicherer (dev@kicherer.org) 2022
	 *
	 */
	
	#include <systemd/sd-journal.h>
	#include "core.h"
	
	#define CRTX_SD_JOURNAL_EVT_NEW_ENTRY "sd_journal.new_entry"
	
	#define CRTX_SD_JOURNAL_EVT_NEW_ENTRY_ID (1<<0)
	
	struct crtx_sd_journal_print_event_userdata {
		FILE *f;
		char *buf;
		size_t buf_size;
		
		void (*cb)(struct crtx_sd_journal_print_event_userdata *udata);
		void *cb_data;
	};
	
	struct crtx_sd_journal_listener {
		struct crtx_listener_base base;
		
		sd_journal *sd_journal;
		int flags;
		int seek; // positive: skip first x entries, negative: read last x entries
		char try_read;
		char same_boot_filter;
		
		int (*event_cb)(struct crtx_sd_journal_listener *jlstnr, void *userdata);
		void *event_cb_data;
	};
	
	struct crtx_listener_base *crtx_setup_sd_journal_listener(void *options);
	void crtx_sd_journal_clear_listener(struct crtx_sd_journal_listener *lstnr);
	CRTX_DECLARE_CALLOC_FUNCTION(sd_journal)
	
	void crtx_sd_journal_init();
	void crtx_sd_journal_finish();
	
	int crtx_sd_journal_print_event(struct crtx_sd_journal_listener *jlstnr, void *userdata);
	int crtx_sd_journal_gen_event_dict(struct crtx_sd_journal_listener *jlstnr, void *userdata);
	
	#ifdef __cplusplus
}
#endif

#endif
