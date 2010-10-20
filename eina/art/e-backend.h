#ifndef _EINA_ART_BACKEND
#define _EINA_ART_BACKEND

#include <glib-object.h>
#include <eina/art/e-search.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_BACKEND eina_art_backend_get_type()

#define EINA_ART_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_BACKEND, EinaArtBackend))

#define EINA_ART_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART_BACKEND, EinaArtBackendClass))

#define EINA_IS_ART_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_BACKEND))

#define EINA_IS_ART_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART_BACKEND))

#define EINA_ART_BACKEND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART_BACKEND, EinaArtBackendClass))

typedef struct {
	GObject parent;
} EinaArtBackend;

typedef struct {
	GObjectClass parent_class;
	void (*finish) (EinaArtBackend *backend, EinaArtSearch *search);
} EinaArtBackendClass;

typedef void (*EinaArtBackendFunc) (EinaArtBackend *backend, EinaArtSearch *search, gpointer backend_data);

GType eina_art_backend_get_type (void);

EinaArtBackend* eina_art_backend_new (gchar *name, EinaArtBackendFunc search, EinaArtBackendFunc cancel, GDestroyNotify notify, gpointer data);
const gchar *
eina_art_backend_get_name(EinaArtBackend *backend);

void
eina_art_backend_run(EinaArtBackend *backend, EinaArtSearch *search);
void
eina_art_backend_finish(EinaArtBackend *backend, EinaArtSearch *search);

G_END_DECLS

#endif /* _EINA_ART_BACKEND */
