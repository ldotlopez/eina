#ifndef _EINA_PLUGIN_COVERPLUS_BANSHEE_H
#define _EINA_PLUGIN_COVERPLUS_BANSHEE_H

#include <glib.h>
#include <lomo/stream.h>
#include <eina/eina-artwork.h>

G_BEGIN_DECLS

void
banshee_search(EinaArtwork *cover, LomoStream *stream, gpointer data);

G_END_DECLS

#endif
