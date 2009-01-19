/*
 * eina/eina-obj.c
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

#define GEL_DOMAIN "Eina::Obj"

#include <glib/gi18n.h>
#include <gel/gel-ui.h>
#include <eina/eina-obj.h>

static GQuark
eina_obj_quark(void)
{
	GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-obj");
	return ret;
}

enum {
	EINA_OBJ_NO_ERROR = 0,
	EINA_OBJ_PLUGIN_NOT_FOUND,
	EINA_OBJ_SHARED_NOT_FOUND,
	EINA_OBJ_CANNOT_LOAD_UI
};

gboolean eina_obj_init
(EinaObj *self, GelApp *app, gchar *name, EinaObjFlag flags, GError **error)
{
	if (!gel_app_shared_set(app, name, (gpointer) self))
		return FALSE;

	self->name = g_strdup(name);
	self->app  = app;
	self->lomo = GEL_APP_GET_LOMO(app);

	if (flags & EINA_OBJ_GTK_UI)
	{
		self->ui = gel_ui_load_resource(self->name, error);
		if (self->ui == NULL)
		{
			eina_obj_fini(self);
			return FALSE;
		}
	}

	return TRUE;
}

void eina_obj_fini(EinaObj *self)
{
	if (self == NULL)
	{
		gel_warn(N_("Trying to free a NULL pointer"));
		return;
	}
		
	gel_app_shared_unregister(self->app, self->name);
	gel_free_and_invalidate(self->ui,    NULL, g_object_unref);
	gel_free_and_invalidate(self->name,  NULL, g_free);
	gel_free_and_invalidate(self,        NULL, g_free);
}

gpointer
eina_obj_require(EinaObj *self, gchar *plugin_name, GError **error)
{
	GelApp *app = eina_obj_get_app(self);

	GelPlugin *plugin = gel_app_load_plugin_by_name(app, plugin_name, error);
	if (plugin == NULL)
		return FALSE;

	gpointer ret = gel_app_shared_get(app, plugin_name);
	if (ret != NULL)
		return ret;

	g_set_error(error, eina_obj_quark(),
		EINA_OBJ_SHARED_NOT_FOUND, N_("Shared mem for %s not found"), plugin_name);

	gel_app_unload_plugin(app, plugin, NULL);
	return FALSE;

#if 0
	// 1. Try to get a reference to plugin and reference+1 it
	GelPlugin *plugin = gel_app_get_plugin_by_name(app, plugin_name);
	if (plugin == NULL)
	{
		if ((plugin = gel_app_query_plugin_by_name(app, plugin_name, error)) == NULL)
			return NULL;
	}
	else
		gel_plugin_ref(plugin);

	// 2. Ensure its init
	GError *err = NULL;
	if (!gel_plugin_is_enabled(plugin) && (!gel_plugin_init(plugin, error)))
	{
		gel_plugin_unref(plugin);
		if ((gel_plugin_get_usage(plugin) == 0) && !gel_plugin_fini(plugin, &err))
		{
				g_warning("Unable to unload plugin %s: %s", gel_plugin_stringify(plugin), err->message);
			g_error_free(err);
		}
	}

	// 3. Retrieve mem
	gpointer ret = gel_app_shared_get(app, plugin_name);
	if (ret != NULL)
		return ret;

	// 4. Handle failure
	g_set_error(error, eina_obj_quark(),
		EINA_OBJ_SHARED_NOT_FOUND, N_("Shared mem for %s not found"), plugin_name);
	eina_obj_unrequire(self, plugin_name, NULL);

	return FALSE;
#endif
}

gboolean
eina_obj_unrequire(EinaObj *self, gchar *plugin_name, GError **error)
{
	 GelApp *app = eina_obj_get_app(self);

	GelPlugin *plugin = gel_app_get_plugin_by_name(app, plugin_name);
	if (plugin == NULL)
		return FALSE;

	if (!gel_app_unload_plugin(app, plugin, error))
		return FALSE;

	return TRUE;
}

