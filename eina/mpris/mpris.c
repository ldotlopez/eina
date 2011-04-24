#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-mpris-player.h"
#if HAVE_INDICATE
#include <libindicate/server.h>
#endif

typedef struct {
	EinaMprisPlayer *emp;
	#if HAVE_INDICATE
	IndicateServer  *is;
	#endif
} MprisPlugin;

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new(MprisPlugin, 1);

	_plugin->emp = eina_mpris_player_new(app, PACKAGE);

	#if HAVE_INDICATE
	_plugin->is = indicate_server_ref_default();

	gchar *server_type = g_strconcat("music.", PACKAGE, NULL);
	indicate_server_set_type(_plugin->is, server_type);
	g_free(server_type);

	gchar *uc_package = g_ascii_strup(PACKAGE, -1);
	gchar *env_package_name = g_strconcat(uc_package, "_DESKTOP_FILE", NULL);
	const gchar *env_desktop_file = g_getenv(env_package_name);
	if (env_desktop_file)
	{
		g_free(uc_package);
		g_free(env_package_name);
		g_warning("Set indicate desktop file to: '%s'", env_desktop_file);
		indicate_server_set_desktop_file(_plugin->is, env_desktop_file);
	}
	else
	{
		gchar *tmp = g_strconcat(PACKAGE, ".desktop", NULL);
		gchar *desktop_file = g_build_filename(PACKAGE_PREFIX, "share", "applications", tmp, NULL);
		g_warning("Set indicate desktop file to: '%s'", desktop_file);
		indicate_server_set_desktop_file(_plugin->is, desktop_file);
		g_free(tmp);
		g_free(desktop_file);
	}
	indicate_server_show(_plugin->is);
	#endif

	gel_plugin_set_data(plugin, _plugin);

	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = gel_plugin_steal_data(plugin);
	gel_free_and_invalidate(_plugin->emp, NULL, g_object_unref);
	#if HAVE_INDICATE
	gel_free_and_invalidate(_plugin->is, NULL, g_object_unref);
	#endif
	g_free(_plugin);
	return TRUE;
}

