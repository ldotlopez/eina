#include "fieshta-stream.h"
#include <glib/gprintf.h>
#include <clutter-gtk/gtk-clutter-util.h>
G_DEFINE_TYPE (FieshtaStream, fieshta_stream, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamPrivate))

typedef struct _FieshtaStreamPrivate FieshtaStreamPrivate;

struct _FieshtaStreamPrivate {
    int dummy;
};

static void
fieshta_stream_dispose (GObject *object)
{
  G_OBJECT_CLASS (fieshta_stream_parent_class)->dispose (object);
}

static void
fieshta_stream_class_init (FieshtaStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (FieshtaStreamPrivate));

  object_class->dispose = fieshta_stream_dispose;
}

static void
fieshta_stream_init (FieshtaStream *self)
{
}

FieshtaStream*
fieshta_stream_new (GdkPixbuf *cover, gchar *title, gchar *artist)
{
	FieshtaStream *self = g_object_new (FIESHTA_STREAM_TYPE_FIESHTA, NULL);
	ClutterColor white = {255, 255, 255, 255};

	self->cover =  gtk_clutter_texture_new_from_pixbuf(cover);
	clutter_actor_set_size(self->cover, 256, 256);
	clutter_actor_set_position(self->cover, 0, 0);

	self->title = clutter_label_new_with_text("Sans Bold 70", title);
	clutter_actor_set_position(self->title, 256, 0);
	clutter_label_set_color((ClutterLabel *) self->title, &white);
	clutter_label_set_ellipsize((ClutterLabel *) self->title, TRUE);

	self->artist = clutter_label_new_with_text("Sans Bold 70", artist);
	clutter_actor_set_position(self->artist, 256, clutter_actor_get_height(self->title));
	clutter_label_set_color((ClutterLabel *) self->artist, &white);
	clutter_label_set_ellipsize((ClutterLabel *) self->artist, TRUE);

	clutter_container_add((ClutterContainer *) self, self->cover, self->title, self->artist, NULL);
 	clutter_actor_set_anchor_point((ClutterActor*) self, 256, 128);

	return self;
}

