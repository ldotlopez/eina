#ifndef _CURL_ENGINE_H
#define _CURL_ENGINE_H

#include <glib.h>
#include <curl/curl.h>

G_BEGIN_DECLS

typedef struct _CurlEngine CurlEngine;
typedef struct _CurlQuery  CurlQuery;
typedef void (*CurlEngineFinishFunc)(CurlEngine *self, CurlQuery *query, gpointer data);

enum {
	CURL_ENGINE_NO_ERROR,
	CURL_ENGINE_GENERAL_ERROR
};

CurlEngine *curl_engine_new();
void        curl_engine_free(CurlEngine *self);

CurlQuery*  curl_engine_query(CurlEngine *self, gchar *uri, CurlEngineFinishFunc finish, gpointer data);
void        curl_engine_cancel(CurlEngine *self, CurlQuery *query);

gchar* curl_query_get_uri(CurlQuery *query);
gboolean curl_query_finish(CurlQuery *query, guint8 **buffer, gsize *size, GError **error);


G_END_DECLS

#endif
