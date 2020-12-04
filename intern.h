#ifndef INTERN_H
#define INTERN_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#define CRTX_STRINGIFYB(x) #x
#define CRTX_STRINGIFY(x) CRTX_STRINGIFYB(x)

#define CRTX_VLEVEL_SILENT 0
#define CRTX_VLEVEL_ERR 1
#define CRTX_VLEVEL_INFO 2
#define CRTX_VLEVEL_DBG 3
#define CRTX_VLEVEL_VDBG 4

#define CRTX_SUCCESS 0
// #define CRTX_ERROR -1

#define CRTX_INFO(fmt, ...) do { crtx_printf(CRTX_VLEVEL_INFO, fmt, ##__VA_ARGS__); } while (0)

#ifndef CRTX_VDEBUG
	#define CRTX_DBG(fmt, ...) do { crtx_printf(CRTX_VLEVEL_DBG, fmt, ##__VA_ARGS__); } while (0)
	#define CRTX_VDBG(fmt, ...) do { crtx_printf(CRTX_VLEVEL_VDBG, fmt, ##__VA_ARGS__); } while (0)
#else
	#define CRTX_DBG(fmt, ...) do { crtx_printf(CRTX_VLEVEL_DBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
	#define CRTX_VDBG(fmt, ...) do { crtx_printf(CRTX_VLEVEL_VDBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#endif

#ifndef SEGV_ON_ERROR
#define CRTX_ERROR(fmt, ...) do { crtx_printf(CRTX_VLEVEL_ERR, "\x1b[31m"); crtx_printf(CRTX_VLEVEL_ERR, fmt, ##__VA_ARGS__); crtx_printf(CRTX_VLEVEL_ERR, "\x1b[0m"); } while (0)
#else
#define CRTX_ERROR(fmt, ...) do { crtx_printf(CRTX_VLEVEL_ERR, "\x1b[31m"); crtx_printf(CRTX_VLEVEL_ERR, fmt, ##__VA_ARGS__); crtx_printf(CRTX_VLEVEL_ERR, "\x1b[0m"); char *d = 0; printf("%c", *d); } while (0)
#endif

#define CRTX_ASSERT(x) do { if (!(x)) { CRTX_ERROR(__FILE__ ":" CRTX_STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } } while (0)

#define CRTX_ASSERT_STR(x, msg, ...) do { if (!(x)) { CRTX_ERROR(__FILE__ ":" CRTX_STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } } while (0)

#if __STDC_VERSION__ >= 201112L
#define CRTX_STATIC_ASSERT(X, msg) _Static_assert(X, msg)
#else
#define CRTX_STATIC_ASSERT(X, msg) ({ extern int __attribute__((error("assertion failed: '" #X "' - " #msg))) crtx_static_assert(); ((X)?0:crtx_static_assert()),0; })
#endif

#define CRTX_POPCOUNT32(x) __builtin_popcount(x)

#define CRTX_DEFINE_ALLOC_FUNCTION(lstnr) \
	struct crtx_ ## lstnr ## _listener * crtx_alloc_ ## lstnr ## _listener() { \
		return calloc(1, sizeof(struct crtx_ ## lstnr ## _listener)); \
	}
	
#ifdef __cplusplus
}
#endif

#endif
