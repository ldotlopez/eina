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

gboolean eina_obj_init
(EinaObj *self, GelPlugin *plugin, gchar *name, EinaObjFlag flags, GError **error)
{
	if (!gel_app_shared_set(gel_plugin_get_app(plugin), name, (gpointer) self))
		return FALSE;

	self->plugin = plugin;
	self->name   = g_strdup(name);
	self->lomo   = GEL_APP_GET_LOMO(gel_plugin_get_app(plugin));

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
		
	gel_app_shared_unregister(eina_obj_get_app(self), self->name);
	gel_free_and_invalidate(self->ui,    NULL, g_object_unref);
	gel_free_and_invalidate(self->name,  NULL, g_free);
	gel_free_and_invalidate(self,        NULL, g_free);
}

