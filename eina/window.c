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
#include <eina/window.h>

GEL_AUTO_QUARK_FUNC(window)

static gboolean
window_delete_event_cb(EinaWindow *window, GdkEvent *ev, GelApp *app);

G_MODULE_EXPORT gboolean
window_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaWindow *window = eina_window_new();
	if (!gel_app_shared_set(app, "window", (gpointer) window))
	{
		g_set_error(error, window_quark(), EINA_WINDOW_ERROR_REGISTER,
			N_("Cannot register EinaWindow object into GelApp"));
		g_object_unref(window);
		return FALSE;
	}

	g_signal_connect(window, "delete-event", (GCallback) window_delete_event_cb, app);
	gtk_widget_show(GTK_WIDGET(window));

	// Due some bug on OSX main window needs to be resizable on creation, but
	// without dock or other attached widgets to main window it must not be
	// resizable.
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);


	return TRUE;
}

G_MODULE_EXPORT gboolean
window_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaWindow *window = gel_app_shared_get(app, "window");
	if (!window || !EINA_IS_WINDOW(window))
	{
		g_set_error(error, window_quark(), EINA_WINDOW_ERROR_NOT_FOUND,
			N_("EinaWindow not found in GelApp"));
		return FALSE;
	}

	gtk_widget_destroy((GtkWidget *) window);
	gel_app_shared_free(app, "window");

	return TRUE;
}

static gboolean
window_delete_event_cb(EinaWindow *window, GdkEvent *ev, GelApp *app)
{
	if (eina_window_get_persistant(window))
		gtk_widget_hide((GtkWidget *) window);
	else
		g_object_unref(app);

	return TRUE;
}

EINA_PLUGIN_INFO_SPEC(window,
	NULL,
	NULL,
	NULL,
	NULL,
	N_("Main window plugin"),
	NULL,
	NULL
	);
