#define GEL_DOMAIN "Eina::Settings"

#include <glib.h>
#include <gel/gel.h>
#include "config.h"
#include "base.h"
#include "settings.h"
#include "class-conf-file.h"

gboolean _settings_load(EinaConf *conf);

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean settings_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaConf *conf;
	gchar *out;

	if ((conf = eina_conf_new()) == FALSE)
		return FALSE;

	out = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "settings", NULL);
	eina_conf_set_filename(conf, out);
	g_free(out);

	if (!gel_hub_shared_set(hub, "settings", conf))
		goto fail;

	_settings_load(conf);
	// Wait until some idle to load settings
	// _idle_add((GSourceFunc) _settings_load, conf);
	return TRUE;

fail:
	g_object_unref(conf);
	return FALSE;
}

G_MODULE_EXPORT gboolean settings_exit
(gpointer data)
{
	EinaConf *conf = (EinaConf *) data;
	g_object_unref(conf);
	return TRUE;
}

/* Wrap eina_conf_load to return FALSE */
gboolean _settings_load(EinaConf *conf) {
	eina_conf_load(conf);
	return FALSE;
}

G_MODULE_EXPORT GelHubSlave settings_connector = {
	"settings",
	&settings_init,
	&settings_exit
};

