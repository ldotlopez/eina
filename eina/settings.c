/*
 * eina/settings.c
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

#define GEL_DOMAIN "Eina::Settings"

#define GEL_PLUGIN_DATA_TYPE EinaConf

#include <config.h>
#include <eina/settings.h>
#include <eina/class-conf-file.h>

static gboolean
settings_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaConf *conf = eina_conf_new();

	gchar *out = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "settings", NULL);
	eina_conf_set_filename(conf, out);
	g_free(out);
	eina_conf_load(conf);

	if (!gel_app_shared_set(gel_plugin_get_app(plugin), "settings", conf))
	{
		g_object_unref(conf);
		return FALSE;
	}

	plugin->data = conf;
	return TRUE;
}

static gboolean
settings_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaConf *conf = GEL_PLUGIN_DATA(plugin);
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

