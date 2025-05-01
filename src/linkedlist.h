#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "llist.h"

#undef CRTX_DLL_PREFIX
#undef CRTX_DLL_TYPE
#undef CRTX_DLL_FCT

#define CRTX_DLL
#include "llist.h"

#ifdef __cplusplus
}
#endif

#endif
