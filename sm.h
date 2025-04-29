#ifndef CRTX_SM_H
#define CRTX_SM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

enum crtx_sms_event {
	CRTX_SMSE_ENTER,
	CRTX_SMSE_TRIGGER,
	CRTX_SMSE_LEAVE,
};

struct crtx_sm;
struct crtx_sm_state;

typedef int (*crtx_sm_callback)(struct crtx_sm_state *sms, enum crtx_sms_event event);
typedef int (*crtx_sm_release_callback)(struct crtx_sm *sm);

struct crtx_sm_state {
	char *name;
	
	crtx_sm_callback enter;
	crtx_sm_callback trigger;
	crtx_sm_callback leave;
	
	struct crtx_sm *sm;
};

#define CRTX_SMF_RELEASE	(1<<0)

struct crtx_sm {
	struct crtx_sm_state *cur_state;
	
	_Atomic unsigned int n_schedules;
	crtx_sm_release_callback release_cb;
	int flags;
	
	void *userdata;
};

int crtx_change_state(struct crtx_sm *sm, struct crtx_sm_state *sms);
int crtx_release_sm(struct crtx_sm *sm);

#ifdef __cplusplus
}
#endif

#endif
