/*
 * Mario Kicherer (dev@kicherer.org) 2026
 *
 */

#include <ctype.h>
#include <errno.h>
#include <linux/nl80211.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "core.h"
#include "wifi.h"

static const char *wifi_cmd_name(uint8_t cmd) {
	switch (cmd) {
		case NL80211_CMD_NEW_WIPHY:
			return "new_wiphy";
		case NL80211_CMD_DEL_WIPHY:
			return "del_wiphy";
		case NL80211_CMD_NEW_INTERFACE:
			return "new_interface";
		case NL80211_CMD_DEL_INTERFACE:
			return "del_interface";
		case NL80211_CMD_NEW_STATION:
			return "new_station";
		case NL80211_CMD_DEL_STATION:
			return "del_station";
		case NL80211_CMD_CONNECT:
			return "connect";
		case NL80211_CMD_DISCONNECT:
			return "disconnect";
		case NL80211_CMD_ROAM:
			return "roam";
		case NL80211_CMD_NEW_SCAN_RESULTS:
			return "new_scan_results";
		case NL80211_CMD_SCAN_ABORTED:
			return "scan_aborted";
		case NL80211_CMD_REG_CHANGE:
			return "reg_change";
		case NL80211_CMD_AUTHENTICATE:
			return "authenticate";
		case NL80211_CMD_ASSOCIATE:
			return "associate";
		case NL80211_CMD_DEAUTHENTICATE:
			return "deauthenticate";
		case NL80211_CMD_DISASSOCIATE:
			return "disassociate";
		case NL80211_CMD_MICHAEL_MIC_FAILURE:
			return "michael_mic_failure";
		case NL80211_CMD_REG_BEACON_HINT:
			return "reg_beacon_hint";
		case NL80211_CMD_NEW_SURVEY_RESULTS:
			return "new_survey_results";
		case NL80211_CMD_FRAME:
			return "frame";
		case NL80211_CMD_FRAME_TX_STATUS:
			return "frame_tx_status";
		case NL80211_CMD_NOTIFY_CQM:
			return "notify_cqm";
		case NL80211_CMD_NEW_PEER_CANDIDATE:
			return "new_peer_candidate";
		case NL80211_CMD_SCHED_SCAN_RESULTS:
			return "sched_scan_results";
		case NL80211_CMD_SCHED_SCAN_STOPPED:
			return "sched_scan_stopped";
		case NL80211_CMD_UNEXPECTED_FRAME:
			return "unexpected_frame";
		case NL80211_CMD_CH_SWITCH_NOTIFY:
			return "ch_switch_notify";
		case NL80211_CMD_CONN_FAILED:
			return "conn_failed";
		case NL80211_CMD_RADAR_DETECT:
			return "radar_detect";
		case NL80211_CMD_FT_EVENT:
			return "ft_event";
		case NL80211_CMD_CH_SWITCH_STARTED_NOTIFY:
			return "ch_switch_started_notify";
		case NL80211_CMD_WIPHY_REG_CHANGE:
			return "wiphy_reg_change";
		case NL80211_CMD_NAN_MATCH:
			return "nan_match";
		case NL80211_CMD_PORT_AUTHORIZED:
			return "port_authorized";
		case NL80211_CMD_EXTERNAL_AUTH:
			return "external_auth";
		case NL80211_CMD_STA_OPMODE_CHANGED:
			return "sta_opmode_changed";
		case NL80211_CMD_CONTROL_PORT_FRAME:
			return "control_port_frame";
		case NL80211_CMD_PEER_MEASUREMENT_RESULT:
			return "peer_measurement_result";
		case NL80211_CMD_PEER_MEASUREMENT_COMPLETE:
			return "peer_measurement_complete";
		case NL80211_CMD_NOTIFY_RADAR:
			return "notify_radar";
		case NL80211_CMD_UPDATE_OWE_INFO:
			return "update_owe_info";
		case NL80211_CMD_UNPROT_BEACON:
			return "unprot_beacon";
		case NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS:
			return "control_port_frame_tx_status";
		case NL80211_CMD_OBSS_COLOR_COLLISION:
			return "obss_color_collision";
		case NL80211_CMD_COLOR_CHANGE_STARTED:
			return "color_change_started";
		case NL80211_CMD_COLOR_CHANGE_ABORTED:
			return "color_change_aborted";
		case NL80211_CMD_COLOR_CHANGE_COMPLETED:
			return "color_change_completed";
		case NL80211_CMD_ASSOC_COMEBACK:
			return "assoc_comeback";
		case NL80211_CMD_LINKS_REMOVED:
			return "links_removed";
		case NL80211_CMD_NAN_NEXT_DW_NOTIFICATION:
			return "nan_next_dw_notification";
		case NL80211_CMD_NAN_CLUSTER_JOINED:
			return "nan_cluster_joined";
		default:
			return "unknown";
	}
}

static int wifi_event_type(uint8_t cmd) {
	switch (cmd) {
		case NL80211_CMD_NEW_WIPHY:
			return CRTX_WIFI_ET_NEW_WIPHY;
		case NL80211_CMD_DEL_WIPHY:
			return CRTX_WIFI_ET_DEL_WIPHY;
		case NL80211_CMD_NEW_INTERFACE:
			return CRTX_WIFI_ET_NEW_INTERFACE;
		case NL80211_CMD_DEL_INTERFACE:
			return CRTX_WIFI_ET_DEL_INTERFACE;
		case NL80211_CMD_NEW_STATION:
			return CRTX_WIFI_ET_NEW_STATION;
		case NL80211_CMD_DEL_STATION:
			return CRTX_WIFI_ET_DEL_STATION;
		case NL80211_CMD_CONNECT:
			return CRTX_WIFI_ET_CONNECT;
		case NL80211_CMD_DISCONNECT:
			return CRTX_WIFI_ET_DISCONNECT;
		case NL80211_CMD_ROAM:
			return CRTX_WIFI_ET_ROAM;
		case NL80211_CMD_NEW_SCAN_RESULTS:
			return CRTX_WIFI_ET_NEW_SCAN_RESULTS;
		case NL80211_CMD_SCAN_ABORTED:
			return CRTX_WIFI_ET_SCAN_ABORTED;
		case NL80211_CMD_REG_CHANGE:
			return CRTX_WIFI_ET_REG_CHANGE;
		case NL80211_CMD_AUTHENTICATE:
			return CRTX_WIFI_ET_AUTHENTICATE;
		case NL80211_CMD_ASSOCIATE:
			return CRTX_WIFI_ET_ASSOCIATE;
		case NL80211_CMD_DEAUTHENTICATE:
			return CRTX_WIFI_ET_DEAUTHENTICATE;
		case NL80211_CMD_DISASSOCIATE:
			return CRTX_WIFI_ET_DISASSOCIATE;
		case NL80211_CMD_MICHAEL_MIC_FAILURE:
			return CRTX_WIFI_ET_MICHAEL_MIC_FAILURE;
		case NL80211_CMD_REG_BEACON_HINT:
			return CRTX_WIFI_ET_REG_BEACON_HINT;
		case NL80211_CMD_NEW_SURVEY_RESULTS:
			return CRTX_WIFI_ET_NEW_SURVEY_RESULTS;
		case NL80211_CMD_FRAME:
			return CRTX_WIFI_ET_FRAME;
		case NL80211_CMD_FRAME_TX_STATUS:
			return CRTX_WIFI_ET_FRAME_TX_STATUS;
		case NL80211_CMD_NOTIFY_CQM:
			return CRTX_WIFI_ET_NOTIFY_CQM;
		case NL80211_CMD_NEW_PEER_CANDIDATE:
			return CRTX_WIFI_ET_NEW_PEER_CANDIDATE;
		case NL80211_CMD_SCHED_SCAN_RESULTS:
			return CRTX_WIFI_ET_SCHED_SCAN_RESULTS;
		case NL80211_CMD_SCHED_SCAN_STOPPED:
			return CRTX_WIFI_ET_SCHED_SCAN_STOPPED;
		case NL80211_CMD_UNEXPECTED_FRAME:
			return CRTX_WIFI_ET_UNEXPECTED_FRAME;
		case NL80211_CMD_CH_SWITCH_NOTIFY:
			return CRTX_WIFI_ET_CH_SWITCH_NOTIFY;
		case NL80211_CMD_CONN_FAILED:
			return CRTX_WIFI_ET_CONN_FAILED;
		case NL80211_CMD_RADAR_DETECT:
			return CRTX_WIFI_ET_RADAR_DETECT;
		case NL80211_CMD_FT_EVENT:
			return CRTX_WIFI_ET_FT_EVENT;
		case NL80211_CMD_CH_SWITCH_STARTED_NOTIFY:
			return CRTX_WIFI_ET_CH_SWITCH_STARTED_NOTIFY;
		case NL80211_CMD_WIPHY_REG_CHANGE:
			return CRTX_WIFI_ET_WIPHY_REG_CHANGE;
		case NL80211_CMD_NAN_MATCH:
			return CRTX_WIFI_ET_NAN_MATCH;
		case NL80211_CMD_PORT_AUTHORIZED:
			return CRTX_WIFI_ET_PORT_AUTHORIZED;
		case NL80211_CMD_EXTERNAL_AUTH:
			return CRTX_WIFI_ET_EXTERNAL_AUTH;
		case NL80211_CMD_STA_OPMODE_CHANGED:
			return CRTX_WIFI_ET_STA_OPMODE_CHANGED;
		case NL80211_CMD_CONTROL_PORT_FRAME:
			return CRTX_WIFI_ET_CONTROL_PORT_FRAME;
		case NL80211_CMD_PEER_MEASUREMENT_RESULT:
			return CRTX_WIFI_ET_PEER_MEASUREMENT_RESULT;
		case NL80211_CMD_PEER_MEASUREMENT_COMPLETE:
			return CRTX_WIFI_ET_PEER_MEASUREMENT_COMPLETE;
		case NL80211_CMD_NOTIFY_RADAR:
			return CRTX_WIFI_ET_NOTIFY_RADAR;
		case NL80211_CMD_UPDATE_OWE_INFO:
			return CRTX_WIFI_ET_UPDATE_OWE_INFO;
		case NL80211_CMD_UNPROT_BEACON:
			return CRTX_WIFI_ET_UNPROT_BEACON;
		case NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS:
			return CRTX_WIFI_ET_CONTROL_PORT_FRAME_TX_STATUS;
		case NL80211_CMD_OBSS_COLOR_COLLISION:
			return CRTX_WIFI_ET_OBSS_COLOR_COLLISION;
		case NL80211_CMD_COLOR_CHANGE_STARTED:
			return CRTX_WIFI_ET_COLOR_CHANGE_STARTED;
		case NL80211_CMD_COLOR_CHANGE_ABORTED:
			return CRTX_WIFI_ET_COLOR_CHANGE_ABORTED;
		case NL80211_CMD_COLOR_CHANGE_COMPLETED:
			return CRTX_WIFI_ET_COLOR_CHANGE_COMPLETED;
		case NL80211_CMD_ASSOC_COMEBACK:
			return CRTX_WIFI_ET_ASSOC_COMEBACK;
		case NL80211_CMD_LINKS_REMOVED:
			return CRTX_WIFI_ET_LINKS_REMOVED;
		case NL80211_CMD_NAN_NEXT_DW_NOTIFICATION:
			return CRTX_WIFI_ET_NAN_NEXT_DW_NOTIFICATION;
		case NL80211_CMD_NAN_CLUSTER_JOINED:
			return CRTX_WIFI_ET_NAN_CLUSTER_JOINED;
		default:
			return 0;
	}
}

static uint64_t wifi_monitor_type(uint8_t cmd) {
	switch (cmd) {
		case NL80211_CMD_NEW_WIPHY:
			return CRTX_WIFI_NEW_WIPHY;
		case NL80211_CMD_DEL_WIPHY:
			return CRTX_WIFI_DEL_WIPHY;
		case NL80211_CMD_NEW_INTERFACE:
			return CRTX_WIFI_NEW_INTERFACE;
		case NL80211_CMD_DEL_INTERFACE:
			return CRTX_WIFI_DEL_INTERFACE;
		case NL80211_CMD_NEW_STATION:
			return CRTX_WIFI_NEW_STATION;
		case NL80211_CMD_DEL_STATION:
			return CRTX_WIFI_DEL_STATION;
		case NL80211_CMD_CONNECT:
			return CRTX_WIFI_CONNECT;
		case NL80211_CMD_DISCONNECT:
			return CRTX_WIFI_DISCONNECT;
		case NL80211_CMD_ROAM:
			return CRTX_WIFI_ROAM;
		case NL80211_CMD_NEW_SCAN_RESULTS:
			return CRTX_WIFI_NEW_SCAN_RESULTS;
		case NL80211_CMD_SCAN_ABORTED:
			return CRTX_WIFI_SCAN_ABORTED;
		case NL80211_CMD_REG_CHANGE:
			return CRTX_WIFI_REG_CHANGE;
		case NL80211_CMD_AUTHENTICATE:
			return CRTX_WIFI_AUTHENTICATE;
		case NL80211_CMD_ASSOCIATE:
			return CRTX_WIFI_ASSOCIATE;
		case NL80211_CMD_DEAUTHENTICATE:
			return CRTX_WIFI_DEAUTHENTICATE;
		case NL80211_CMD_DISASSOCIATE:
			return CRTX_WIFI_DISASSOCIATE;
		case NL80211_CMD_MICHAEL_MIC_FAILURE:
			return CRTX_WIFI_MICHAEL_MIC_FAILURE;
		case NL80211_CMD_REG_BEACON_HINT:
			return CRTX_WIFI_REG_BEACON_HINT;
		case NL80211_CMD_NEW_SURVEY_RESULTS:
			return CRTX_WIFI_NEW_SURVEY_RESULTS;
		case NL80211_CMD_FRAME:
			return CRTX_WIFI_FRAME;
		case NL80211_CMD_FRAME_TX_STATUS:
			return CRTX_WIFI_FRAME_TX_STATUS;
		case NL80211_CMD_NOTIFY_CQM:
			return CRTX_WIFI_NOTIFY_CQM;
		case NL80211_CMD_NEW_PEER_CANDIDATE:
			return CRTX_WIFI_NEW_PEER_CANDIDATE;
		case NL80211_CMD_SCHED_SCAN_RESULTS:
			return CRTX_WIFI_SCHED_SCAN_RESULTS;
		case NL80211_CMD_SCHED_SCAN_STOPPED:
			return CRTX_WIFI_SCHED_SCAN_STOPPED;
		case NL80211_CMD_UNEXPECTED_FRAME:
			return CRTX_WIFI_UNEXPECTED_FRAME;
		case NL80211_CMD_CH_SWITCH_NOTIFY:
			return CRTX_WIFI_CH_SWITCH_NOTIFY;
		case NL80211_CMD_CONN_FAILED:
			return CRTX_WIFI_CONN_FAILED;
		case NL80211_CMD_RADAR_DETECT:
			return CRTX_WIFI_RADAR_DETECT;
		case NL80211_CMD_FT_EVENT:
			return CRTX_WIFI_FT_EVENT;
		case NL80211_CMD_CH_SWITCH_STARTED_NOTIFY:
			return CRTX_WIFI_CH_SWITCH_STARTED_NOTIFY;
		case NL80211_CMD_WIPHY_REG_CHANGE:
			return CRTX_WIFI_WIPHY_REG_CHANGE;
		case NL80211_CMD_NAN_MATCH:
			return CRTX_WIFI_NAN_MATCH;
		case NL80211_CMD_PORT_AUTHORIZED:
			return CRTX_WIFI_PORT_AUTHORIZED;
		case NL80211_CMD_EXTERNAL_AUTH:
			return CRTX_WIFI_EXTERNAL_AUTH;
		case NL80211_CMD_STA_OPMODE_CHANGED:
			return CRTX_WIFI_STA_OPMODE_CHANGED;
		case NL80211_CMD_CONTROL_PORT_FRAME:
			return CRTX_WIFI_CONTROL_PORT_FRAME;
		case NL80211_CMD_PEER_MEASUREMENT_RESULT:
			return CRTX_WIFI_PEER_MEASUREMENT_RESULT;
		case NL80211_CMD_PEER_MEASUREMENT_COMPLETE:
			return CRTX_WIFI_PEER_MEASUREMENT_COMPLETE;
		case NL80211_CMD_NOTIFY_RADAR:
			return CRTX_WIFI_NOTIFY_RADAR;
		case NL80211_CMD_UPDATE_OWE_INFO:
			return CRTX_WIFI_UPDATE_OWE_INFO;
		case NL80211_CMD_UNPROT_BEACON:
			return CRTX_WIFI_UNPROT_BEACON;
		case NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS:
			return CRTX_WIFI_CONTROL_PORT_FRAME_TX_STATUS;
		case NL80211_CMD_OBSS_COLOR_COLLISION:
			return CRTX_WIFI_OBSS_COLOR_COLLISION;
		case NL80211_CMD_COLOR_CHANGE_STARTED:
			return CRTX_WIFI_COLOR_CHANGE_STARTED;
		case NL80211_CMD_COLOR_CHANGE_ABORTED:
			return CRTX_WIFI_COLOR_CHANGE_ABORTED;
		case NL80211_CMD_COLOR_CHANGE_COMPLETED:
			return CRTX_WIFI_COLOR_CHANGE_COMPLETED;
		case NL80211_CMD_ASSOC_COMEBACK:
			return CRTX_WIFI_ASSOC_COMEBACK;
		case NL80211_CMD_LINKS_REMOVED:
			return CRTX_WIFI_LINKS_REMOVED;
		case NL80211_CMD_NAN_NEXT_DW_NOTIFICATION:
			return CRTX_WIFI_NAN_NEXT_DW_NOTIFICATION;
		case NL80211_CMD_NAN_CLUSTER_JOINED:
			return CRTX_WIFI_NAN_CLUSTER_JOINED;
		default:
			return 0;
	}
}

static char *wifi_strdup_mac(const unsigned char *mac, size_t len) {
	char *s;

	if (!mac || len < 6)
		return 0;

	s = (char *) calloc(1, 18);
	if (!s)
		return 0;

	snprintf(s, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return s;
}

static char *wifi_strdup_ssid(const unsigned char *ssid, size_t len) {
	char *s;
	size_t i, off, alloc;

	if (!ssid)
		return 0;

	alloc = len * 4 + 1;
	s = (char *) calloc(1, alloc);
	if (!s)
		return 0;

	for (i = 0, off = 0; i < len && off + 1 < alloc; i++) {
		if (isprint(ssid[i]) && ssid[i] != '\\') {
			s[off++] = (char) ssid[i];
		} else if (off + 4 < alloc) {
			snprintf(&s[off], alloc - off, "\\x%02x", ssid[i]);
			off += 4;
		}
	}

	return s;
}

static int wifi_matches_filter(struct crtx_wifi_listener *wifi_lstnr, int ifindex, const char *ifname) {
	if (wifi_lstnr->ifindex > 0 && wifi_lstnr->ifindex != ifindex)
		return 0;

	if (wifi_lstnr->ifname && (!ifname || strcmp(wifi_lstnr->ifname, ifname) != 0))
		return 0;

	return 1;
}

static void wifi_fill_common_fields(struct crtx_dict *dict, struct nlattr **attrs, int ifindex, const char *ifname) {
	char ifname_buf[IF_NAMESIZE];
	char *value;

	crtx_dict_new_item(dict, 'u', "ifindex", (uint32_t) ifindex, sizeof(uint32_t), 0);

	if (ifname) {
		crtx_dict_new_item(dict, 's', "ifname", (char *) ifname, 0, CRTX_DIF_CREATE_DATA_COPY);
	} else if (ifindex > 0 && if_indextoname(ifindex, ifname_buf)) {
		crtx_dict_new_item(dict, 's', "ifname", ifname_buf, 0, CRTX_DIF_CREATE_DATA_COPY);
	}

	if (attrs[NL80211_ATTR_WIPHY])
		crtx_dict_new_item(dict, 'u', "wiphy", nla_get_u32(attrs[NL80211_ATTR_WIPHY]), sizeof(uint32_t), 0);
	if (attrs[NL80211_ATTR_WIPHY_FREQ])
		crtx_dict_new_item(dict, 'u', "freq", nla_get_u32(attrs[NL80211_ATTR_WIPHY_FREQ]), sizeof(uint32_t), 0);
	if (attrs[NL80211_ATTR_MAC]) {
		value = wifi_strdup_mac((unsigned char *) nla_data(attrs[NL80211_ATTR_MAC]),
					nla_len(attrs[NL80211_ATTR_MAC]));
		if (value)
			crtx_dict_new_item(dict, 's', "bssid", value, 0, 0);
	}
	if (attrs[NL80211_ATTR_SSID]) {
		value = wifi_strdup_ssid((unsigned char *) nla_data(attrs[NL80211_ATTR_SSID]),
					 nla_len(attrs[NL80211_ATTR_SSID]));
		if (value)
			crtx_dict_new_item(dict, 's', "ssid", value, 0, 0);
	}
}

static int crtx_wifi_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_wifi_listener *wifi_lstnr;
	struct crtx_dict *dict;
	struct crtx_event *event;
	struct nlmsghdr *nlh;
	struct genlmsghdr *gnlh;
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];
	const char *ifname;
	char ifname_buf[IF_NAMESIZE];
	uint8_t cmd;
	int event_type;
	uint64_t monitor_type;
	int ifindex;
	int r;

	wifi_lstnr = (struct crtx_wifi_listener *) arg;

	nlh = nlmsg_hdr(msg);
	gnlh = genlmsg_hdr(nlh);
	cmd = gnlh->cmd;

	monitor_type = wifi_monitor_type(cmd);
	if (!monitor_type)
		return NL_OK;

	if (!((wifi_lstnr->monitor_types ? wifi_lstnr->monitor_types : CRTX_WIFI_MONITOR_ALL) & monitor_type))
		return NL_OK;

	memset(attrs, 0, sizeof(attrs));
	r = genlmsg_parse(nlh, 0, attrs, NL80211_ATTR_MAX, 0);
	if (r < 0) {
		CRTX_ERROR("genlmsg_parse failed: %d\n", r);
		return NL_OK;
	}

	ifindex = attrs[NL80211_ATTR_IFINDEX] ? (int) nla_get_u32(attrs[NL80211_ATTR_IFINDEX]) : 0;
	ifname = 0;
	if (attrs[NL80211_ATTR_IFNAME]) {
		ifname = nla_get_string(attrs[NL80211_ATTR_IFNAME]);
	} else if (ifindex > 0 && if_indextoname(ifindex, ifname_buf)) {
		ifname = ifname_buf;
	}

	if (!wifi_matches_filter(wifi_lstnr, ifindex, ifname))
		return NL_OK;

	event_type = wifi_event_type(cmd);
	if (!event_type)
		return NL_OK;

	dict = crtx_init_dict(0, 0, 0);
	if (!dict) {
		CRTX_ERROR("crtx_init_dict failed\n");
		return NL_OK;
	}

	crtx_dict_new_item(dict, 'u', "cmd", (uint32_t) cmd, sizeof(uint32_t), 0);
	crtx_dict_new_item(dict, 's', "cmd_name", (char *) wifi_cmd_name(cmd), 0, CRTX_DIF_CREATE_DATA_COPY);
	wifi_fill_common_fields(dict, attrs, ifindex, ifname);

	switch (cmd) {
		case NL80211_CMD_CONNECT:
			if (attrs[NL80211_ATTR_STATUS_CODE])
				crtx_dict_new_item(dict, 'u', "status_code",
						   nla_get_u16(attrs[NL80211_ATTR_STATUS_CODE]), sizeof(uint32_t), 0);
			crtx_dict_new_item(dict, 'u', "connected",
					   attrs[NL80211_ATTR_STATUS_CODE] ?
						   nla_get_u16(attrs[NL80211_ATTR_STATUS_CODE]) == 0 :
						   1,
					   sizeof(uint32_t), 0);
			crtx_dict_new_item(dict, 'u', "timed_out", attrs[NL80211_ATTR_TIMED_OUT] ? 1 : 0, sizeof(uint32_t), 0);
			break;
		case NL80211_CMD_DISCONNECT:
			if (attrs[NL80211_ATTR_REASON_CODE])
				crtx_dict_new_item(dict, 'u', "reason_code",
						   nla_get_u16(attrs[NL80211_ATTR_REASON_CODE]), sizeof(uint32_t), 0);
			crtx_dict_new_item(dict, 'u', "disconnected_by_ap",
					   attrs[NL80211_ATTR_DISCONNECTED_BY_AP] ? 1 : 0, sizeof(uint32_t), 0);
			break;
		case NL80211_CMD_ROAM:
			crtx_dict_new_item(dict, 'u', "connected", 1, sizeof(uint32_t), 0);
			break;
		case NL80211_CMD_NEW_SCAN_RESULTS:
		case NL80211_CMD_SCAN_ABORTED:
		case NL80211_CMD_SCHED_SCAN_RESULTS:
		case NL80211_CMD_SCHED_SCAN_STOPPED:
			crtx_dict_new_item(dict, 's', "scan_state", (char *) wifi_cmd_name(cmd), 0,
					   CRTX_DIF_CREATE_DATA_COPY);
			break;
	}

	r = crtx_create_event(&event);
	if (r) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(-r));
		crtx_dict_unref(dict);
		return NL_OK;
	}

	event->description = "nl80211";
	event->type = event_type;
	crtx_event_set_dict_data(event, dict, 0);
	crtx_add_event(wifi_lstnr->base.graph, event);

	return NL_OK;
}

static struct crtx_genl_callback wifi_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_wifi_msg2dict, 0 },
	{ 0 },
};

static struct crtx_genl_group wifi_groups[] = {
	{ "nl80211", NL80211_MULTICAST_GROUP_MLME, 0, 0, 0, 0, 0 },
	{ "nl80211", NL80211_MULTICAST_GROUP_SCAN, 0, 0, 0, 0, 0 },
	{ 0 },
};

static int wifi_start_listener(struct crtx_listener_base *listener) {
	struct crtx_wifi_listener *wifi_lstnr;
	int ret;

	wifi_lstnr = (struct crtx_wifi_listener *) listener;

	ret = crtx_start_listener(&wifi_lstnr->genl_lstnr.base);
	if (ret) {
		CRTX_ERROR("starting genl listener failed\n");
		return ret;
	}

	wifi_lstnr->base.mode = CRTX_NO_PROCESSING_MODE;

	return 0;
}

static int wifi_stop_listener(struct crtx_listener_base *listener) {
	struct crtx_wifi_listener *wifi_lstnr;

	wifi_lstnr = (struct crtx_wifi_listener *) listener;

	crtx_stop_listener(&wifi_lstnr->genl_lstnr.base);

	return 0;
}

static void wifi_shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_wifi_listener *wifi_lstnr;

	wifi_lstnr = (struct crtx_wifi_listener *) listener;

	crtx_shutdown_listener(&wifi_lstnr->genl_lstnr.base);
	wifi_lstnr->genl_lstnr.base.graph = 0;
}

struct crtx_listener_base *crtx_setup_wifi_listener(void *options) {
	struct crtx_wifi_listener *wifi_lstnr;
	struct crtx_genl_callback *cbit;
	int ret;

	wifi_lstnr = (struct crtx_wifi_listener *) options;

	CRTX_STATIC_ASSERT(sizeof(wifi_callbacks) == sizeof(wifi_lstnr->callbacks), "size mismatch");
	CRTX_STATIC_ASSERT(sizeof(wifi_groups) == sizeof(wifi_lstnr->groups), "size mismatch");
	memcpy(wifi_lstnr->callbacks, wifi_callbacks, sizeof(wifi_callbacks));
	memcpy(wifi_lstnr->groups, wifi_groups, sizeof(wifi_groups));

	for (cbit = wifi_lstnr->callbacks; cbit->func; cbit++) {
		cbit->arg = wifi_lstnr;
	}

	wifi_lstnr->genl_lstnr.callbacks = wifi_lstnr->callbacks;
	wifi_lstnr->genl_lstnr.groups = wifi_lstnr->groups;

	ret = crtx_setup_listener("netlink_ge", &wifi_lstnr->genl_lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(netlink_ge) failed\n");
		return 0;
	}

	wifi_lstnr->base.shutdown = &wifi_shutdown_listener;
	wifi_lstnr->base.start_listener = &wifi_start_listener;
	wifi_lstnr->base.stop_listener = &wifi_stop_listener;

	return &wifi_lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(wifi)

void crtx_wifi_init() {}
void crtx_wifi_finish() {}

#ifdef CRTX_TEST

static int wifi_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;

	if (event->data.type == 'u') {
		if (event->data.uint32 == CRTX_LSTNR_STOPPED)
			crtx_init_shutdown();
		return 0;
	}

	crtx_event_get_payload(event, 0, 0, &dict);
	crtx_print_dict(dict);

	return 0;
}

int wifi_main(int argc, char **argv) {
	struct crtx_wifi_listener wifi_lstnr;
	int r;

	memset(&wifi_lstnr, 0, sizeof(struct crtx_wifi_listener));

	r = crtx_setup_listener("wifi", &wifi_lstnr);
	if (r) {
		CRTX_ERROR("create_listener(wifi) failed\n");
		exit(1);
	}

	wifi_lstnr.base.state_graph = wifi_lstnr.base.graph;
	crtx_create_task(wifi_lstnr.base.graph, 0, "wifi_event_handler", wifi_event_handler, 0);

	r = crtx_start_listener(&wifi_lstnr.base);
	if (r) {
		CRTX_ERROR("starting wifi listener failed: %s (%d)\n", strerror(-r), r);
		return 1;
	}

	crtx_loop();

	crtx_shutdown_listener(&wifi_lstnr.base);

	return 0;
}

CRTX_TEST_MAIN(wifi_main);

#endif
