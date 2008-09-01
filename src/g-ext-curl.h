#ifndef __CURL_H__
#define __CURL_H__

#include <curl/curl.h>

typedef struct _GExtCurl GExtCurl;

GExtCurl *g_ext_curl_new(void);
void g_ext_curl_ref  (GExtCurl *self);
void g_ext_curl_unref(GExtCurl *self);

CURLMcode g_ext_curl_add_handler(GExtCurl *self, CURL *handler);
CURLMcode g_ext_curl_remove_handler(GExtCurl *self, CURL *handler);

CURLMcode g_ext_curl_perform(GExtCurl *self);

#endif
