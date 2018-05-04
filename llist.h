/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// no preprocessor guard necessary as linkedlist.h will include this header
// twice

#ifdef CRTX_DLL
	#define CRTX_DLL_PREFIX crtx_dll
#else
	#ifndef CRTX_DLL_PREFIX
// 	#undef CRTX_DLL_PREFIX
	#define CRTX_DLL_PREFIX crtx_ll
	#else
	#define STOP
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
	
// 	void *data;
	union {
		void *data;
		size_t size;
		struct crtx_event *event;
		struct crtx_graph *graph;
		struct crtx_inotify_listener *in_listener;
		struct crtx_transform_dict_handler *transform_handler;
		struct crtx_handler_category *handler_category;
		struct crtx_handler_category_entry *handler_category_entry;
	};
	
	int payload[0];
};

char CRTX_DLL_FCT(append)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item);
CRTX_DLL_TYPE * CRTX_DLL_FCT(append_new)(CRTX_DLL_TYPE **head, void *data);
char CRTX_DLL_FCT(unlink)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item);
char CRTX_DLL_FCT(unlink_data)(CRTX_DLL_TYPE **head, void *data);

// #undef CRTX_DLL_PREFIX
#endif
