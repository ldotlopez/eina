#ifndef _EINA_ART_SEARCH
#define _EINA_ART_SEARCH

#include <glib-object.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_SEARCH eina_art_search_get_type()

#define EINA_ART_SEARCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_SEARCH, EinaArtSearch))

#define EINA_ART_SEARCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART_SEARCH, EinaArtSearchClass))

#define EINA_IS_ART_SEARCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_SEARCH))

#define EINA_IS_ART_SEARCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART_SEARCH))

#define EINA_ART_SEARCH_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART_SEARCH, EinaArtSearchClass))

typedef struct {
	GObject parent;
} EinaArtSearch;

typedef struct {
	GObjectClass parent_class;
} EinaArtSearchClass;

GType eina_art_search_get_type (void);

typedef void (*EinaArtSearchCallback) (EinaArtSearch *search, gpointer data);
EinaArtSearch* eina_art_search_new (LomoStream *stream, EinaArtSearchCallback callback, gpointer data);

const gchar*
eina_art_search_stringify(EinaArtSearch *search);

void
eina_art_search_set_domain(EinaArtSearch *search, GObject *domain);
GObject *
eina_art_search_get_domain(EinaArtSearch *search);

void
eina_art_search_set_bpointer(EinaArtSearch *search, gpointer bpointer);
gpointer
eina_art_search_get_bpointer(EinaArtSearch *search);

gpointer
eina_art_search_get_result(EinaArtSearch *search);
void
eina_art_search_set_result(EinaArtSearch *search, gpointer result);

void
eina_art_search_run_callback(EinaArtSearch *search);



G_END_DECLS

#endif /* _EINA_ART_SEARCH */
