#include <eina/eina-plugin2.h>
#include "lastfm-priv.h"

gboolean
lastfm_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaLastFMData *data = g_new0(EinaLastFMData, 1);
	gel_plugin_set_data(plugin, data);

	return TRUE;
}

gboolean
lastfm_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}
