#ifndef _EINA_ARTWORK
#define _EINA_ARTWORK

#include <gtk/gtk.h>
#include <lomo/stream.h>

G_BEGIN_DECLS

#define EINA_TYPE_ARTWORK eina_artwork_get_type()

#define EINA_ARTWORK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ARTWORK, EinaArtwork))

#define EINA_ARTWORK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ARTWORK, EinaArtworkClass))

#define EINA_IS_ARTWORK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ARTWORK))

#define EINA_IS_ARTWORK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ARTWORK))

#define EINA_ARTWORK_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ARTWORK, EinaArtworkClass))

typedef struct {
	GtkImage parent;
} EinaArtwork;

typedef struct {
	GtkImageClass parent_class;
	void (*change) (EinaArtwork *self);
} EinaArtworkClass;

typedef void (*EinaArtworkProviderSearchFunc) (EinaArtwork *self, LomoStream *stream, gpointer data);
typedef void (*EinaArtworkProviderCancelFunc) (EinaArtwork *self, gpointer data);

GType eina_artwork_get_type (void);

EinaArtwork* eina_artwork_new (void);

void
eina_artwork_set_stream(EinaArtwork *self, LomoStream *stream);
LomoStream *
eina_artwork_get_stream(EinaArtwork *self);

gboolean
eina_artwork_add_provider(EinaArtwork *self,
	const gchar *name,
	EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel,
	gpointer provider_data);
gboolean
eina_artwork_remove_provider(EinaArtwork *self, const gchar *name);

void
eina_artwork_provider_success(EinaArtwork *self, GType type, gpointer data);
void
eina_artwork_provider_fail(EinaArtwork *self);

#ifdef EINA_COMPILATION
typedef struct _EinaArtworkProvider EinaArtworkProvider;

EinaArtworkProvider *
eina_artwork_provider_new(const gchar *name, EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel, gpointer provider_data);
void
eina_artwork_provider_free(EinaArtworkProvider *self);

void
eina_artwork_provider_search(EinaArtworkProvider *self, EinaArtwork *artwork);
void
eina_artwork_provider_cancel(EinaArtworkProvider *self, EinaArtwork *artwork);

GList *
eina_artwork_find_provider(EinaArtwork *self, const gchar *name);

#define eina_artwork_have_provider(self,name) (!eina_artwork_find_provider(self,name))

#define eina_artwork_find_provider_data(self,name) \
	(eina_artwork_have_provider(self,name) ? ((EinaArtworkProvider *) eina_artwork_find_provider(self,name)->data) : NULL)

#endif

G_END_DECLS

#endif /* _EINA_ARTWORK */
