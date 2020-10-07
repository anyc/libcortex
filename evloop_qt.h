#ifndef CRTX_EVLOOP_QT_H
#define CRTX_EVLOOP_QT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

void crtx_evloop_qt_init(struct crtx_event_loop *evloop);
void crtx_evloop_qt_finish(struct crtx_event_loop *evloop);

#ifdef __cplusplus
}
#endif

#endif
