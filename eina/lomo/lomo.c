/*
 * eina/lomo/lomo.c
 *
 * Copyright (C) 2004-2010 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lomo.h"
#include <lomo/lomo-player.h>
#include <lomo/lomo-util.h>
#include <eina/ext/eina-fs.h>
#include <eina/application/application.h>

typedef struct {
	GList *signals;
	GList *handlers;
} LomoEventHandlerGroup;

static void
lomo_random_cb(LomoPlayer *lomo, gboolean value, gpointer data);

GEL_DEFINE_QUARK_FUNC(lomo)

gboolean
lomo_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = NULL;
	gint    *argc = gel_plugin_engine_get_argc(engine);
	gchar ***argv = gel_plugin_engine_get_argv(engine);

	lomo_init(argc, argv);
	if ((lomo = lomo_player_new("audio-output", "autoaudiosink", NULL)) == NULL)
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE, N_("Cannot create engine"));
		return FALSE;
	}

	if (!gel_plugin_engine_set_interface(engine, "lomo", lomo))
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_SET_SHARED, N_("Cannot share engine"));
		g_object_unref(lomo);
		return FALSE;
	}
	g_object_ref_sink(lomo);


	GelUIApplication *application = eina_plugin_get_application(plugin);
	GSettings *settings = gel_ui_application_get_settings(application, EINA_LOMO_PREFERENCES_DOMAIN);

	g_object_set((LomoPlayer *) lomo,
		"repeat", g_settings_get_boolean(settings, EINA_LOMO_REPEAT_KEY),
		"random", g_settings_get_boolean(settings, EINA_LOMO_RANDOM_KEY),
		NULL);

	static gchar *props[] = {
		EINA_LOMO_VOLUME_KEY,
		EINA_LOMO_MUTE_KEY,
		EINA_LOMO_REPEAT_KEY,
		EINA_LOMO_RANDOM_KEY,
		EINA_LOMO_AUTO_PARSE_KEY,
		EINA_LOMO_AUTO_PLAY_KEY,
		NULL };
	for (gint i = 0; props[i] ; i++)
		g_settings_bind(settings, props[i], lomo, props[i], G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(lomo, "random", (GCallback) lomo_random_cb, NULL);

	// Quick hack
	gint    argsc = *argc;
	gchar **argsv = *argv;

	for (guint i = 1; (i <= argsc) && argsv && argsv[i] && argsv[i][0]; i++)
	{
		gchar *uri = lomo_create_uri(argsv[i]);
		g_warning("+ '%s'", uri);
		lomo_player_append_uri(lomo, uri);
	}

	if (lomo_player_get_playlist(lomo) == NULL)
	{
		gchar *output = g_build_filename(g_get_user_config_dir(), PACKAGE, "playlist", NULL);
		if (g_file_test(output, G_FILE_TEST_IS_REGULAR))
		{
			gchar *buffer = NULL;
			GError *err = NULL;
			if (!g_file_get_contents(output, &buffer, NULL, &err))
			{
				g_warning(N_("Unable to recover previous playlist: %s"), err->message);
				g_error_free(err);
			}
			else
			{
				gchar **v = g_uri_list_extract_uris(buffer);
				g_free(buffer);

				for (guint i = 0; v && v[i]; i++)
					lomo_player_append_uri(lomo, v[i]);

				g_strfreev(v);
			}
		}
		gel_free_and_invalidate(output, NULL, g_free);
	}

	return TRUE;
}

gboolean
lomo_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = gel_plugin_engine_get_interface(engine, "lomo");
	if (!lomo)
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE, N_("Cannot destroy engine"));
		return FALSE;
	}
	gel_plugin_engine_set_interface(engine, "lomo", NULL);

	GString *gs = g_string_new(NULL);
	GList *i = (GList *) lomo_player_get_playlist(lomo);
	while (i)
	{
		gs = g_string_append(gs, lomo_stream_get_tag(LOMO_STREAM(i->data), LOMO_TAG_URI));
		gs = g_string_append_c(gs, '\n');
		i = i->next;
	}
	gchar *output = g_build_filename(g_get_user_config_dir(), PACKAGE, "playlist", NULL);
	gchar *bname = g_path_get_dirname(output);

	if (!g_file_test(bname, G_FILE_TEST_IS_DIR))
	{
		if (g_mkdir_with_parents(bname, 0755) == -1)
		{
			g_warning(N_("Can't create dir '%s': %s"), bname, strerror(errno));
			gel_free_and_invalidate(bname,  NULL, g_free);
			gel_free_and_invalidate(output, NULL, g_free);
		}
	}

	GError *err = NULL;
	if (!output || !g_file_set_contents(output, gs->str, -1, &err))
		g_warning(N_("Unble to save playlist: %s"), err ? err->message : N_("Missing parent directory"));
	g_string_free(gs, TRUE);

	g_signal_handlers_disconnect_by_func(lomo, lomo_random_cb, NULL);
	g_object_unref(G_OBJECT(lomo));

	return TRUE;
}

static void
lomo_random_cb(LomoPlayer *lomo, gboolean value, gpointer data)
{
	if (value) lomo_player_randomize(lomo);
}

