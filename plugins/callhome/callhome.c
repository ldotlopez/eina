/*
 * plugins/callhome/callhome.c
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

#define GEL_DOMAIN "Eina::Plugin::Callhome"
#define EINA_PLUGIN_NAME "Callhome"
#define PING_URL "http://cuarentaydos.com/uuid.php?q=%s"

#include <config.h>
#include <uuid/uuid.h>
#include <eina/eina-plugin.h>

enum {
	EINA_CALLHOME_NO_ERROR = 0,
	EINA_CALLHOME_ERROR_GENERAL
};

GEL_AUTO_QUARK_FUNC(callhome)

static gchar *
build_uuid(gchar *seed, GError **error)
{
	uuid_t uuid;
	char uuid_str[37] = "\0";

	memset(uuid_str, 0, 37);
	uuid_clear(uuid);

	// If seed, try to import it (verify)
	if (seed)
	{
		uuid_parse(seed, uuid);
		if (!uuid_is_null(uuid))
			return g_strdup(seed);
	}

	// No seed or invalid, generate a new one
	uuid_generate(uuid);
	if (uuid_is_null(uuid))
	{
		g_set_error(error, callhome_quark(), EINA_CALLHOME_ERROR_GENERAL, N_("Cannot create UUID object"));
		return NULL;
	}
	uuid_unparse(uuid, uuid_str);
	return g_strdup(uuid_str);

	// No seed or invalid, generate a new one
	/*
	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_str);
	if (uuid_is_null(uuid))
	{
		g_set_error(error, callhome_quark(), EINA_CALLHOME_ERROR_GENERAL, N_("Cannot create UUID object"));
		return NULL;
	}

	return g_strdup(uuid_str);
	*/
}

static void
cleanup(GelPlugin *plugin)
{
	CurlEngine *engine = (CurlEngine *) gel_plugin_get_data(plugin);
	if (engine)
	{
		gel_run_once_on_idle((GSourceFunc) curl_engine_free, (gpointer) engine, NULL);
		gel_plugin_set_data(plugin, NULL);
	}
}

static void
query_finished(CurlEngine *self, CurlQuery *query, gpointer data)
{
	cleanup((GelPlugin *) data);
}

// --
// Main
// --
G_MODULE_EXPORT gboolean
callhome_plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	gchar *uuid_str = NULL;
	gchar *buffer = NULL;

	// Load and check UUID from configdir
	gchar *uuid_path = g_build_filename(g_get_user_config_dir(), PACKAGE, "callhome-uuid", NULL);
	if (g_file_get_contents(uuid_path, &buffer, NULL, NULL))
	{
		uuid_str = build_uuid(buffer, error);
		g_free(buffer);
	}
	if ((!uuid_str) && ((uuid_str = build_uuid(NULL, error)) == NULL))
	{
		g_free(uuid_path);
		return FALSE;
	}

	gchar *url = g_strdup_printf(PING_URL, uuid_str);
	CurlEngine *engine = curl_engine_new();

	gel_plugin_set_data(plugin, (gpointer) engine);
	curl_engine_query(engine, url, query_finished, plugin);

	g_file_set_contents(uuid_path, uuid_str, -1, NULL);
	g_free(uuid_str);
	g_free(uuid_path);

	return TRUE;
}

G_MODULE_EXPORT gboolean
callhome_plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	cleanup(plugin);
	return TRUE;
}

EINA_PLUGIN_INFO_SPEC(callhome,
	PACKAGE_VERSION, GEL_PLUGIN_NO_DEPS,
	NULL, NULL,

	N_("Help us to follow Eina's usage"),
	N_("Call home will help us to know how many (not who) users has Eina.\n"
	   "This plugins generate one aleatory, unique and anonimous number (an UUID, search Google ) "
	   "and send it to Eina's home. We only track how many different UUID got.\n"
	   "No personal data is transmitted or stored."),
	NULL
);

