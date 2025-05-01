/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// no preprocessor guard as linkedlist.h will include this header twice

/*
 * user code can create a specialized linked-list implementation by including
 * the llist.c file similar to the following code:
 * 
 * #undef CRTX_DLL_PREFIX
 * #define CRTX_DLL_CUSTOM_UNION_TYPE struct my_app_type *my_value
 * #define CRTX_DLL_PREFIX my_app_ll
 * #include <crtx/llist.c>
 * #undef CRTX_DLL_PREFIX
 * 
 * And then call the functions like this, for example:
 * 
 * my_app_ll_append_new(&my_list_head, my_value);
 */

#ifndef CRTX_DLL_PREFIX
	#ifdef CRTX_DLL
		#define CRTX_DLL_PREFIX crtx_dll
	#else
		#ifndef CRTX_DLL_PREFIX
		#define CRTX_DLL_PREFIX crtx_ll
		#else
		#define STOP
		#endif
	#endif
#endif

#ifndef STOP

#define CRTX_DLL_TYPE struct CRTX_DLL_PREFIX

#define CRTX_DLL_FCT_HELPER2(x,y) x ## _ ## y
#define CRTX_DLL_FCT_HELPER(x,y) CRTX_DLL_FCT_HELPER2(x,y)
#define CRTX_DLL_FCT(x) CRTX_DLL_FCT_HELPER(CRTX_DLL_PREFIX, x)

CRTX_DLL_TYPE {
	#ifdef CRTX_DLL
	CRTX_DLL_TYPE *prev;
	#endif
	CRTX_DLL_TYPE *next;
	
	union {
		void *data;
		size_t size;
		struct crtx_event *event;
		struct crtx_graph *graph;
		struct crtx_inotify_listener *in_listener;
		struct crtx_transform_dict_handler *transform_handler;
		struct crtx_handler_category *handler_category;
		struct crtx_handler_category_entry *handler_category_entry;
		struct crtx_dict *dict;
		
		#ifdef CRTX_DLL_CUSTOM_UNION_TYPE
		CRTX_DLL_CUSTOM_UNION_TYPE;
		#endif
	};
	
	int payload[0];
};

int CRTX_DLL_FCT(append)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item);
CRTX_DLL_TYPE * CRTX_DLL_FCT(append_new)(CRTX_DLL_TYPE **head, void *data);

int CRTX_DLL_FCT(unlink)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item);
CRTX_DLL_TYPE *CRTX_DLL_FCT(unlink_data)(CRTX_DLL_TYPE **head, void *data);
int CRTX_DLL_FCT(unlink_data2)(CRTX_DLL_TYPE **head, void *data);

int CRTX_DLL_FCT(insert)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item, CRTX_DLL_TYPE *before);
CRTX_DLL_TYPE * CRTX_DLL_FCT(insert_new)(CRTX_DLL_TYPE **head, void *data, CRTX_DLL_TYPE *before);

// #undef CRTX_DLL_PREFIX
#endif
