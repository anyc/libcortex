#ifndef _CRTX_READLINE_H
#define _CRTX_READLINE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// struct crtx_readline_listener {
// 	struct crtx_listener_base base;
// };

// void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action);
// struct crtx_listener_base *crtx_setup_readline_listener(void *options);

void crtx_readline_init();
void crtx_readline_finish();

#ifdef __cplusplus
}
#endif

#endif
