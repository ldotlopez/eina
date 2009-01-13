#ifndef _EINA_PLUGIN_COVERPLUS_INFOLDER_H
#define _EINA_PLUGIN_COVERPLUS_INFOLDER_H

#include <glib.h>
#include <lomo/stream.h>
#include <eina/eina-plugin.h>
#include <eina/eina-artwork.h>

G_BEGIN_DECLS

typedef struct _CoverPlusInfolder CoverPlusInfolder;

CoverPlusInfolder*
coverplus_infolder_new(EinaPlugin *plugin, GError **error);
void
coverplus_infolder_destroy(CoverPlusInfolder *self);

void
coverplus_infolder_search_cb(EinaArtwork *artwork, LomoStream *stream, CoverPlusInfolder *self);
void
coverplus_infolder_cancel_cb(EinaArtwork *artwork, CoverPlusInfolder *self);

G_END_DECLS

#endif
