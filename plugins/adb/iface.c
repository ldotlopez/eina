#define GEL_DOMAIN "Eina::Plugin::ADB"
#define EINA_PLUGIN_DATA_TYPE Adb
#include "adb.h"
#include <eina/plugin.h>

static gboolean
adb_plugin_init(EinaPlugin *plugin, GError **error)
{
	
	Adb *self;

	self = adb_new(eina_plugin_get_lomo(plugin), error);
	if (self == NULL)
		return FALSE;
	plugin->data = (gpointer) self;
	return TRUE;
}

static gboolean
adb_plugin_exit(EinaPlugin *plugin, GError **error)
{
	Adb *self = EINA_PLUGIN_DATA(plugin);
	adb_free(self);
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin adb_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("ADB"),
	"0.0.1",
	N_("Audio database"),
	N_("Audio database"),
	NULL,
	"Luis Lopez Lopez <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net/",
	adb_plugin_init, adb_plugin_exit,

	NULL, NULL
};

