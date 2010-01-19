/*
 * eina/window.c
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

#define GEL_DOMAIN "Eina::Window"
#include <config.h>
#include <gel/gel.h>
#include <eina/eina-plugin.h>
#include <eina/about.h>
#define CODENAME "They don't believe"

// GEL_AUTO_QUARK_FUNC(about)

struct _EinaAbout {
	EinaObj parent;
	GtkWidget *about;
};

static void
about_response_cb(GtkWidget *w, gint response, EinaAbout *self);

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
	gchar *authors[] = {
		"xuzo <xuzo@cuarentaydos.com>",
		NULL
	};
	gchar *license = 
	"Eina is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n"
	"\n"
	"Eina is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License along with Eina; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.";

	GdkPixbuf *logo_pb = NULL;
	gchar *logo_path = gel_plugin_get_resource(eina_obj_get_plugin((EinaObj *) self), GEL_RESOURCE_IMAGE, "eina.svg");
	if (!logo_path)
		gel_warn(N_("Missing resource: %s"), "eina.svg");
	else
	{
		logo_pb = gdk_pixbuf_new_from_file_at_size(logo_path, 192, 192, NULL);
		g_free(logo_path);
	}

	gchar *comments = g_strconcat("(" CODENAME ")\n\n", N_("A classic player for the modern era"), NULL);
	self->about = gtk_about_dialog_new();
	g_object_set((GObject *) self->about,
		"artists", artists,
		"authors", authors,
		"program-name", "Eina",
		"copyright", "Copyright © 2003-2010 xuzo\nCopyright © 2003-2005 xuzo and eru",
		"comments", comments,
		"website", EINA_PLUGIN_GENERIC_URL,
		"license", license,
		"version", PACKAGE_VERSION,
		"wrap-license", TRUE,
		"logo", logo_pb,
		"title", N_("About Eina"),
		NULL);
	g_free(comments);

	g_signal_connect(self->about, "response", (GCallback) about_response_cb, self);

	gtk_widget_show(self->about);
	return (GtkAboutDialog *) self->about;
}

static void
about_response_cb(GtkWidget *w, gint response, EinaAbout *self)
{
	gtk_widget_destroy(w);
	self->about = NULL;
}

EINA_PLUGIN_SPEC(about,
	PACKAGE_VERSION,
	GEL_PLUGIN_NO_DEPS,
	NULL,
	NULL,
	N_("About window plugin"),
	NULL,
	NULL,
	about_init,
	about_fini);
