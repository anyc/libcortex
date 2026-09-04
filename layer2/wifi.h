#ifndef _CRTX_WIFI_H
#define _CRTX_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2026
 *
 */

#include "crtx/netlink_ge.h"
#include <stdint.h>

#define CRTX_WIFI_NEW_WIPHY                     (UINT64_C(1) << 0)
#define CRTX_WIFI_DEL_WIPHY                     (UINT64_C(1) << 1)
#define CRTX_WIFI_NEW_INTERFACE                 (UINT64_C(1) << 2)
#define CRTX_WIFI_DEL_INTERFACE                 (UINT64_C(1) << 3)
#define CRTX_WIFI_NEW_STATION                   (UINT64_C(1) << 4)
#define CRTX_WIFI_DEL_STATION                   (UINT64_C(1) << 5)
#define CRTX_WIFI_CONNECT                       (UINT64_C(1) << 6)
#define CRTX_WIFI_DISCONNECT                    (UINT64_C(1) << 7)
#define CRTX_WIFI_ROAM                          (UINT64_C(1) << 8)
#define CRTX_WIFI_NEW_SCAN_RESULTS              (UINT64_C(1) << 9)
#define CRTX_WIFI_SCAN_ABORTED                  (UINT64_C(1) << 10)
#define CRTX_WIFI_REG_CHANGE                    (UINT64_C(1) << 11)
#define CRTX_WIFI_AUTHENTICATE                  (UINT64_C(1) << 12)
#define CRTX_WIFI_ASSOCIATE                     (UINT64_C(1) << 13)
#define CRTX_WIFI_DEAUTHENTICATE                (UINT64_C(1) << 14)
#define CRTX_WIFI_DISASSOCIATE                  (UINT64_C(1) << 15)
#define CRTX_WIFI_MICHAEL_MIC_FAILURE           (UINT64_C(1) << 16)
#define CRTX_WIFI_REG_BEACON_HINT               (UINT64_C(1) << 17)
#define CRTX_WIFI_NEW_SURVEY_RESULTS            (UINT64_C(1) << 18)
#define CRTX_WIFI_FRAME                         (UINT64_C(1) << 19)
#define CRTX_WIFI_FRAME_TX_STATUS               (UINT64_C(1) << 20)
#define CRTX_WIFI_NOTIFY_CQM                    (UINT64_C(1) << 21)
#define CRTX_WIFI_NEW_PEER_CANDIDATE            (UINT64_C(1) << 22)
#define CRTX_WIFI_SCHED_SCAN_RESULTS            (UINT64_C(1) << 23)
#define CRTX_WIFI_SCHED_SCAN_STOPPED            (UINT64_C(1) << 24)
#define CRTX_WIFI_UNEXPECTED_FRAME              (UINT64_C(1) << 25)
#define CRTX_WIFI_CH_SWITCH_NOTIFY              (UINT64_C(1) << 26)
#define CRTX_WIFI_CONN_FAILED                   (UINT64_C(1) << 27)
#define CRTX_WIFI_RADAR_DETECT                  (UINT64_C(1) << 28)
#define CRTX_WIFI_FT_EVENT                      (UINT64_C(1) << 29)
#define CRTX_WIFI_CH_SWITCH_STARTED_NOTIFY      (UINT64_C(1) << 30)
#define CRTX_WIFI_WIPHY_REG_CHANGE              (UINT64_C(1) << 31)
#define CRTX_WIFI_NAN_MATCH                     (UINT64_C(1) << 32)
#define CRTX_WIFI_PORT_AUTHORIZED               (UINT64_C(1) << 33)
#define CRTX_WIFI_EXTERNAL_AUTH                 (UINT64_C(1) << 34)
#define CRTX_WIFI_STA_OPMODE_CHANGED            (UINT64_C(1) << 35)
#define CRTX_WIFI_CONTROL_PORT_FRAME            (UINT64_C(1) << 36)
#define CRTX_WIFI_PEER_MEASUREMENT_RESULT       (UINT64_C(1) << 37)
#define CRTX_WIFI_PEER_MEASUREMENT_COMPLETE     (UINT64_C(1) << 38)
#define CRTX_WIFI_NOTIFY_RADAR                  (UINT64_C(1) << 39)
#define CRTX_WIFI_UPDATE_OWE_INFO               (UINT64_C(1) << 40)
#define CRTX_WIFI_UNPROT_BEACON                 (UINT64_C(1) << 41)
#define CRTX_WIFI_CONTROL_PORT_FRAME_TX_STATUS  (UINT64_C(1) << 42)
#define CRTX_WIFI_OBSS_COLOR_COLLISION          (UINT64_C(1) << 43)
#define CRTX_WIFI_COLOR_CHANGE_STARTED          (UINT64_C(1) << 44)
#define CRTX_WIFI_COLOR_CHANGE_ABORTED          (UINT64_C(1) << 45)
#define CRTX_WIFI_COLOR_CHANGE_COMPLETED        (UINT64_C(1) << 46)
#define CRTX_WIFI_ASSOC_COMEBACK                (UINT64_C(1) << 47)
#define CRTX_WIFI_LINKS_REMOVED                 (UINT64_C(1) << 48)
#define CRTX_WIFI_NAN_NEXT_DW_NOTIFICATION      (UINT64_C(1) << 49)
#define CRTX_WIFI_NAN_CLUSTER_JOINED            (UINT64_C(1) << 50)
#define CRTX_WIFI_SCAN                          (CRTX_WIFI_NEW_SCAN_RESULTS | CRTX_WIFI_SCAN_ABORTED | CRTX_WIFI_SCHED_SCAN_RESULTS | CRTX_WIFI_SCHED_SCAN_STOPPED)
#define CRTX_WIFI_MONITOR_ALL                   UINT64_MAX

#define CRTX_EVENT_TYPE_FAMILY_WIFI (710)
#define CRTX_WIFI_ET_NEW_WIPHY                  (CRTX_EVENT_TYPE_FAMILY_WIFI + 1)
#define CRTX_WIFI_ET_DEL_WIPHY                  (CRTX_EVENT_TYPE_FAMILY_WIFI + 2)
#define CRTX_WIFI_ET_NEW_INTERFACE              (CRTX_EVENT_TYPE_FAMILY_WIFI + 3)
#define CRTX_WIFI_ET_DEL_INTERFACE              (CRTX_EVENT_TYPE_FAMILY_WIFI + 4)
#define CRTX_WIFI_ET_NEW_STATION                (CRTX_EVENT_TYPE_FAMILY_WIFI + 5)
#define CRTX_WIFI_ET_DEL_STATION                (CRTX_EVENT_TYPE_FAMILY_WIFI + 6)
#define CRTX_WIFI_ET_CONNECT                    (CRTX_EVENT_TYPE_FAMILY_WIFI + 7)
#define CRTX_WIFI_ET_DISCONNECT                 (CRTX_EVENT_TYPE_FAMILY_WIFI + 8)
#define CRTX_WIFI_ET_ROAM                       (CRTX_EVENT_TYPE_FAMILY_WIFI + 9)
#define CRTX_WIFI_ET_NEW_SCAN_RESULTS           (CRTX_EVENT_TYPE_FAMILY_WIFI + 10)
#define CRTX_WIFI_ET_SCAN_ABORTED               (CRTX_EVENT_TYPE_FAMILY_WIFI + 11)
#define CRTX_WIFI_ET_REG_CHANGE                 (CRTX_EVENT_TYPE_FAMILY_WIFI + 12)
#define CRTX_WIFI_ET_AUTHENTICATE               (CRTX_EVENT_TYPE_FAMILY_WIFI + 13)
#define CRTX_WIFI_ET_ASSOCIATE                  (CRTX_EVENT_TYPE_FAMILY_WIFI + 14)
#define CRTX_WIFI_ET_DEAUTHENTICATE             (CRTX_EVENT_TYPE_FAMILY_WIFI + 15)
#define CRTX_WIFI_ET_DISASSOCIATE               (CRTX_EVENT_TYPE_FAMILY_WIFI + 16)
#define CRTX_WIFI_ET_MICHAEL_MIC_FAILURE        (CRTX_EVENT_TYPE_FAMILY_WIFI + 17)
#define CRTX_WIFI_ET_REG_BEACON_HINT            (CRTX_EVENT_TYPE_FAMILY_WIFI + 18)
#define CRTX_WIFI_ET_NEW_SURVEY_RESULTS         (CRTX_EVENT_TYPE_FAMILY_WIFI + 19)
#define CRTX_WIFI_ET_FRAME                      (CRTX_EVENT_TYPE_FAMILY_WIFI + 20)
#define CRTX_WIFI_ET_FRAME_TX_STATUS            (CRTX_EVENT_TYPE_FAMILY_WIFI + 21)
#define CRTX_WIFI_ET_NOTIFY_CQM                 (CRTX_EVENT_TYPE_FAMILY_WIFI + 22)
#define CRTX_WIFI_ET_NEW_PEER_CANDIDATE         (CRTX_EVENT_TYPE_FAMILY_WIFI + 23)
#define CRTX_WIFI_ET_SCHED_SCAN_RESULTS         (CRTX_EVENT_TYPE_FAMILY_WIFI + 24)
#define CRTX_WIFI_ET_SCHED_SCAN_STOPPED         (CRTX_EVENT_TYPE_FAMILY_WIFI + 25)
#define CRTX_WIFI_ET_UNEXPECTED_FRAME           (CRTX_EVENT_TYPE_FAMILY_WIFI + 26)
#define CRTX_WIFI_ET_CH_SWITCH_NOTIFY           (CRTX_EVENT_TYPE_FAMILY_WIFI + 27)
#define CRTX_WIFI_ET_CONN_FAILED                (CRTX_EVENT_TYPE_FAMILY_WIFI + 28)
#define CRTX_WIFI_ET_RADAR_DETECT               (CRTX_EVENT_TYPE_FAMILY_WIFI + 29)
#define CRTX_WIFI_ET_FT_EVENT                   (CRTX_EVENT_TYPE_FAMILY_WIFI + 30)
#define CRTX_WIFI_ET_CH_SWITCH_STARTED_NOTIFY   (CRTX_EVENT_TYPE_FAMILY_WIFI + 31)
#define CRTX_WIFI_ET_WIPHY_REG_CHANGE           (CRTX_EVENT_TYPE_FAMILY_WIFI + 32)
#define CRTX_WIFI_ET_NAN_MATCH                  (CRTX_EVENT_TYPE_FAMILY_WIFI + 33)
#define CRTX_WIFI_ET_PORT_AUTHORIZED            (CRTX_EVENT_TYPE_FAMILY_WIFI + 34)
#define CRTX_WIFI_ET_EXTERNAL_AUTH              (CRTX_EVENT_TYPE_FAMILY_WIFI + 35)
#define CRTX_WIFI_ET_STA_OPMODE_CHANGED         (CRTX_EVENT_TYPE_FAMILY_WIFI + 36)
#define CRTX_WIFI_ET_CONTROL_PORT_FRAME         (CRTX_EVENT_TYPE_FAMILY_WIFI + 37)
#define CRTX_WIFI_ET_PEER_MEASUREMENT_RESULT    (CRTX_EVENT_TYPE_FAMILY_WIFI + 38)
#define CRTX_WIFI_ET_PEER_MEASUREMENT_COMPLETE  (CRTX_EVENT_TYPE_FAMILY_WIFI + 39)
#define CRTX_WIFI_ET_NOTIFY_RADAR               (CRTX_EVENT_TYPE_FAMILY_WIFI + 40)
#define CRTX_WIFI_ET_UPDATE_OWE_INFO            (CRTX_EVENT_TYPE_FAMILY_WIFI + 41)
#define CRTX_WIFI_ET_UNPROT_BEACON              (CRTX_EVENT_TYPE_FAMILY_WIFI + 42)
#define CRTX_WIFI_ET_CONTROL_PORT_FRAME_TX_STATUS (CRTX_EVENT_TYPE_FAMILY_WIFI + 43)
#define CRTX_WIFI_ET_OBSS_COLOR_COLLISION       (CRTX_EVENT_TYPE_FAMILY_WIFI + 44)
#define CRTX_WIFI_ET_COLOR_CHANGE_STARTED       (CRTX_EVENT_TYPE_FAMILY_WIFI + 45)
#define CRTX_WIFI_ET_COLOR_CHANGE_ABORTED       (CRTX_EVENT_TYPE_FAMILY_WIFI + 46)
#define CRTX_WIFI_ET_COLOR_CHANGE_COMPLETED     (CRTX_EVENT_TYPE_FAMILY_WIFI + 47)
#define CRTX_WIFI_ET_ASSOC_COMEBACK             (CRTX_EVENT_TYPE_FAMILY_WIFI + 48)
#define CRTX_WIFI_ET_LINKS_REMOVED              (CRTX_EVENT_TYPE_FAMILY_WIFI + 49)
#define CRTX_WIFI_ET_NAN_NEXT_DW_NOTIFICATION   (CRTX_EVENT_TYPE_FAMILY_WIFI + 50)
#define CRTX_WIFI_ET_NAN_CLUSTER_JOINED         (CRTX_EVENT_TYPE_FAMILY_WIFI + 51)
#define CRTX_WIFI_ET_CONNECTED                  CRTX_WIFI_ET_CONNECT
#define CRTX_WIFI_ET_DISCONNECTED               CRTX_WIFI_ET_DISCONNECT
#define CRTX_WIFI_ET_SCAN                       CRTX_WIFI_ET_NEW_SCAN_RESULTS

struct crtx_wifi_listener {
	struct crtx_listener_base base;

	struct crtx_genl_listener genl_lstnr;

	uint64_t monitor_types;

	int ifindex;
	const char *ifname;

	struct crtx_genl_callback callbacks[2];
	struct crtx_genl_group groups[3];
};

struct crtx_listener_base *crtx_setup_wifi_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(wifi)

void crtx_wifi_init();
void crtx_wifi_finish();

#ifdef __cplusplus
}
#endif

#endif
