#ifndef _CRTX_CURL_H
#define _CRTX_CURL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <curl/curl.h>

#include "core.h"

struct crtx_curl_listener;

typedef int crtx_CURLOPT_XFERINFOFUNCTION(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
typedef int crtx_CURLOPT_PROGRESSFUNCTION(void *userdata, double dltotal, double dlnow, double ultotal, double ulnow);
typedef size_t crtx_CURLOPT_WRITEFUNCTION(char *ptr, size_t size, size_t nmemb, void *userdata);
typedef size_t crtx_CURLOPT_HEADERFUNCTION(char *buffer, size_t size, size_t nitems, void *userdata);
typedef int crtx_curl_done_cb(void *userdata, struct CURLMsg *msg, long response_code, long error_code);

struct crtx_curl_listener {
	struct crtx_listener_base base;
	
	CURL *easy;
	char *url;
	char error[CURL_ERROR_SIZE];
	
	#if LIBCURL_VERSION_NUM >= 0x072000 // 0.7.32
	crtx_CURLOPT_XFERINFOFUNCTION *progress_callback;
	#else
	crtx_CURLOPT_PROGRESSFUNCTION *progress_callback;
	#endif
	void *progress_cb_data;
	crtx_CURLOPT_HEADERFUNCTION *header_callback;
	void *header_cb_data;
	crtx_CURLOPT_WRITEFUNCTION *write_callback;
	void *write_cb_data;
	FILE *write_file_ptr;
	int write_fd;
	
	crtx_curl_done_cb *done_callback;
	void *done_cb_data;
};

#define CRTX_EVENT_TYPE_FAMILY_CURL (500)
#define CRTX_CURL_ET_FINISHED (CRTX_EVENT_TYPE_FAMILY_CURL + 1)
#define CRTX_CURL_ET_PROGRESS (CRTX_EVENT_TYPE_FAMILY_CURL + 2)
#define CRTX_CURL_ET_HEADER (CRTX_EVENT_TYPE_FAMILY_CURL + 3)

int crtx_curl_progress_callback(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
size_t crtx_curl_header_callback(char *buffer, size_t size, size_t nitems, void *userdata);

struct crtx_listener_base *crtx_new_curl_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(curl)

void crtx_curl_init();
void crtx_curl_finish();

#endif
