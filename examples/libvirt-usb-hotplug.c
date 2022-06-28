/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * This application dynamically adds and removes USB devices to/from a
 * libvirt-managed virtual machine if the devices are plugged in or out or if
 * virtual machines are started.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libvirt/libvirt.h>
#include <libxml/parser.h>

#include "intern.h"
#include "core.h"
#include "udev.h"
#include "dict_inout_json.h"
#include "libvirt.h"
#include "timer.h"
#include "cache.h"
#include "dict.h"


char * req_usb_attrs[] = {
	"ID_VENDOR_ID",
	"ID_MODEL_ID",
	"ID_VENDOR_FROM_DATABASE",
	"ID_MODEL",
	0,
};

struct crtx_udev_raw2dict_attr_req r2ds[] = {
	{ "usb", "usb_device", req_usb_attrs },
	{ 0 },
};

struct crtx_dict *cfg_dict = 0;
struct crtx_libvirt_listener lvlist;
struct crtx_timer_retry_listener *retry_lstnr;
struct crtx_cache *udev_cache;

char *test_filters[] = {
	"usb",
	"usb_device",
	0,
	0,
};

static char template[] = " \n\
<hostdev mode='subsystem' type='usb'> \n\
	<source> \n\
		<vendor id=\"0x%s\" /> \n\
		<product id=\"0x%s\" /> \n\
	</source> \n\
</hostdev> \n\
";


// check if given USB device is already attached to the given virtual machine
char is_dev_attached(virDomainPtr dom, char *check_vendor, char *check_product) {
	xmlDoc *document;
	xmlNode *root, *node, *node2, *node3, *node4;
	char *xml;
	char stop;
	
	
	xml = virDomainGetXMLDesc(dom, 0);
	document = xmlReadMemory(xml, strlen(xml), 0, 0, 0);
	root = xmlDocGetRootElement(document);
	
	stop = 0;
	for (node = root->children; node; node = node->next) {
		if (!xmlStrcmp(node->name, (const xmlChar *) "devices")) {
			for (node2 = node->children; node2; node2 = node2->next) {
				if (!xmlStrcmp(node2->name, (const xmlChar *) "hostdev")) {
					char *type;
					type = xmlGetProp(node2, (const xmlChar *) "type");
					if (!xmlStrcmp(type, (const xmlChar *) "usb")) {
						for (node3 = node2->children; node3; node3 = node3->next) {
							if (!xmlStrcmp(node3->name, (const xmlChar *) "source")) {
								char *vendor, *product;
								vendor = product = 0;
								for (node4 = node3->children; node4; node4 = node4->next) {
									if (!xmlStrcmp(node4->name, (const xmlChar *) "vendor")) {
										vendor = (char*) xmlGetProp(node4, (const xmlChar *) "id");
									} else
									if (!xmlStrcmp(node4->name, (const xmlChar *) "product")) {
										product = (char*) xmlGetProp(node4, (const xmlChar *) "id");
									}
								}
								
								if (product && vendor) {
									if (!strcmp(&product[2], check_product) && !strcmp(&vendor[2], check_vendor))
										stop = 1;
								}
								if (product)
									xmlFree(product);
								if (vendor)
									xmlFree(vendor);
							}
							if (stop) break;
						}
					}
					xmlFree(type);
				}
				if (stop) break;
			}
		}
		if (stop) break;
	}
	
	xmlFreeDoc(document);
	free(xml);
	
	return stop;
}

static char udev_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict, *usb_dict;
	int ret;
	char *vendor, *product, *vm_name;
	char xml[sizeof(template)+4];
	virDomainPtr dom;
	char *action;
	
	
	dict = crtx_udev_raw2dict((struct udev_device *) crtx_event_get_ptr(event), r2ds, 1);
	
	// upgrade event payload
	crtx_event_set_dict_data(event, dict, 0);
	
	usb_dict = crtx_get_dict(dict, "usb-usb_device");
	
	vendor = crtx_get_string(usb_dict, "ID_VENDOR_ID");
	product = crtx_get_string(usb_dict, "ID_MODEL_ID");
	action = crtx_get_string(dict, "ACTION");
	
	// find VM for this device in the config dictionary
	vm_name = 0;
	{
		struct crtx_dict *vm_dict, *usb_dev_list;
		struct crtx_dict_item *di, *di2;
		int i, j;
		char *v, *p;
		
		
		for (i=0; i < cfg_dict->signature_length; i++) {
			di = crtx_get_item_by_idx(cfg_dict, i);
			
			if (di->type != 'D')
				continue;
			
			vm_dict = di->dict;
			for (j=0; j < vm_dict->signature_length; j++) {
				di2 = crtx_get_item_by_idx(vm_dict, j);
				
				if (di2->type != 'D')
					continue;
				
				usb_dev_list = di2->dict;
				
				if (usb_dev_list->signature_length < 2)
					continue;
				
				v = crtx_get_item_by_idx(usb_dev_list, 0)->string;
				p = crtx_get_item_by_idx(usb_dev_list, 1)->string;
				
				if (!strcmp(v, vendor) && !strcmp(p, product)) {
					vm_name = di->key;
					break;
				}
			}
			
			if (vm_name)
				break;
		}
	}
	
	if (!vm_name) {
		CRTX_INFO("no VM found for device %s:%s (%s)\n", vendor, product, action);
		return 0;
	} else {
		CRTX_INFO("event %s device %s:%s for VM \"%s\"\n", action, vendor, product, vm_name);
	}
	
	dom = virDomainLookupByName(lvlist.conn, vm_name);
	if (dom == NULL) {
		CRTX_ERROR("Failed to find Domain \"%s\"\n", vm_name);
		return 0;
	}
	
	if (!virDomainIsActive(dom)) {
		CRTX_INFO("domain %s not active\n", vm_name);
		
		virDomainFree(dom);
		return 0;
	}
	
	if (!strcmp(action, "add") || !strcmp(action, "initial")) {
		if (is_dev_attached(dom, vendor, product)) {
			CRTX_INFO("dev already present\n");
		} else {
			CRTX_INFO("adding\n");
			snprintf(xml, sizeof(template)+4, template, vendor, product);
			
			ret = virDomainAttachDevice(dom, xml);
			if (ret < 0) {
				CRTX_ERROR("virDomainAttachDevice failed: %d\n", ret);
			}
		}
	} else if (!strcmp(action, "remove")) {
		if (!is_dev_attached(dom, vendor, product)) {
			CRTX_INFO("dev not present\n");
		} else {
			CRTX_INFO("removing\n");
			snprintf(xml, sizeof(template)+4, template, vendor, product);
			
			ret = virDomainDetachDevice(dom, xml);
			if (ret < 0) {
				CRTX_ERROR("virDomainDetachDevice failed: %d\n", ret);
			}
		}
	}
	
	virDomainFree(dom);
	
	return 0;
}

static char libvirt_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	// first handle events that indicate a change of our listener state
	if (!strcmp(event->description, "listener_state")) {
		if (event->data.uint32 == CRTX_LSTNR_STARTED) {
			crtx_stop_listener(&retry_lstnr->timer_lstnr.base);
		}
		if (event->data.uint32 == CRTX_LSTNR_STOPPED) {
			crtx_start_listener(&retry_lstnr->timer_lstnr.base);
		}
	} else
	if (!strcmp(event->description, "domain_event")) {
		const char *vm_name;
		char *type;
		virDomainPtr dom;
		struct crtx_dict_item *id_item;
		struct crtx_dict *dict;
		
		crtx_event_get_payload(event, 0, 0, &dict);
		
		if (!dict) {
			CRTX_DBG("unknown event data\n");
			return 0;
		}
		
		vm_name = crtx_get_string(dict, "name");
		id_item = crtx_get_item(dict, "id");
		type = crtx_get_string(dict, "type");
		
		if (!type) {
			CRTX_DBG("unknown event data for %s\n", vm_name);
			return 0;
		}
		
		// get a libvirt handle for this VM (by Id first)
		dom = 0;
		if (id_item && id_item->type == 'u') {
			int id;
			
			if (id_item->uint32 <= INT_MAX) {
				id = id_item->uint32;
				
				// getId return unsigned int but this function wants int
				dom = virDomainLookupByID(lvlist.conn, id);
				
				if (!vm_name)
					vm_name = virDomainGetName(dom);
			} else
				CRTX_DBG("received domain id is larger than INT_MAX\n");
		}
		
		if (dom == 0 && vm_name) {
			dom = virDomainLookupByName(lvlist.conn, vm_name);
			if (dom == NULL) {
				fprintf(stderr, "Failed to find Domain \"%s\"\n", vm_name);
				return 0;
			}
		}
		
		CRTX_INFO("VM \"%s\": %s\n", vm_name, type);
		
		// we are only interested in starting VMs as stopped VMs forget their
		// dynamically attached devices automatically
		if (!strcmp(type, "Started")) {
			struct crtx_dict *vm_dict, *usb_dev_list;
			struct crtx_dict_item *di, *di2;
			int i, j;
			char *v, *p;
			
			// find the entry for this VM in the config and check if one of the
			// configured USB devices is present in the presence cache
			for (i=0; i < cfg_dict->signature_length; i++) {
				di = crtx_get_item_by_idx(cfg_dict, i);
				
				if (di->type != 'D')
					continue;
				
				if (strcmp(vm_name, di->key))
					continue;
				
				vm_dict = di->dict;
				for (j=0; j < vm_dict->signature_length; j++) {
					di2 = crtx_get_item_by_idx(vm_dict, j);
					
					if (di2->type != 'D')
						continue;
					
					usb_dev_list = di2->dict;
					
					if (usb_dev_list->signature_length < 2)
						continue;
					
					v = crtx_get_item_by_idx(usb_dev_list, 0)->string;
					p = crtx_get_item_by_idx(usb_dev_list, 1)->string;
					
					// lookup vendor:product in the presence cache
					int k;
					struct crtx_dict *cache_dict;
					cache_dict = udev_cache->entries;
					for (k=0; k < cache_dict->signature_length; k++) {
						struct crtx_dict_item *di, *di_data;
						char r;
						char *cache_vendor, *cache_product;
						
						di = crtx_get_item_by_idx(cache_dict, k);
						
						r = crtx_dict_locate_value(di->dict, "data/usb-usb_device/ID_VENDOR_ID", 's', &cache_vendor, sizeof(char*));
						if (r)
							continue;
						r = crtx_dict_locate_value(di->dict, "data/usb-usb_device/ID_MODEL_ID", 's', &cache_product, sizeof(char*));
						if (r)
							continue;
						
						if (!strcmp(v, cache_vendor) && !strcmp(p, cache_product)) {
							if (is_dev_attached(dom, v, p)) {
								CRTX_INFO("dev already present\n");
							} else {
								char xml[sizeof(template)+4];
								int ret;
								
								
								CRTX_INFO("adding %s:%s to \"%s\"\n", v, p, vm_name);
								
								snprintf(xml, sizeof(template)+4, template, v, p);
								
								ret = virDomainAttachDevice(dom, xml);
								if (ret < 0) {
									CRTX_ERROR("virDomainAttachDevice failed: %d\n", ret);
								}
							}
						}
					}
				}
				
				break;
			}
		}
		
		virDomainFree(dom);
	}
	
	return 0;
}

// create the primary key for the presence cache entry and determine if this is
// an "add" or "remove" event
static int create_key_action_cb(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_dict *usb_dict, *data_dict;
	char *vendor, *product;
	
	
	crtx_event_get_payload(event, 0, 0, &data_dict);
	usb_dict = crtx_get_dict(data_dict, "usb-usb_device");
	
	if (!usb_dict) {
		CRTX_ERROR("cannot create key for udev event, crtx_get_dict \"usb-usb_device\" failed\n");
		crtx_print_dict_item(&event->data, 0);
		return 1;
	}
	
	vendor = crtx_get_string(usb_dict, "ID_VENDOR_ID");
	product = crtx_get_string(usb_dict, "ID_MODEL_ID");
	
	if (!vendor || !product) {
		CRTX_ERROR("cannot create key for udev event\n");
		crtx_print_dict_item(&event->data, 0);
		return 1;
	}
	
	key->type = 's';
	key->string = (char*) malloc(strlen(vendor)+1+strlen(product)+1);
	key->flags = CRTX_DIF_ALLOCATED_KEY;
	sprintf(key->string, "%s-%s", vendor, product);
	
	return 0;
}


static int cache_update_on_hit(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	struct crtx_dict *dict;
	char *action;
	
	
	dict = crtx_event_get_dict(event);
	
	action = crtx_get_string(dict, "ACTION");
	if (!action) {
		CRTX_ERROR("no field ACTION in dict\n");
		crtx_print_dict(dict);
		return 1;
	}
	
	if (!strcmp(action, "add") || !strcmp(action, "initial")) {
		if (c_entry)
			return crtx_cache_update_on_hit(cache, key, event, c_entry);
	} else
	if (!strcmp(action, "remove")) {
		if (c_entry)
			crtx_cache_remove_entry(cache, key);
	} else {
		return 1;
	}
}

static int cache_on_add_cb(struct crtx_cache *cache, struct crtx_dict_item *key, struct crtx_event *event) {
	return cache_update_on_hit(cache, key, event, 0);
}

int main(int argc, char **argv) {
	struct crtx_udev_listener ulist;
	struct crtx_task *cache_task;
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	// load config from libvirt-usb-hotplug.json
	ret = crtx_dict_json_from_file(&cfg_dict, "", "libvirt-usb-hotplug");
	if (ret < 0) {
		printf("no config file found\n");
		return 1;
	}
	
	/*
	 * create the libvirt listener
	 */
	memset(&lvlist, 0, sizeof(struct crtx_libvirt_listener));
	
	if (argc == 1)
		lvlist.hypervisor = "qemu:///system";
	else
		lvlist.hypervisor = argv[1];
	
	ret = crtx_setup_listener("libvirt", &lvlist);
	if (ret) {
		CRTX_ERROR("create_listener(libvirt) failed\n");
		exit(1);
	}
	
	// send state changes also to the normal event graph
	lvlist.base.state_graph = lvlist.base.graph;
	
	// add our callback function
	crtx_create_task(lvlist.base.graph, 0, "libvirt_test", libvirt_event_handler, 0);
	
	// add a retry listener that will restart the libvirt listener every 5 seconds
	// if starting a connection fails. Attach the retry listener to lvlist.parent
	// as it will also receive state changes.
	retry_lstnr = crtx_timer_retry_listener(&lvlist.base, 5);
	
	crtx_start_listener(&lvlist.base);
	
	
	
	/*
	 * create the udev listener
	 */
	
	memset(&ulist, 0, sizeof(struct crtx_udev_listener));
	
	ulist.sys_dev_filter = test_filters;
	ulist.query_existing = 1;
	
	ret = crtx_setup_listener("udev", &ulist);
	if (ret) {
		CRTX_ERROR("create_listener(udev) failed\n");
		exit(1);
	}
	
	crtx_create_task(ulist.base.graph, 10, "udev_test", udev_event_handler, 0);
	
// 	// create a presence cache that will maintain a list of present USB devices
// 	cache_task = crtx_create_presence_cache_task("udev_cache", create_key_action_cb);
// 	cache_task->position = 20;
// 	
// 	// add cache task to the udev event graph
// 	udev_cache_task = (struct crtx_cache_task*) cache_task->userdata;
// 	add_task(ulist.base.graph, cache_task);
	
	{
		ret = crtx_create_cache_task(&cache_task, "udev_cache", create_key_action_cb);
		if (ret) {
			CRTX_ERROR("crtx_create_cache_task failed\n");
			return ret;
		}
		
		udev_cache = (struct crtx_cache*) cache_task->userdata;
		
		udev_cache->on_hit = &cache_update_on_hit;
		udev_cache->on_miss = &crtx_cache_add_on_miss;
		udev_cache->on_add = &cache_on_add_cb;
// 		udev_cache->userdata = ;
		udev_cache->flags = CRTX_CACHE_SIMPLE_LAYOUT;
		
		crtx_add_task(ulist.base.graph, cache_task);
	}
	
	ret = crtx_start_listener(&ulist.base);
	if (ret) {
		CRTX_ERROR("starting udev listener failed\n");
		return 1;
	}
	
	
	crtx_loop();
	
	// cleanup
	crtx_free_cache_task(cache_task);
	crtx_shutdown_listener(&ulist.base);
	
	crtx_finish();
	
	crtx_dict_unref(cfg_dict);
	
	return 0;
}
