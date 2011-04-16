#ifndef _EINA_COVER_CLUTTER
#define _EINA_COVER_CLUTTER

#include <glib-object.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_COVER_CLUTTER eina_cover_clutter_get_type()

#define EINA_COVER_CLUTTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_COVER_CLUTTER, EinaCoverClutter))
#define EINA_COVER_CLUTTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_COVER_CLUTTER, EinaCoverClutterClass))
#define EINA_IS_COVER_CLUTTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_COVER_CLUTTER))
#define EINA_IS_COVER_CLUTTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_COVER_CLUTTER))
#define EINA_COVER_CLUTTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_COVER_CLUTTER, EinaCoverClutterClass))

typedef struct _EinaCoverClutterPrivate EinaCoverClutterPrivate;
typedef struct {
	GtkClutterEmbed parent;
	EinaCoverClutterPrivate *priv;
} EinaCoverClutter;

typedef struct {
	GtkClutterEmbedClass parent_class;
} EinaCoverClutterClass;

GType eina_cover_clutter_get_type (void);

EinaCoverClutter* eina_cover_clutter_new (void);
void              eina_cover_clutter_set_cover(EinaCoverClutter *self, GdkPixbuf *pixbuf);

G_END_DECLS

#endif
