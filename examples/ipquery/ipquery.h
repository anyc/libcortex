#ifndef IPQUERY_H
#define IPQUERY_H

#include <crtx/core.h>
#include <crtx/cache.h>
#include <crtx/dict.h>
#include <crtx/nl_route_raw.h>

struct ipq_monitor {
	struct crtx_nl_route_raw_listener *nlr_list;
	
	struct crtx_task *cache_task;
	
	struct crtx_signals *initial_response;
	
	void (*on_add)(struct crtx_dict *dict);
	void (*on_update)(struct crtx_dict *dict);
	void (*on_remove)(struct crtx_dict *dict);
};

int ipq_init();

int ipq_ips_setup_monitor(struct ipq_monitor **monitor_p);
int ipq_ips_start_monitor(struct ipq_monitor *monitor);
int ipq_ips_stop_monitor(struct ipq_monitor *monitor);
int ipq_ips_finish_monitor(struct ipq_monitor *monitor);

int ipq_loop();
int ipq_get_ips(struct crtx_dict **ips);
int ipq_finish();

#endif
