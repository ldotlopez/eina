#ifndef _EINA_PLUGIN_LASTFM_H
#define _EINA_PLUGIN_LASTFM_H

#include <glib.h>
G_BEGIN_DECLS

#define GEL_DOMAIN "Eina::Plugin::LastFM"
#define EINA_PLUGIN_NAME "LastFM"
#define EINA_PLUGIN_DATA_TYPE LastFM

#include <eina/plugin.h>
#include "submit.h"

typedef struct _LastFM LastFM;
struct _LastFM {
	GtkWidget *configuration_widget;
	gchar *daemon_path;
	gchar *client_path;

	// Subplugins
	struct _LastFMSubmit *submit;
};

gboolean
lastfm_init(EinaPlugin *self, GError **error);

gboolean
lastfm_exit(EinaPlugin *self, GError **error);

G_END_DECLS

#endif
