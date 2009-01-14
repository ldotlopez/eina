#include <config.h>

#define GEL_DOMAIN "Eina::Plugin::Recently"
#define EINA_PLUGIN_DATA_TYPE "Recently"
#define EINA_PLUGIN_NAME "Recently"

#include <eina/eina-plugin.h>
#include "plugins/adb/adb.h"

gboolean
recently_plugin_init(EinaPlugin *plugin, GError **error)
{
	GelApp *app = eina_plugin_get_app(plugin);
	Adb *adb = gel_app_shared_get(app, "adb");
	gel_warn("ADB object retrieved: %p", adb);

	return TRUE;
}

gboolean
recently_plugin_fini(EinaPlugin *plugin, GError **error)
{
	return TRUE;
}

EinaPlugin recently2_plugin = {
	EINA_PLUGIN_SERIAL,
	"recently", PACKAGE_VERSION,
	N_("Stores your recent playlists"),
	N_("Recently plugin 2. ADB based version"), // long desc
	NULL, // icon
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	recently_plugin_init, recently_plugin_fini,
	NULL, NULL
};
