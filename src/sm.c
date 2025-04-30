/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include "core.h"
#include "sm.h"

static int change_sms(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_sm_state *sms;
	
	sms = (struct crtx_sm_state*) userdata;
	
	sms->sm->n_schedules -= 1;
	
	if (sms->sm->flags & CRTX_SMF_RELEASE) {
		if (sms->sm->release_cb && sms->sm->n_schedules == 0)
			sms->sm->release_cb(sms->sm);
		return 0;
	}
	
	if (sms == sms->sm->cur_state) {
		if (sms->trigger)
			sms->trigger(sms, CRTX_SMSE_TRIGGER);
		
		return 0;
	}
	
	if (sms->sm->cur_state && sms->sm->cur_state->leave)
		sms->sm->cur_state->leave(sms, CRTX_SMSE_LEAVE);
	
	sms->sm->cur_state = sms;
	
	if (sms->sm->cur_state->enter)
		sms->sm->cur_state->enter(sms, CRTX_SMSE_ENTER);
	
	return 0;
}

int crtx_change_state(struct crtx_sm *sm, struct crtx_sm_state *sms) {
	sm->n_schedules += 1;
	if (!sms)
		sms = sm->cur_state;
	crtx_evloop_schedule_callback(change_sms, sms, 0);
	
	return 0;
}

int crtx_release_sm(struct crtx_sm *sm) {
	sm->flags |= CRTX_SMF_RELEASE;
	
	if (sm->n_schedules == 0)
		crtx_change_state(sm, 0);
	
	return 0;
}
