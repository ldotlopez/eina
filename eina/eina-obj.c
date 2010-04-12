/*
 * eina/eina-obj.c
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

#define GEL_DOMAIN "Eina::Obj"

#include <glib/gi18n.h>
#include <gel/gel-ui.h>
#include <eina/eina-obj.h>

gboolean eina_obj_init
(EinaObj *self, GelPlugin *plugin, gchar *name, EinaObjFlag flags, GError **error)
{
	if (!gel_app_shared_set(gel_plugin_get_app(plugin), name, (gpointer) self))
		return FALSE;

	self->plugin = plugin;
	self->name   = g_strdup(name);
	self->lomo   = gel_app_get_lomo(gel_plugin_get_app(plugin));

	if (flags & EINA_OBJ_GTK_UI)
	{
		gchar *n = g_strconcat(name, ".ui", NULL);
		gchar *ui_path = gel_plugin_get_resource(plugin, GEL_RESOURCE_UI, n);
		g_free(n);

		if (!ui_path)
		{
			eina_obj_fini(self);
			return FALSE;
		}

		self->ui = gtk_builder_new();
		if (!gtk_builder_add_from_file(self->ui, ui_path, error))
		{
			g_free(ui_path);
			eina_obj_fini(self);
			return FALSE;
		}
		g_free(ui_path);
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
		
	gel_app_shared_unregister(eina_obj_get_app(self), self->name);
	eina_obj_strip(self, EINA_OBJ_ALL);
	gel_free_and_invalidate(self->name,  NULL, g_free);
	gel_free_and_invalidate(self,        NULL, g_free);
}

gboolean
eina_obj_load_default_ui(EinaObj *self, GError **error)
{
	gchar *res = g_strconcat(self->name, ".ui", NULL);
	gboolean ret = eina_obj_load_ui(self, res, error);
	g_free(res);
	return ret;
}

gboolean
eina_obj_load_ui(EinaObj *self, gchar *resource, GError **error)
{
	g_return_val_if_fail(self->plugin != NULL, FALSE);

	gchar *pathname = gel_plugin_get_resource(self->plugin, GEL_RESOURCE_UI, resource);
	g_return_val_if_fail(pathname != NULL, FALSE);

	gel_free_and_invalidate(self->ui, NULL, g_object_unref);
	self->ui = gtk_builder_new();

	GError *err = NULL;
	if (!gtk_builder_add_from_file(self->ui, pathname, &err))
	{
		g_free(pathname);
		return FALSE;
	}

	return TRUE;
}

gboolean
eina_obj_load_objects_from_file(EinaObj *self, gchar *filename, gchar **objects, GError **error)
{
	if (self->ui == NULL)
		self->ui = gtk_builder_new();
	return (gtk_builder_add_objects_from_file(self->ui, filename, objects, error) != 0);
}

gboolean
eina_obj_load_objects_from_resource(EinaObj *self, gchar *resource, gchar **objects, GError **error)
{
	gchar *filename = gel_plugin_get_resource(self->plugin, GEL_RESOURCE_UI, resource);
	if (filename == NULL)
	{
		g_set_error(error, g_quark_from_static_string("eina-obj"), 1, N_("Resource not found: %s"), resource);
		return FALSE;
	}
	gboolean ret = eina_obj_load_objects_from_file(self, filename, objects, error);
	g_free(filename);
	return ret;
}

gboolean
eina_obj_load_objects_from_string(EinaObj *self, gchar *string, gint len, gchar **objects, GError **error)
{
	if (self->ui == NULL)
		self->ui = gtk_builder_new();
	return (gtk_builder_add_objects_from_string(self->ui, string, len, objects, error) != 0);
}


void
eina_obj_strip(EinaObj *self, EinaObjFlag flags)
{
	if ((flags & EINA_OBJ_GTK_UI) && self->ui)
	{
		g_object_unref(eina_obj_get_ui(self));
		self->ui = NULL;
	}
}
