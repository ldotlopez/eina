/*
 * eina/lomo.c
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

#define GEL_DOMAIN "Eina::Lomo"

#include <config.h>
#include <eina/eina-plugin.h>
#include <eina/lomo.h>

typedef struct {
	GList *signals;
	GList *handlers;
} LomoEventHandlerGroup;

static void
lomo_mute_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);
static void
lomo_repeat_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);
static void
lomo_random_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);
static void
conf_change_cb(EinaConf *conf, gchar *key, LomoPlayer *engine);

GEL_DEFINE_QUARK_FUNC(lomo)

gboolean
lomo_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	LomoPlayer *engine = NULL;

	if ((engine = lomo_player_new("audio-output", "autoaudiosink", NULL)) == NULL)
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE, N_("Cannot create engine"));
		return FALSE;
	}
	// lomo_player_set_auto_parse(engine, TRUE);


	if (!gel_app_shared_set(gel_plugin_get_app(plugin), "lomo", engine))
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_SET_SHARED, N_("Cannot share engine"));
		g_object_unref(engine);
		return FALSE;
	}
	g_object_ref_sink(engine);

	EinaConf *conf = gel_app_get_settings(app);
	lomo_player_set_volume(engine, eina_conf_get_int (conf, "/core/volume", 50   ));
	lomo_player_set_mute  (engine, eina_conf_get_bool(conf, "/core/mute",   FALSE));
	lomo_player_set_repeat(engine, eina_conf_get_bool(conf, "/core/repeat", FALSE));
	lomo_player_set_random(engine, eina_conf_get_bool(conf, "/core/random", FALSE));

	lomo_player_set_auto_parse(engine, eina_conf_get_bool(conf, "/core/auto-parse", TRUE));
	lomo_player_set_auto_play (engine, eina_conf_get_bool(conf, "/core/auto-play",  FALSE));

	g_signal_connect(conf, "change", (GCallback) conf_change_cb, engine);

	g_signal_connect(engine, "mute",   (GCallback) lomo_mute_cb,   conf);
	g_signal_connect(engine, "repeat", (GCallback) lomo_repeat_cb, conf);
	g_signal_connect(engine, "random", (GCallback) lomo_random_cb, conf);

	return TRUE;
}

gboolean
lomo_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	LomoPlayer *engine = gel_app_get_lomo(app);

	if (engine == NULL)
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE, N_("Cannot destroy engine"));
		return FALSE;
	}

	EinaConf *conf = gel_app_get_settings(app);
	eina_conf_set_int(conf, "/core/volume", lomo_player_get_volume(engine));
	g_signal_handlers_disconnect_by_func(conf, conf_change_cb, engine);

	g_object_unref(G_OBJECT(engine));
	gel_app_shared_free(app, "lomo");

	return TRUE;
}

void
lomo_event_handler_group_free(LomoEventHandlerGroup *group)
{
	g_list_foreach(group->signals, (GFunc) g_free, NULL);
	g_list_free(group->signals);
	g_list_free(group->handlers);
	g_free(group);
}

gpointer
eina_plugin_lomo_add_handlers(EinaPlugin *plugin, ...)
{
	g_warning("This function is unsupported");
	LomoPlayer *lomo = eina_plugin_get_lomo(plugin);
	g_return_val_if_fail(lomo != NULL, NULL);

	GelApp *app = gel_plugin_get_app(plugin);
	GHashTable *handlers = gel_app_shared_get(app, "lomo-event-handlers");
	if (!handlers)
	{
		handlers = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) lomo_event_handler_group_free);
		gel_app_shared_set(app, "lomo-event-handlers", handlers);
	}

	LomoEventHandlerGroup *group = g_new0(LomoEventHandlerGroup, 1);

	va_list p;
	gchar *signal;
	gpointer callback;

	va_start(p, plugin);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
		{
			g_signal_connect(lomo, signal, callback, plugin);
			group->signals  = g_list_prepend(group->signals,  g_strdup(signal));
			group->handlers = g_list_prepend(group->handlers, callback);
		}
		signal = va_arg(p, gchar*);
	}
	va_end(p);

	g_hash_table_insert(handlers, group, group);
	return group;
}

void
eina_plugin_lomo_remove_handlers(EinaPlugin *plugin, gpointer handler_pointer)
{
	g_warning("This function is unsupported");
}

static void
lomo_mute_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf)
{
	eina_conf_set_bool(conf, "/core/mute", value);
}

static void
lomo_repeat_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf)
{
	eina_conf_set_bool(conf, "/core/repeat", value);
}

static void
lomo_random_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf)
{
	eina_conf_set_bool(conf, "/core/random", value);
	if (value)
		lomo_player_randomize(lomo);
}

static void
conf_change_cb(EinaConf *conf, gchar *key, LomoPlayer *engine)
{
	// We need to check current engine values to prevent infinite loop

	if (g_str_equal(key, "/core/random") && (lomo_player_get_random(engine) != eina_conf_get_boolean(conf, key, FALSE)))
		lomo_player_set_random(engine, eina_conf_get_boolean(conf, key, FALSE));

	if (g_str_equal(key, "/core/repeat") && (lomo_player_get_repeat(engine) != eina_conf_get_boolean(conf, key, FALSE)))
		lomo_player_set_repeat(engine, eina_conf_get_boolean(conf, key, FALSE));

	gboolean auto_play_a = lomo_player_get_auto_play(engine);
	gboolean auto_play_b = eina_conf_get_bool(conf, key, TRUE);
	if (g_str_equal(key, "/core/auto-play") && (auto_play_a != auto_play_b))
		lomo_player_set_auto_play(engine, auto_play_b);

	gboolean auto_parse_a = lomo_player_get_auto_parse(engine);
	gboolean auto_parse_b = eina_conf_get_bool(conf, key, TRUE);
	if (g_str_equal(key, "/core/auto-parse") && (auto_parse_a != auto_parse_b))
		lomo_player_set_auto_parse(engine, auto_parse_b);

}

EINA_PLUGIN_INFO_SPEC(lomo,
	PACKAGE_VERSION,			// version
	"settings",		            // deps
	NULL,						// author
	NULL,						// url

	N_("Build-in lomo plugin"),	// short
	NULL,						// long
	NULL						// icon
);

