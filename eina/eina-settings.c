/*
 * eina/eina-settings.c
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
#include <eina/eina-settings.h>

static GQuark
settings_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-settings");
	return ret;
}

static gboolean
settings_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	// Build paths
	gchar *cfg_file = NULL;
	const gchar *config_dir = g_get_user_config_dir();
	if (config_dir == NULL)
		cfg_file = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "settings", NULL);
	else
		cfg_file = g_build_filename(config_dir, PACKAGE_NAME, "settings", NULL);
	gchar *dirname = g_path_get_dirname(cfg_file);

	// Create folder structure
	if (g_mkdir_with_parents(dirname, 0755) == -1)
	{
		// XXX: Add errno info
		g_set_error(error, settings_quark(), EINA_SETTINGS_CANNOT_CREATE_CONFIG_DIR,
			N_("Cannot create config folder %s"), dirname);
		g_free(dirname);
		g_free(cfg_file);
		return FALSE;
	}
	g_free(dirname);

	// Load settings
	EinaConf *conf = eina_conf_new();
	eina_conf_set_source(conf, cfg_file);
	g_free(cfg_file);
	eina_conf_load(conf);

	// Insert into GelApp
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
	if ((conf == NULL) || !EINA_IS_CONF_CLASS(conf))
	{
		g_set_error(error, settings_quark(), EINA_SETTINGS_CONF_OBJECT_NOT_FOUND,
			N_("Conf object not found"));
		return FALSE;
	}

	gel_app_shared_unregister(app, "settings");
	g_object_unref(conf);

	return TRUE;
}

G_MODULE_EXPORT GelPlugin settings_plugin = {
	GEL_PLUGIN_SERIAL,
	"settings", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in settings plugin"), NULL, NULL,

	settings_init, settings_fini,

	NULL, NULL, NULL
};

