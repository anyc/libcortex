/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdlib.h>
#include <errno.h>

#include "llist.h"

int CRTX_DLL_FCT(append)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item) {
	CRTX_DLL_TYPE *it;
	
	if (!head)
		return -EINVAL;
	
	for (it = *head; it && it->next; it=it->next) {}
	
	if (!it) {
		*head = item;
		#ifdef CRTX_DLL
		item->prev = 0;
		#endif
	} else {
		it->next = item;
		#ifdef CRTX_DLL
		item->prev = it;
		#endif
	}
	item->next = 0;
	
	return 0;
}

int CRTX_DLL_FCT(unlink)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item) {
	#ifdef CRTX_DLL
	if (*head == item) {
		*head = item->next;
		if (item->next)
			item->next->prev = item->prev;
	} else {
		if (item->prev)
			item->prev->next = item->next;
		if (item->next)
			item->next->prev = item->prev;
	}
	
	item->prev = 0;
	item->next = 0;
	#else
	if (*head == item) {
		*head = item->next;
	} else {
		CRTX_DLL_TYPE *i;
		
		for (i=*head; i && i->next != item; i = i->next) {}
		if (i) {
			i->next = item->next;
		} else {
// 			VDBG("item not found in llist\n");
			return -ENOENT;
		}
	}
	item->next = 0;
	#endif
	
	return 0;
}

CRTX_DLL_TYPE *CRTX_DLL_FCT(unlink_data)(CRTX_DLL_TYPE **head, void *data) {
	CRTX_DLL_TYPE *i;
	
	for (i=*head; i && i->data != data; i = i->next) {}
	if (i) {
// 		return (CRTX_DLL_FCT(unlink)(head, i) == 0);
		CRTX_DLL_FCT(unlink)(head, i);
		return i;
	} else {
// 		printf("unlink_data called with an unknown item\n");
		return 0;
	}
}

CRTX_DLL_TYPE * CRTX_DLL_FCT(append_new)(CRTX_DLL_TYPE **head, void *data) {
	CRTX_DLL_TYPE *it;
	int r;
	
	it = (CRTX_DLL_TYPE*) malloc(sizeof(CRTX_DLL_TYPE));
	it->data = data;
	
	r = CRTX_DLL_FCT(append)(head, it);
	if (r) {
		free(it);
		return 0;
	}
	
	return it;
}

#ifdef CRTX_DLL
int CRTX_DLL_FCT(insert)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item, CRTX_DLL_TYPE *before) {
	if (!head)
		return -EINVAL;
	
	if (!before->prev) {
		*head = item;
		item->prev = 0;
		item->next = before;
		before->prev = item;
	} else {
		before->prev->next = item;
		item->prev = before->prev;
		before->prev = item;
		item->next = before;
	}
	
	return 0;
}


CRTX_DLL_TYPE * CRTX_DLL_FCT(insert_new)(CRTX_DLL_TYPE **head, void *data, CRTX_DLL_TYPE *before) {
	CRTX_DLL_TYPE *it;
	int r;
	
	it = (CRTX_DLL_TYPE*) malloc(sizeof(CRTX_DLL_TYPE));
	it->data = data;
	
	r = CRTX_DLL_FCT(insert)(head, it, before);
	if (r) {
		free(it);
		return 0;
	}
	
	return it;
}
#endif
