#ifndef _EINA_COVER_IMAGE
#define _EINA_COVER_IMAGE

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_COVER_IMAGE eina_cover_image_get_type()

#define EINA_COVER_IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_COVER_IMAGE, EinaCoverImage))

#define EINA_COVER_IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_COVER_IMAGE, EinaCoverImageClass))

#define EINA_IS_COVER_IMAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_COVER_IMAGE))

#define EINA_IS_COVER_IMAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_COVER_IMAGE))

#define EINA_COVER_IMAGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_COVER_IMAGE, EinaCoverImageClass))

typedef struct {
  GtkDrawingArea parent;
} EinaCoverImage;

typedef struct {
  GtkDrawingAreaClass parent_class;
} EinaCoverImageClass;

GType eina_cover_image_get_type (void);

EinaCoverImage* eina_cover_image_new (void);
void            eina_cover_image_set_from_pixbuf(EinaCoverImage *self, GdkPixbuf *pixbuf);

void     eina_cover_image_set_asis(EinaCoverImage *self, gboolean asis);
gboolean eina_cover_image_get_asis(EinaCoverImage *self);

G_END_DECLS

#endif /* _EINA_COVER_IMAGE */
