/*
 * eina/ext/curl-engine.h
 *
 * Copyright (C) 2004-2011 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CURL_ENGINE_H
#define _CURL_ENGINE_H

#include <glib.h>
#include <curl/curl.h>

G_BEGIN_DECLS

typedef struct _CurlEngine CurlEngine;
typedef struct _CurlQuery  CurlQuery;
typedef void (*CurlEngineFinishFunc)(CurlEngine *self, CurlQuery *query, gpointer data);

typedef enum {
	CURL_ENGINE_NO_ERROR,
	CURL_ENGINE_GENERAL_ERROR
} CurlEngineError;

CurlEngine* curl_engine_new(void);
CurlEngine* curl_engine_get_default(void);
void        curl_engine_free(CurlEngine *self);

CurlQuery*  curl_engine_query(CurlEngine *self, gchar *uri, CurlEngineFinishFunc finish, gpointer data);
void        curl_engine_cancel(CurlEngine *self, CurlQuery *query);

gchar*   curl_query_get_uri(CurlQuery *query);

gboolean curl_query_finish(CurlQuery *query, guint8 **buffer, gsize *size, GError **error);

G_END_DECLS

#endif // _CURL_ENGINE_H

