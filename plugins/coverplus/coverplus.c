#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#include <eina/iface.h>

EINA_PLUGIN_FUNC gboolean
coverplus_exit(EinaPlugin *self)
{
	eina_iface_info("Plugin hooktest unloaded");
	eina_plugin_free(self);
	return TRUE;
}

EINA_PLUGIN_FUNC EinaPlugin*
coverplus_init(GelHub *app, EinaIFace *iface)
{
	EinaPlugin *self = eina_plugin_new(iface,
		"coverplus", "cover", NULL, coverplus_exit,
		NULL, NULL, NULL);
	
	return self;
}

