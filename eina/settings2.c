#define GEL_DOMAIN "Eina::Settings"

#include <config.h>
#include <eina/settings.h>
#include <eina/class-conf-file.h>

static gboolean
settings_init(GelPlugin *plugin, GError **error)
{
	EinaConf *conf = eina_conf_new();

	gchar *out = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "settings", NULL);
	eina_conf_set_filename(conf, out);
	g_free(out);

	if (!gel_app_shared_set(gel_plugin_get_app(plugin), "settings", conf))
	{
		g_object_unref(conf);
		return FALSE;
	}

	return TRUE;
}

static gboolean
settings_fini(GelPlugin *plugin, GError **error)
{
	GelApp   *app  = gel_plugin_get_app(plugin);
	EinaConf *conf = gel_app_shared_get(app, "settings");

	if (conf == NULL)
		return FALSE;

	gel_app_shared_unregister(app, "settings");
	g_object_unref(conf);

	return TRUE;
}

G_MODULE_EXPORT GelPlugin settings_plugin = {
	GEL_PLUGIN_SERIAL,
	"settings", PACKAGE_VERSION,
	N_("Build-in settings plugin"), NULL,
	NULL, NULL, NULL,
	settings_init, settings_fini,
	NULL, NULL
};

