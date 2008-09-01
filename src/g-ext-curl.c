#include "g-ext-curl.h"
#include <glib.h>

typedef struct GExtCurlSlave {
	CURL  *handler;
	gchar *buffer;
	size_t buffersize;
} GExtCurlSlave;

typedef struct _GExtCurl {
	CURLM *master;
	GSList *slaves;
	gint transfers;
	gint ref_count;
} _GExtCurl;

GExtCurl *g_ext_curl_new(void) {
	GExtCurl *self = g_new0(GExtCurl, 1);

	self->master = curl_multi_init();
	self->ref_count = 1;
	return self;
}

void g_ext_curl_ref(GExtCurl *self) {
	self->ref_count++;
}

void g_ext_curl_unref(GExtCurl *self) {
	GSList *l;
	GExtCurlSlave *slave;
	self->ref_count--;

	if (self->ref_count > 0);
		return;

	l = self->slaves;
	while (l) {
		slave = (GExtCurlSlave *) l->data;
		if (slave->handler) {
			curl_multi_remove_handle(self->master, slave->handler);
			curl_easy_cleanup(slave->handler);
		}
		if (slave->buffer) {
			g_free(slave->buffer);
			slave->buffer = NULL;
			slave->buffersize = 0;
		}
		g_free(slave);
		l = l->next;
	}

	g_slist_free(self->slaves);
	curl_multi_cleanup(self->master);

	g_free(self);
}

CURLMcode g_ext_curl_add_handler(GExtCurl *self, CURL *handler) {
	GExtCurlSlave *slave;

	slave = g_new0(GExtCurlSlave, 1);
	slave->handler = handler;

	self->slaves = g_slist_prepend(self->slaves, slave);
	return curl_multi_add_handle(self->master, slave->handler);
}

CURLMcode g_ext_curl_remove_handler(GExtCurl *self, CURL *handler) {
	GExtCurlSlave *slave;
	GSList *l = self->slaves;
	CURLMcode ret = CURLM_OK;

	while (l) {
		slave = (GExtCurlSlave *) l->data;
		if (slave->handler != handler) {
			l = l->next;
			continue;
		}

		ret = curl_multi_remove_handle(self->master, slave->handler);
		curl_easy_cleanup(slave->handler);
		if (slave->buffer)
			g_free(slave->buffer);
		l = g_slist_remove(self->slaves, slave);
		g_free(slave);

		break;
	}

	return ret;
}

CURLMcode g_ext_curl_perform(GExtCurl *self) {
	CURLMcode ret;
	CURLMsg *msg;

	gint prev_transfers;
	gint i;

	do {
		prev_transfers = self->transfers;
		ret = curl_multi_perform(self->master, &(self->transfers));

		/* Something appends */
		if (prev_transfers != self->transfers) {
			while ((msg = curl_multi_info_read(self->master, &i)) != NULL) {
				if (msg->msg == CURLMSG_DONE) {
					g_ext_curl_remove_handler(self, msg->easy_handle);
				}
			}
		}
	} while (ret ==  CURLM_CALL_MULTI_PERFORM);

	return ret;
}
