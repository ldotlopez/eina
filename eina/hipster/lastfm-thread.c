#include "lastfm-thread.h"
#include <clastfm.h>

G_DEFINE_TYPE (LastFMThread, lastfm_thread, G_TYPE_OBJECT)

struct _LastFMThreadPrivate {
	LASTFM_SESSION *sess;
	GQueue         *queue;
	GThread        *worker_thread;
	GMutex *sess_mutex, *queue_mutex, *subthread_mutex;

};

typedef struct {
	LastFMThread *self;

	LastFMThreadMethodCall *call;

	GCallback      callback;
	gpointer       user_data;
	GDestroyNotify notify;
} ThreadFuncData;

gpointer _lastfm_thread_worker(LastFMThread *self);

ThreadFuncData* thread_data_create  (LastFMThread *self, const LastFMThreadMethodCall *call, GCallback callback, gpointer user_data, GDestroyNotify notify);
void            thread_call_destroy (ThreadFuncData *call);

static void
lastfm_thread_dispose (GObject *object)
{
	LastFMThread *self = LASTFM_THREAD(object);
	LastFMThreadPrivate *priv = self->priv;

	if (!g_queue_is_empty(priv->queue))
	{
		g_mutex_lock(priv->queue_mutex);
		// Destroy packed cmd
		// g_queue_foreach(...)
		g_queue_clear(priv->queue);
		g_mutex_unlock(priv->queue_mutex);

		GMutex *m = priv->queue_mutex; // not sure why I'm doing this, but sounds good
		priv->queue_mutex  = NULL;
		g_mutex_free(m);
	}

	if (priv->sess)
	{
		g_mutex_lock(priv->sess_mutex);
		LASTFM_dinit(priv->sess);
		priv->sess = NULL;
		g_mutex_unlock(priv->sess_mutex);

		GMutex *m = priv->sess_mutex; // not sure why I'm doing this, but sounds good
		priv->sess_mutex = NULL;
		g_mutex_free(m);
	}

	G_OBJECT_CLASS (lastfm_thread_parent_class)->dispose (object);
}

static void
lastfm_thread_class_init (LastFMThreadClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LastFMThreadPrivate));

	object_class->dispose = lastfm_thread_dispose;
}

static void
lastfm_thread_init (LastFMThread *self)
{
	LastFMThreadPrivate *priv = self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), LASTFM_TYPE_THREAD, LastFMThreadPrivate));
	priv->sess_mutex      = g_mutex_new();
	priv->queue_mutex     = g_mutex_new();
	priv->subthread_mutex = g_mutex_new();
	priv->queue = g_queue_new();
}

LastFMThread*
lastfm_thread_new (void)
{
	return g_object_new (LASTFM_TYPE_THREAD, NULL);
}

void
lastfm_thread_call (LastFMThread *self, const LastFMThreadMethodCall *call)
{
	lastfm_thread_call_full (self, call, NULL, NULL, NULL);
}

void
lastfm_thread_call_full (LastFMThread *self, const LastFMThreadMethodCall *call,
	GCallback callback, gpointer user_data, GDestroyNotify notify)
{
	g_return_if_fail (LASTFM_IS_THREAD (self));
	LastFMThreadPrivate *priv = self->priv;

	ThreadFuncData *packed_call = thread_data_create(self, call, callback, user_data, notify);

	g_mutex_lock      (priv->queue_mutex);
	g_queue_push_tail (priv->queue, packed_call);
	g_mutex_unlock    (priv->queue_mutex);

	g_mutex_lock(priv->subthread_mutex);
	if (!priv->worker_thread)
		priv->worker_thread = g_thread_create((GThreadFunc) _lastfm_thread_worker, self, FALSE, NULL);
	g_mutex_unlock(priv->subthread_mutex);
}

gpointer
_lastfm_thread_worker(LastFMThread *self)
{
	g_return_val_if_fail (LASTFM_IS_THREAD (self), NULL);
	LastFMThreadPrivate *priv = self->priv;

	g_mutex_lock(priv->queue_mutex);
	while (!g_queue_is_empty(priv->queue))
	{
		ThreadFuncData *tdata = g_queue_pop_head(priv->queue);
		g_mutex_unlock(priv->queue_mutex);

		LastFMThreadMethodCall *call = tdata->call;

		if (g_str_equal("init", call->method_name))
		{
			if (!call->api_key || !call->api_secret)
				g_warn_if_fail(call->api_key && call->api_secret);
			else
			{
				g_mutex_lock(priv->sess_mutex);
				if (priv->sess)
					LASTFM_dinit(priv->sess);
				g_debug("Init (%s:%s)", call->api_key, call->api_secret);
				priv->sess = LASTFM_init(call->api_key, call->api_secret);
				g_mutex_unlock(priv->sess_mutex);

			}
		}

		else if (g_str_equal("dinit", call->method_name))
		{
			g_mutex_lock(priv->sess_mutex);
			if (priv->sess)
			{
				LASTFM_dinit(priv->sess);
				priv->sess = NULL;
			}
			g_mutex_unlock(priv->sess_mutex);
		}

		else if (g_str_equal("login", call->method_name))
		{
			g_mutex_lock(priv->sess_mutex);
			if (!priv->sess || !call->username || !call->password)
			{
				g_warn_if_fail(priv->sess);
				g_warn_if_fail(call->username && call->password);
			}
			else
			{
				g_debug("Login (as %s): code=%d, status=%s",
					call->username,
					LASTFM_login(priv->sess, call->username, call->password),
					LASTFM_status(priv->sess));
			}
			g_mutex_unlock(priv->sess_mutex);
		}

		else if (g_str_equal("track_scrobble", call->method_name))
		{
			g_mutex_lock(priv->sess_mutex);
			g_debug("track_scrobble: code:=%d, status=%s",
				LASTFM_track_scrobble(priv->sess,
					call->title, call->artist, call->album,
					call->start_stamp, call->length,
					0, 0, NULL),
				LASTFM_status(priv->sess));
			g_mutex_unlock(priv->sess_mutex);
		}

		else
		{
			g_debug("Unknow method called: %s", tdata->call->method_name);
		}
		thread_call_destroy(tdata);
	}
	g_mutex_unlock(priv->queue_mutex);

	g_mutex_lock(priv->subthread_mutex);
	priv->worker_thread = NULL;
	g_mutex_unlock(priv->subthread_mutex);

	return NULL;
}

ThreadFuncData*
thread_data_create(LastFMThread *self, const LastFMThreadMethodCall *call,
	GCallback callback, gpointer user_data, GDestroyNotify notify)
{
	ThreadFuncData *ret = g_new0(ThreadFuncData, 1);

	ret->self      = self;
	ret->callback  = callback;
	ret->user_data = user_data;
	ret->notify    = notify;

	ret->call = g_new0(LastFMThreadMethodCall, 1);

	#define _copy_member(x) do { ret->call->x = (call->x ? g_strdup(call->x) : NULL); } while(0)

	_copy_member(method_name);
	_copy_member(api_key);
	_copy_member(api_secret);
	_copy_member(username);
	_copy_member(password);
	_copy_member(title);
	_copy_member(artist);
	_copy_member(album);

	ret->call->start_stamp = call->start_stamp;
	ret->call->length      = call->length;
	return ret;
}

void
thread_call_destroy(ThreadFuncData *tdata)
{
	#define _free_str_member(x) do { if (tdata->call->x) g_free(tdata->call->x); } while(0)

	_free_str_member (method_name);
	_free_str_member (api_key);
	_free_str_member (api_secret);
	_free_str_member (username);
	_free_str_member (password);
	_free_str_member (title);
	_free_str_member (artist);
	_free_str_member (album);
	g_free(tdata->call);
	g_free(tdata);
}
