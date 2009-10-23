/*
 * eina/lomo.c
 *
 * Copyright (C) 2004-2009 Eina
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

static void
lomo_mute_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);
static void
lomo_repeat_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);
static void
lomo_random_cb(LomoPlayer *lomo, gboolean value, EinaConf *conf);

GEL_AUTO_QUARK_FUNC(lomo)

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
		return FALSE;
	}

	EinaConf *conf = gel_app_get_settings(app);
	lomo_player_set_volume(engine, eina_conf_get_int (conf, "/core/volume", 50   ));
	lomo_player_set_mute  (engine, eina_conf_get_bool(conf, "/core/mute",   FALSE));
	lomo_player_set_repeat(engine, eina_conf_get_bool(conf, "/core/repeat", FALSE));
	lomo_player_set_repeat(engine, eina_conf_get_bool(conf, "/core/repeat", FALSE));

	g_signal_connect(engine, "mute",   (GCallback) lomo_mute_cb,   conf);
	g_signal_connect(engine, "repeat", (GCallback) lomo_repeat_cb, conf);
	g_signal_connect(engine, "random", (GCallback) lomo_random_cb, conf);

	gel_implement("Set volume, random, shuffle,...");
	return TRUE;
}

gboolean
lomo_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	LomoPlayer *engine = gel_app_get_lomo(app);

	if ((engine == NULL) || !gel_app_shared_unregister(app, "lomo"))
	{
		g_set_error(error, lomo_quark(), EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE, N_("Cannot destroy engine"));
		return FALSE;
	}

	EinaConf *conf = gel_app_get_settings(app);
	eina_conf_set_int(conf, "/core/volume", lomo_player_get_volume(engine));
	g_object_unref(G_OBJECT(engine));

	return TRUE;
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

EINA_PLUGIN_SPEC(lomo,
	PACKAGE_VERSION,			// version
	"settings",		            // deps
	NULL,						// author
	NULL,						// url

	N_("Build-in lomo plugin"),	// short
	NULL,						// long
	NULL,						// icon

	lomo_plugin_init,			// init
	lomo_plugin_fini			// fini
);

