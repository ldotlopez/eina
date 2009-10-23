/*
 * eina/window.c
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

#define GEL_DOMAIN "Eina::Window"
#include <config.h>
#include <gel/gel.h>
#include <eina/eina-plugin.h>
#include <eina/about.h>

// GEL_AUTO_QUARK_FUNC(about)

struct _EinaAbout {
	EinaObj parent;
	GtkWidget *about;
};

static gboolean
about_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaAbout *self = g_new0(EinaAbout, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "about", EINA_OBJ_NONE, error))
	{
		g_free(self);
		return FALSE;
	}
	plugin->data = self;
	return TRUE;
}

static gboolean
about_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaAbout *self = (EinaAbout *) plugin->data;
	if (self->about != NULL)
		gtk_widget_destroy(self->about);

	return TRUE;
}

GtkAboutDialog *
eina_about_show(EinaAbout *self)
{
	if (self->about && GTK_IS_ABOUT_DIALOG(self->about))
	{
		gtk_widget_show(self->about);
		return (GtkAboutDialog *) self->about;
	}
	
	gchar *artists[] = {
		"Logo: Pau Tomas <pautomas@gmail.com>",
		"Website and some artwork: Jacobo Tarragon <tarragon@aditel.org>",
		NULL
	};

	self->about = gtk_about_dialog_new();
	g_object_set((GObject *) self->about,
		"artists", artists,
		NULL);
	gtk_widget_show(self->about);
	return (GtkAboutDialog *) self->about;
}

EINA_PLUGIN_SPEC(about,
	PACKAGE_VERSION,
	NULL,
	NULL,
	NULL,
	N_("About window plugin"),
	NULL,
	NULL,
	about_init,
	about_fini);
