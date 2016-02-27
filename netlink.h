
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
	
	struct crtx_dict *(*raw2dict)(struct nlmsghdr *nlh);
	
	
	
	int sockfd;
	char stop;
};

struct crtx_listener_base *crtx_new_netlink_listener(void *options);
void crtx_netlink_init();
void crtx_netlink_finish();