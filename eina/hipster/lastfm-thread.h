
/* lastfm-thread.h */

#ifndef _LASTFM_THREAD
#define _LASTFM_THREAD

#include <glib-object.h>

G_BEGIN_DECLS

#define LASTFM_TYPE_THREAD lastfm_thread_get_type()

#define LASTFM_THREAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LASTFM_TYPE_THREAD, LastFMThread)) 
#define LASTFM_THREAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LASTFM_TYPE_THREAD, LastFMThreadClass)) 
#define LASTFM_IS_THREAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LASTFM_TYPE_THREAD)) 
#define LASTFM_IS_THREAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LASTFM_TYPE_THREAD)) 
#define LASTFM_THREAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LASTFM_TYPE_THREAD, LastFMThreadClass))

typedef struct _LastFMThreadPrivate LastFMThreadPrivate;
typedef struct {
	GObject parent;
	LastFMThreadPrivate *priv;
} LastFMThread;

typedef struct {
	GObjectClass parent_class;
} LastFMThreadClass;

typedef struct {
	gchar *method_name;
	gchar *api_key, *api_secret;
	gchar *username, *password;
	gchar *title, *artist, *album;
	guint64 start_stamp, length;
} LastFMThreadMethodCall;

GType lastfm_thread_get_type (void);

LastFMThread* lastfm_thread_new (void);

void lastfm_thread_call     (LastFMThread *self, const LastFMThreadMethodCall *call);
void lastfm_thread_call_full(LastFMThread *self, const LastFMThreadMethodCall *call,
	GCallback callback, gpointer user_data, GDestroyNotify notify);

G_END_DECLS

#endif /* _LASTFM_THREAD */

