
#include <stdlib.h>

#include "llist.h"

char CRTX_DLL_FCT(append)(CRTX_DLL_TYPE **head, CRTX_DLL_TYPE *item) {
	CRTX_DLL_TYPE *it;
	
	if (!head)
		return 0;
	
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
	
	return 1;
}

CRTX_DLL_TYPE * CRTX_DLL_FCT(append_new)(CRTX_DLL_TYPE **head, void *data) {
	CRTX_DLL_TYPE *it;
	
	it = (CRTX_DLL_TYPE*) malloc(sizeof(CRTX_DLL_TYPE));
	it->data = data;
	
	if (!CRTX_DLL_FCT(append)(head, it)) {
		free(it);
		return 0;
	}
	
	return it;
}