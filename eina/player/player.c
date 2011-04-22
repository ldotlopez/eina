/*
 * eina/player/player.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "player.h"
#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include <eina/dock/dock.h>
#include <eina/preferences/preferences.h>

#define EINA_PLAYER_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.player"
#define EINA_PLAYER_STREAM_MARKUP_KEY  "stream-markup"
#define BUGS_URI ""
#define HELP_URI ""

static void
player_dnd_cb(GtkWidget *w, GType type, const guchar *data, EinaApplication *app);
static gboolean
player_action_activated_cb(EinaPlayer *player, GtkAction *action, EinaApplication *app);
static void 
action_activated_cb(GtkAction *action, EinaPlayer *player);
static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlayer *player);

static const gchar *ui_mng_xml = 
"<menubar name='Main'>"
"  <menu name='File' action='file-menu'>"
"    <menuitem name='Open' action='open-action' position='top'/>"
"  </menu>"
"  <menu name='Help' action='help-menu'>"
"    <menuitem name='Help'  action='help-action'  />"
"    <menuitem name='Bug'   action='bug-action'   />"
"    <menuitem name='About' action='about-action' />"
"  </menu>"
"</menubar>";

static GtkActionEntry ui_mng_actions[] = {
	{ "open-action",  GTK_STOCK_OPEN,  N_("_Add or queue files"), "<control>o", NULL, (GCallback) action_activated_cb },
	{ "help-menu",    NULL,            N_("_Help"),               "<alt>h", NULL, NULL },
	{ "help-action",  GTK_STOCK_HELP,  N_("_Help"),               "F1",     NULL, (GCallback) action_activated_cb },
	{ "bug-action",   EINA_STOCK_BUG,  N_("_Report a bug"),        NULL,    NULL, (GCallback) action_activated_cb },
	{ "about-action", GTK_STOCK_ABOUT, N_("_About"),               NULL,    NULL, (GCallback) action_activated_cb }
};

static EinaDockTab *dock_tab;
static guint __ui_merge_id = 0;
static EinaPreferencesTab *__prefs_tab = NULL;

G_MODULE_EXPORT gboolean
player_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	GSettings *settings = g_settings_new(EINA_PLAYER_PREFERENCES_DOMAIN);

	GtkWidget *player = eina_player_new();
	g_object_set(player,
		"lomo-player",    eina_application_get_interface(app, "lomo"),
		"default-pixbuf", gdk_pixbuf_new_from_file(eina_art_plugin_get_default_cover_path(), NULL),
		"stream-markup",  g_settings_get_string(settings, EINA_PLAYER_STREAM_MARKUP_KEY),
		NULL);
	gel_ui_widget_enable_drop(player, (GCallback) player_dnd_cb, app);

	g_object_set_data((GObject *) player, "eina-application", app);
	g_object_set_data((GObject *) player, "eina-plugin", plugin);

	// Connect actions
	g_signal_connect(player,   "action-activated", (GCallback) player_action_activated_cb, app);
	g_signal_connect(settings, "changed",          (GCallback) settings_changed_cb, player);

	// Export
	eina_application_set_interface(app, "player", player);

	// Pack and show
	g_object_set(G_OBJECT(player),
		"hexpand", TRUE,
		"vexpand", FALSE,
		NULL);
/*
	gtk_box_pack_start (
		(GtkBox *) eina_application_get_window_content_area(app),
		player,
		FALSE, FALSE, 0);
	gtk_widget_show_all(player);
*/

	dock_tab = eina_plugin_add_dock_widget(plugin, "player", player, gtk_label_new(_("Player")), EINA_DOCK_DEFAULT);

	// Attach menus
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
	GError *e = NULL;
	if ((__ui_merge_id = gtk_ui_manager_add_ui_from_string (ui_mng, ui_mng_xml, -1 , &e)) == 0)
	{
		g_warning(N_("Unable to add UI to window GtkUIManager: '%s'"), e->message);
		g_error_free(e);
	}
	GtkActionGroup *ag = gtk_action_group_new("player");
	gtk_action_group_add_actions(ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), player);
	gtk_ui_manager_insert_action_group(ui_mng, ag, G_MAXINT);

	gchar *prefs_ui_file = g_build_filename(gel_plugin_get_data_dir(plugin), "preferences.ui", NULL);
	GtkWidget *widget = gel_ui_generic_new_from_file(prefs_ui_file);
	g_free(prefs_ui_file);

	__prefs_tab = eina_preferences_tab_new();
	g_object_set(G_OBJECT(__prefs_tab),
		"label-text", N_("Player"),
		"widget", widget,
		NULL);
	
	GSettings *lomo_sets = eina_application_get_settings(app, EINA_LOMO_PREFERENCES_DOMAIN);
	eina_preferences_tab_bindv(__prefs_tab,
		lomo_sets, "repeat", "repeat", "active",
		lomo_sets, "random", "random", "active",
		lomo_sets, "auto-play", "auto-play", "active",
		lomo_sets, "auto-parse", "auto-parse", "active",
		NULL);

	EinaPreferences *prefs = eina_plugin_get_preferences(plugin);
	eina_preferences_add_tab(prefs, __prefs_tab);

	return TRUE;
}

G_MODULE_EXPORT gboolean
player_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	GtkWidget *player = eina_application_get_player(app);
	if (!player || !EINA_IS_PLAYER(player))
		g_warn_if_fail(EINA_IS_PLAYER(player));
	else
	{
		EinaDock *dock = eina_application_get_dock(app);
		eina_dock_remove_widget(dock, dock_tab);
	}

	if (__ui_merge_id > 0)
	{
		GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
		gtk_ui_manager_remove_ui(ui_mng, __ui_merge_id);
	}
	if (__prefs_tab)
	{
		EinaPreferences *prefs = eina_plugin_get_preferences(plugin);
		eina_preferences_remove_tab(prefs, __prefs_tab);
	}
	return TRUE;
}

static void
about_show(EinaPlayer *player)
{
	const gchar *artists[] = {
		"Logo: Pau Tomas <pautomas@gmail.com>",
		"Website and some artwork: Jacobo Tarragon <tarragon@aditel.org>",
		NULL
	};
	const gchar *authors[] = {
		"Luis Lopez <luis.lopez@cuarentaydos.com>",
		NULL
	};
	gchar *license = 
	N_("Eina is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n"
	"\n"
	"Eina is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public License along with Eina; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.");

	GdkPixbuf *logo_pb = NULL;
	gchar *logo_path = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "eina.svg");
	if (!logo_path)
		g_warning(N_("Missing resource: %s"), "eina.svg");
	else
	{
		logo_pb = gdk_pixbuf_new_from_file_at_size(logo_path, 128, 128, NULL);
		g_free(logo_path);
	}

	gchar *comments = g_strconcat("(" EINA_CODENAME ")\n\n", N_("A classic player for the modern era"), NULL);
	GtkWidget *about = gtk_about_dialog_new();
	g_object_set((GObject *) about,
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

	g_signal_connect(about, "response", (GCallback) gtk_widget_destroy, NULL);
	gtk_widget_show(about);
}

static void
player_dnd_cb(GtkWidget *w, GType type, const guchar *data, EinaApplication *app)
{
	g_return_if_fail(type == G_TYPE_STRING);

	gchar **uris = g_uri_list_extract_uris((gchar *) data);
	GList *l = gel_strv_to_list(uris, FALSE);
	if (l)
		lomo_player_clear(eina_application_get_lomo(app));
	eina_fs_load_from_uri_multiple(app, l);
	g_list_free(l);
	g_strfreev(uris);
}

static gboolean
player_action_activated_cb(EinaPlayer *player, GtkAction *action, EinaApplication *app)
{
	const gchar *name = gtk_action_get_name(action);

	GError *error = NULL;

	// Handle action
	if (g_str_equal(name, "open-action"))
		eina_fs_load_from_default_file_chooser(app);

	// Action is unknow to us, return FALSE
	else
		return FALSE;

	// Show errors if any
	if (error != NULL)
	{
		g_warning(N_("Unable to complete action '%s': %s"), name, error->message);
		g_error_free(error);
	}
	// Successful or not, we try to handle it, return TRUE
	return TRUE;
}

static void 
action_activated_cb(GtkAction *action, EinaPlayer *player)
{
	g_return_if_fail(GTK_IS_ACTION(action));
	g_return_if_fail(EINA_IS_PLAYER(player));

	EinaApplication *app = EINA_APPLICATION(g_object_get_data((GObject *) player, "eina-application"));

	const gchar *name = gtk_action_get_name(action);
	if (g_str_equal("open-action", name) && EINA_IS_APPLICATION(app))
		eina_fs_load_from_default_file_chooser(app);

	else if (g_str_equal("help-action", name))
		; // gel_ui_application_xdg_open(application, HELP_URL);

	else if (g_str_equal("bug-action", name))
		; // gel_ui_application_xdg_open(application, BUG_URL);

	else if (g_str_equal("about-action", name))
		about_show(player);
	
	else
		g_warning(N_("Unknow action: %s"), gtk_action_get_name(action));
}

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlayer *player)
{
	if (g_str_equal(key, EINA_PLAYER_STREAM_MARKUP_KEY))
	{
		eina_player_set_stream_markup(player, g_settings_get_string(settings, key));
	}

	else
	{
		g_warning(N_("Unhanded key '%s'"), key);
	}
}

