#ifndef INTERN_H
#define INTERN_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define CRTX_SILENT 0
#define CRTX_ERR 1
#define CRTX_INFO 2
#define CRTX_DBG 3
#define CRTX_VDBG 4

#define CRTX_SUCCESS 0
#define CRTX_ERROR -1

#define INFO(fmt, ...) do { crtx_printf(CRTX_INFO, fmt, ##__VA_ARGS__); } while (0)

#ifndef DEVDEBUG
#define DBG(fmt, ...) do { crtx_printf(CRTX_DBG, fmt, ##__VA_ARGS__); } while (0)
#define VDBG(fmt, ...) do { crtx_printf(CRTX_VDBG, fmt, ##__VA_ARGS__); } while (0)
#else
#define DBG(fmt, ...) do { crtx_printf(CRTX_DBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define VDBG(fmt, ...) do { crtx_printf(CRTX_VDBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#endif

#ifndef SEGV_ON_ERROR
#define ERROR(fmt, ...) do { crtx_printf(CRTX_ERR, "\x1b[31m"); crtx_printf(CRTX_ERR, fmt, ##__VA_ARGS__); crtx_printf(CRTX_ERR, "\x1b[0m"); } while (0)
#else
#define ERROR(fmt, ...) do { crtx_printf(CRTX_ERR, "\x1b[31m"); crtx_printf(CRTX_ERR, fmt, ##__VA_ARGS__); crtx_printf(CRTX_ERR, "\x1b[0m"); char *d = 0; printf("%c", *d); } while (0)
#endif

#define ASSERT(x) do { if (!(x)) { ERROR(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } } while (0)

#define ASSERT_STR(x, msg, ...) do { if (!(x)) { ERROR(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } } while (0)

#define POPCOUNT32(x) __builtin_popcount(x);

#endif
