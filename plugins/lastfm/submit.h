#ifndef _EINA_PLUGIN_LASTFM_SUBMIT_H
#define _EINA_PLUGIN_LASTFM_SUBMIT_H

#include <glib.h>

G_BEGIN_DECLS

#include <eina/plugin.h>
#include "lastfm.h"

typedef struct _LastFMSubmit LastFMSubmit;

gboolean
lastfm_submit_init(EinaPlugin *plugin, GError **error);
gboolean
lastfm_submit_exit(EinaPlugin *plugin, GError **error);

G_END_DECLS

#endif

