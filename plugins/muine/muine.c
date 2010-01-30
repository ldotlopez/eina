/*
 * plugins/muine/muine.c
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

#define GEL_DOMAIN "Eina::Plugin::Muine"
#define GEL_PLUGIN_DATA_TYPE EinaMuine
#include <eina/eina-plugin.h>
#include <plugins/adb/adb.h>

enum {
	EINA_MUINE_NO_ERROR = 0,
	EINA_MUINE_ERROR_UI_NOT_FOUND,
	EINA_MUINE_ERROR_MISSING_OBJECTS
};

enum {
	COMBO_COLUMN_ICON,
	COMBO_COLUMN_MARKUP,
};

struct _EinaMuine {
	EinaObj  parent;

	GtkWidget *dock;

	GtkTreeView *list;
	GtkComboBox *combo;

	GtkListStore *list_model;
	GtkListStore *combo_model;
};
typedef struct _EinaMuine EinaMuine;

GEL_AUTO_QUARK_FUNC(muine)

static gboolean
muine_init(EinaMuine *self, GError **error)
{
	self->dock  = eina_obj_get_typed(self, GTK_WIDGET, "main-widget");
	self->list  = eina_obj_get_typed(self, GTK_TREE_VIEW, "list-view");
	self->combo = eina_obj_get_typed(self, GTK_COMBO_BOX, "combo-view");
	self->list_model  = eina_obj_get_typed(self, GTK_LIST_STORE, "list-model");
	self->combo_model = eina_obj_get_typed(self, GTK_LIST_STORE, "combo-model");

	if (!self->dock || !self->list || !self->combo || !self->list_model || !self->combo_model)
	{
		g_set_error(error, muine_quark(), EINA_MUINE_ERROR_MISSING_OBJECTS,
			N_("Missing widgets D:%p LV:%p CV:%p LS:%p CS:%p"),
			self->dock, self->list, self->combo, self->list_model, self->combo_model);
		return FALSE;
	}

	Adb *adb = eina_obj_get_adb(self);
	// select count(*) as count,artist,album from fast_meta group
	// by(lower(album));
	gchar *q = "select count(*) as count,artist,album from fast_meta group by(lower(album)) order by album";
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(adb->db, q, -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot select a fake stream using query %s", q);
		sqlite3_free(q);
	}

	gchar *album, *artist;
	gint   count;
	while (stmt && (sqlite3_step(stmt) == SQLITE_ROW))
	{
		count  = (gint)   sqlite3_column_int (stmt, 0);
		artist = g_markup_escape_text((gchar*) sqlite3_column_text(stmt, 1), -1);
		album  = g_markup_escape_text((gchar*) sqlite3_column_text(stmt, 2), -1);

		gchar *markup = g_strdup_printf("<big>%s</big>\n%s - %d streams", album, artist, count);
		gtk_list_store_insert_with_values(self->combo_model, NULL, 0,
			COMBO_COLUMN_MARKUP, markup,
			-1);
		g_free(markup);
		g_free(artist);
		g_free(album);
	}
    sqlite3_finalize(stmt);


	gtk_widget_unparent(self->dock);
	gtk_widget_show(self->dock);

	EinaDock *dock = eina_obj_get_dock(self);
	eina_dock_add_widget(dock, "muine", gtk_label_new(N_("M")), self->dock);
	return TRUE;
}

static gboolean
muine_destroy(EinaMuine *self, GError **error)
{
	if (self && self->dock)
	{
		EinaDock *dock = eina_obj_get_dock(self);
		eina_dock_remove_widget(dock, "muine");
	}
	return TRUE;
}

G_MODULE_EXPORT gboolean
muine_iface_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaMuine *self;

	self = g_new0(EinaMuine, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "muine", EINA_OBJ_GTK_UI, error))
		return FALSE;

	/*
	gchar *ui_path = NULL;
	gchar *ui_xml  = NULL;

	// This snippet is very common, it should move to some common place
	if (!(ui_path = gel_plugin_get_resource(EINA_OBJ(self)->plugin, GEL_RESOURCE_UI, "muine.ui")) ||
	    !(g_file_get_contents(ui_path, &ui_xml, NULL, error)))
	{
		if (ui_path == NULL)
		{
			g_set_error(error, muine_quark(), EINA_MUINE_ERROR_UI_NOT_FOUND, N_("Resource not found: %s"), "muine.ui");
			return FALSE;
		}
		else
			g_free(ui_path);
	}
	*/

	plugin->data = self;
	gel_warn("Muine is on fire");

	if (!muine_init(self, error))
	{
		eina_obj_fini((EinaObj *) self);
		return FALSE;
	}

	return TRUE;
}

G_MODULE_EXPORT gboolean
muine_iface_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaMuine *self = GEL_PLUGIN_DATA(plugin);
	gel_warn("Muine is... out");

	gboolean ret = muine_destroy(self, error);
	if (ret == FALSE)
		return FALSE;
	
	eina_obj_fini((EinaObj *) self);
	return TRUE;
}

EINA_PLUGIN_SPEC(muine,
	"0.0.1",
	"dock,adb",

	NULL,
	NULL,

	N_("Muine-like playlist"),
	NULL,
	NULL,

	muine_iface_init,
	muine_iface_fini
);

