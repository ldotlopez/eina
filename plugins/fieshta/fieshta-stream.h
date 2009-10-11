#ifndef _FIESHTA_STREAM
#define _FIESHTA_STREAM

#include <glib-object.h>
#include <clutter/clutter.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define FIESHTA_STREAM_TYPE_FIESHTA fieshta_stream_get_type()

#define FIESHTA_STREAM_FIESHTA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStream))

#define FIESHTA_STREAM_FIESHTA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamClass))

#define FIESHTA_STREAM_IS_FIESHTA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIESHTA_STREAM_TYPE_FIESHTA))

#define FIESHTA_STREAM_IS_FIESHTA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FIESHTA_STREAM_TYPE_FIESHTA))

#define FIESHTA_STREAM_FIESHTA_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamClass))

typedef struct {
  ClutterStage parent;
  ClutterActor *cover, *title, *artist;
} FieshtaStream;

typedef struct {
  ClutterStageClass parent_class;
} FieshtaStreamClass;

GType fieshta_stream_get_type (void);

FieshtaStream* fieshta_stream_new (GdkPixbuf *cover, gchar *title, gchar *artist);

G_END_DECLS

#endif /* _FIESHTA_STREAM */
