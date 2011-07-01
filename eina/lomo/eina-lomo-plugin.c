/*
 * eina/lomo/eina-lomo-plugin.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-lomo-plugin.h"

GEL_DEFINE_QUARK_FUNC(eina_lomo_plugin)
EINA_DEFINE_EXTENSION(EinaLomoPlugin, eina_lomo_plugin, EINA_TYPE_LOMO_PLUGIN)

static void     schedule_save_playlist(LomoPlayer *lomo);
static gboolean save_playlist         (LomoPlayer *lomo);

static guint timeout_id = 0;

static gboolean
eina_lomo_plugin_activate (EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	LomoPlayer *lomo = NULL;
	gint    *argc = eina_application_get_argc(application);
	gchar ***argv = eina_application_get_argv(application);

	lomo_init(argc, argv);
	if ((lomo = lomo_player_new("audio-output", "autoaudiosink", NULL)) == NULL)
	{
		g_set_error(error, eina_lomo_plugin_quark(),
			EINA_LOMO_PLUGIN_ERROR_CANNOT_CREATE_ENGINE, N_("Cannot create engine"));
		return FALSE;
	}

	if (!eina_application_set_interface(application, "lomo", lomo))
	{
		g_set_error(error, eina_lomo_plugin_quark(),
			EINA_LOMO_PLUGIN_ERROR_CANNOT_SET_SHARED, N_("Cannot share engine"));
		g_object_unref(lomo);
		return FALSE;
	}
	g_object_ref_sink(lomo);

	static gchar *props[] = {
		EINA_LOMO_VOLUME_KEY,
		EINA_LOMO_MUTE_KEY,
		EINA_LOMO_REPEAT_KEY,
		EINA_LOMO_RANDOM_KEY,
		EINA_LOMO_AUTO_PARSE_KEY,
		EINA_LOMO_AUTO_PLAY_KEY,
		EINA_LOMO_GAPLESS_MODE_KEY
		};

	GSettings *settings = eina_application_get_settings(application, EINA_LOMO_PREFERENCES_DOMAIN);
	for (gint i = 0; i < G_N_ELEMENTS(props); i++)
		g_settings_bind(settings, props[i], lomo, props[i], G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(lomo, "insert", (GCallback) schedule_save_playlist, NULL);
	g_signal_connect(lomo, "clear",  (GCallback) save_playlist, NULL);

	return TRUE;
}

static gboolean
eina_lomo_plugin_deactivate (EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	LomoPlayer *lomo = eina_application_steal_interface(application, "lomo");
	if (!lomo)
	{
		g_set_error(error, eina_lomo_plugin_quark(),
			EINA_LOMO_PLUGIN_ERROR_CANNOT_DESTROY_ENGINE, N_("Cannot destroy engine"));
		return FALSE;
	}

	save_playlist(lomo);

	g_object_unref(G_OBJECT(lomo));

	return TRUE;
}

/**
 * eina_application_get_lomo:
 * @application: An #EinaApplication
 *
 * Gets the #LomoPlayer object from an #EinaApplication
 *
 * Returns: (transfer none): A #LomoPlayer
 */
LomoPlayer*
eina_application_get_lomo(EinaApplication *application)
{
	return LOMO_PLAYER(eina_application_get_interface(application, "lomo"));
}

/**
 * schedule_save_playlist:
 * @lomo: A #LomoPlayer
 *
 * Schedules save_playlist after one second
 */
static void
schedule_save_playlist(LomoPlayer *lomo)
{
	if (timeout_id)
		g_source_remove(timeout_id);
	timeout_id = g_timeout_add_seconds(1, (GSourceFunc) save_playlist, lomo);
}

/**
 * save_playlist:
 * @lomo: A #LomoPlayer
 *
 * Saves playlist in ~/XDG_CONFIG_DIR/PACKAGE/playlist
 *
 * Returns: %FALSE
 */
static gboolean
save_playlist(LomoPlayer *lomo)
{
	timeout_id = 0;

	g_return_val_if_fail(LOMO_IS_PLAYER(lomo), FALSE);

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
			g_string_free(gs, TRUE);
			return FALSE;
		}
	}

	GError *err = NULL;
	if (!output || !g_file_set_contents(output, gs->str, -1, &err))
		g_warning(N_("Unble to save playlist: %s"), err ? err->message : N_("Missing parent directory"));

	g_string_free(gs, TRUE);

	return FALSE;
}


