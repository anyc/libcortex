
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

struct crtx_netlink_listener {
	struct crtx_listener_base parent;
	
	int socket_protocol;
	
	sa_family_t nl_family;
	// 	unsigned short nl_pad;
	// 	pid_t nl_pid;
	uint32_t nl_groups;
	
	uint16_t *nlmsg_types;
	
	struct crtx_dict *(*raw2dict)(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *nlh);
	char all_fields;
	
	struct crtx_signal msg_done;
	
	int sockfd;
	char stop;
};

struct crtx_listener_base *crtx_new_netlink_listener(void *options);
char crtx_netlink_send_req(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *n);
void crtx_netlink_init();
void crtx_netlink_finish();