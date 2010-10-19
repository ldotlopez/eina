#ifndef _EINA_ART
#define _EINA_ART

#include <glib-object.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART eina_art_get_type()

#define EINA_ART(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART, EinaArt))

#define EINA_ART_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART, EinaArtClass))

#define EINA_IS_ART(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART))

#define EINA_IS_ART_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART))

#define EINA_ART_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART, EinaArtClass))

typedef struct {
	GObject parent;
} EinaArt;

typedef struct {
	GObjectClass parent_class;
} EinaArtClass;

typedef struct _EinaArtSearch EinaArtSearch;
typedef struct _EinaArtBackend EinaArtBackend;

typedef void (*EinaArtFunc)(EinaArt *art, EinaArtSearch *search, gpointer data);
#define EINA_ART_FUNC(x) ((EinaArtFunc) x)

GType eina_art_get_type (void);

EinaArt* eina_art_new (void);

EinaArtBackend *
eina_art_add_backend(EinaArt *art, gchar *name, EinaArtFunc search, EinaArtFunc cancel, gpointer data);

EinaArtSearch* eina_art_search(EinaArt *art, LomoStream *stream, EinaArtFunc callback, gpointer pointer);
void       eina_art_cancel(EinaArt *art, EinaArtSearch *search);
gpointer    eina_art_search_get_result(EinaArtSearch *search);

G_END_DECLS

#endif /* _EINA_ART */
