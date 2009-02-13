#include "curl-engine.h"

#include <stdio.h>
#include <glib/gprintf.h>
// #define ce_debug(...) g_printf(__VA_ARGS__);
#define ce_debug(...) ;

struct _CurlEngine {
	CURLM *curlm;
	guint id;
	GList *querys;
};

struct _CurlQuery {
	CURL *curl;
	gchar *uri;
	GByteArray *ba;

	CURLMsg *msg;
	CurlEngineFinishFunc finish;

	gpointer data;
};

static gboolean
curl_engine_timeout_cb(CurlEngine *self);

static CurlQuery *
curl_query_new(gchar *uri, CurlEngineFinishFunc finish, gpointer data);
static void
curl_query_free(CurlQuery *query);
static size_t
curl_query_write_cb(void *buffer, size_t size, size_t nmemb, CurlQuery *query);

GQuark
curl_engine_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("curl-engine-quark");
	return ret;
}

CurlEngine*
curl_engine_new(void)
{
	CurlEngine *self = g_new0(CurlEngine, 1);
	self->curlm = curl_multi_init();

	return self;
}

void
curl_engine_free(CurlEngine *self)
{
	g_source_remove(self->id);

	GList *iter = self->querys;
	while (iter)
	{
		curl_multi_remove_handle(self->curlm, ((CurlQuery *) iter->data)->curl);
		curl_query_free((CurlQuery *) iter->data);

		iter = iter->next;
	}

	curl_multi_cleanup(self->curlm);

	g_free(self);
}


CurlQuery *
curl_engine_query(CurlEngine *self, gchar *url, CurlEngineFinishFunc finish, gpointer data)
{
	CurlQuery *query = curl_query_new(url, finish, data);
	self->querys = g_list_prepend(self->querys, query);

	curl_multi_add_handle(self->curlm, query->curl);

	if (self->id == 0)
		self->id = g_timeout_add(100, (GSourceFunc) curl_engine_timeout_cb, self);

	return query;
}

void
curl_engine_cancel(CurlEngine *self, CurlQuery *query)
{
	curl_multi_remove_handle(self->curlm, query->curl);
	self->querys = g_list_remove(self->querys, query);
	curl_query_free(query);
}

static CurlQuery*
curl_engine_find_matching_query(CurlEngine *self, CURL *handle)
{
	GList *iter = self->querys;
	while (iter)
	{
		if (((CurlQuery *)iter->data)->curl ==handle)
			return (CurlQuery *)iter->data;
		iter = iter->next;
	}
	return NULL;
}

static gboolean
curl_engine_timeout_cb(CurlEngine *self)
{
	gint running;
	while (curl_multi_perform(self->curlm, &running) == CURLM_CALL_MULTI_PERFORM)
		;
	ce_debug("%d running\n", running);

	CURLMsg *msg;
	int msgs;
	while ((msg = curl_multi_info_read(self->curlm, &msgs)) != NULL)
	{
		CurlQuery *q = curl_engine_find_matching_query(self, msg->easy_handle);
		if (q == NULL)
		{
			ce_debug("Cannot find CurlQuery\n");
			continue;
		}


		switch(msg->msg)
		{
		case CURLMSG_DONE:
			ce_debug("Got done on %p: %d\n", msg->easy_handle, msg->data.result);
			// Set values
			q->msg = msg;

			// Call finish
			if (q->finish)
				q->finish(self, q, q->data);

			// Cleanup
			self->querys = g_list_remove(self->querys, q);
			curl_multi_remove_handle(self->curlm, msg->easy_handle);
			curl_query_free(q);
			break;

		default:
			ce_debug("Got unknow message type: %d\n", msg->msg);
			break;
		}
	}

	ce_debug("Done\n");
	if (running == 0)
	{
		self->id = 0;
		return FALSE;
	}
	else
		return TRUE;
}

// --
// CurlQuery
// --
static CurlQuery *
curl_query_new(gchar *uri, CurlEngineFinishFunc finish, gpointer data)
{
	CurlQuery *query = g_new0(CurlQuery, 1);
	query->uri = g_strdup(uri);
	query->ba  = g_byte_array_new();

	query->finish = finish;
	query->data = data;

	query->curl = curl_easy_init();
	curl_easy_setopt(query->curl, CURLOPT_URL,           query->uri);
	curl_easy_setopt(query->curl, CURLOPT_WRITEFUNCTION, curl_query_write_cb);
	curl_easy_setopt(query->curl, CURLOPT_WRITEDATA,     query);

	return query;
}

static void
curl_query_free(CurlQuery *query)
{
	g_byte_array_free(query->ba, TRUE);
	g_free(query->uri);
	// curl_easy_cleanup(query->curl);
	g_free(query);
}

gchar* curl_query_get_uri(CurlQuery *query)
{
	return (query->uri ? g_strdup(query->uri) : NULL);
}

gboolean curl_query_finish(CurlQuery *query, guint8 **buffer, gsize *size, GError **error)
{
	if (query->msg->data.result != 0)
	{
		g_set_error(error, curl_engine_quark(), CURL_ENGINE_GENERAL_ERROR, "Unknow error");
		return FALSE;
	}

	if (buffer)
		*buffer = (guint8 *) g_memdup(query->ba->data, query->ba->len);
	if (size)
		*size = query->ba->len;
	return TRUE;
}

static size_t
curl_query_write_cb(void *buffer, size_t size, size_t nmemb, CurlQuery *query)
{
	query->ba = g_byte_array_append(query->ba, buffer, nmemb*size);
	return nmemb*size;
}


