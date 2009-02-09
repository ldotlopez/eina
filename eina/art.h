#ifndef _ART_H
#define _ART_H

#include <glib.h>
#include <lomo/player.h>

G_BEGIN_DECLS

typedef struct _Art        Art;
typedef struct _ArtBackend ArtBackend;
typedef struct _ArtSearch  ArtSearch;

typedef void (*ArtFunc)(Art *art, ArtSearch *search, gpointer data);

// --
// Art
// --
Art* art_new(void);
void art_destroy(Art *art);

ArtBackend* art_add_backend(Art *art, ArtFunc search_func, ArtFunc cancel_func, gpointer data);
void        art_remove_backend(Art *art, ArtBackend *backend);

ArtSearch* art_search(Art *art, LomoStream *stream, ArtFunc success_func, ArtFunc fail_func, gpointer pointer);
void       art_cancel(Art *art, ArtSearch *search);

// --
// Backends
// --
/*
ArtBackend* art_backend_new(Art *art, ArtFunc search_func, ArtFunc cancel_func, gpointer data);
void        art_backend_destroy(ArtBackend *backend);
*/
// --
// Searches
// --

G_END_DECLS

#endif
