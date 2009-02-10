#ifndef _ART_H
#define _ART_H

#include <glib.h>
#include <lomo/player.h>

G_BEGIN_DECLS

#define GEL_APP_GET_ART(app)  ((Art*) gel_app_shared_get(app, "art"))
#define EINA_OBJ_GET_ART(obj) GEL_APP_GET_ART(EINA_OBJ_GET_APP(obj))

typedef struct _Art        Art;
typedef struct _ArtBackend ArtBackend;
typedef struct _ArtSearch  ArtSearch;

typedef void (*ArtFunc)(Art *art, ArtSearch *search, gpointer data);

// --
// Art
// --
Art* art_new(void);
void art_destroy(Art *art);

ArtBackend* art_add_backend(Art *art, gchar *name, ArtFunc search_func, ArtFunc cancel_func, gpointer data);
void        art_remove_backend(Art *art, ArtBackend *backend);

ArtSearch* art_search(Art *art, LomoStream *stream, ArtFunc success_func, ArtFunc fail_func, gpointer pointer);
void       art_cancel(Art *art, ArtSearch *search);

void art_report_success(Art *art, ArtSearch *search, gpointer result);
void art_report_failure(Art *art, ArtSearch *search);

gpointer art_search_get_result(ArtSearch *search);

G_END_DECLS

#endif
