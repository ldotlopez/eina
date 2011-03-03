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

	_plugin->emp = eina_mpris_player_new(eina_application_get_lomo(app), PACKAGE);

	#if HAVE_INDICATE
	_plugin->is = indicate_server_ref_default();
	indicate_server_set_type(_plugin->is, "music.eina");
	indicate_server_set_desktop_file(_plugin->is, "/usr/share/applications/eina.desktop");
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

