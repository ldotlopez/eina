#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define EINA_PLUGIN_DATA_TYPE "Recently"
#define EINA_PLUGIN_NAME "Recently"

#include <eina/plugin.h>
#include "plugins/adb/adb.h"

gboolean
recently_init(EinaPlugin *plugin, GError **error)
{
	GelHub *hub = eina_plugin_get_hub(plugin);
	Adb *adb = gel_hub_shared_get(hub, "adb");
	gel_warn("ADB object retrieved: %p", adb);

	return TRUE;
}

gboolean
recently_fini(EinaPlugin *plugin, GError **error)
{
	return TRUE;
}

EinaPlugin recently2_plugin = {
	EINA_PLUGIN_SERIAL,
	"Recently",
	"0.0.0",
	"Stores your recent playlists",
	"Recently plugin 2. ADB based version", // long desc
	NULL, // icon
	EINA_PLUGIN_GENERIC_AUTHOR,
	EINA_PLUGIN_GENERIC_URL,
	"Adb",
	recently_init,
	recently_fini,
	NULL, NULL
};
