/*
 * eina/player/eina-player-plugin.c
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

#include "eina-player-plugin.h"
#include <lomo/lomo-em-art-provider.h>
#include <eina/ext/eina-fs.h>
#include <eina/ext/eina-stock.h>
#include <eina/ext/eina-extension.h>
#include <eina/dock/eina-dock-plugin.h>
#include <eina/lomo/eina-lomo-plugin.h>
#include <eina/preferences/eina-preferences-plugin.h>

#define EINA_TYPE_PLAYER_PLUGIN         (eina_player_plugin_get_type ())
#define EINA_PLAYER_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPlugin))
#define EINA_PLAYER_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPlugin))
#define EINA_IS_PLAYER_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_PLAYER_PLUGIN))
#define EINA_IS_PLAYER_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_PLAYER_PLUGIN))
#define EINA_PLAYER_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPluginClass))

typedef struct {
	EinaDockTab *dock_tab;
	guint ui_merge_id;
	EinaPreferencesTab *prefs_tab;
} EinaPlayerPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPlugin, eina_player_plugin)

#define EINA_PLAYER_PREFERENCES_DOMAIN EINA_APP_DOMAIN".preferences.player"
#define EINA_PLAYER_STREAM_MARKUP_KEY  "stream-markup"

static void
player_dnd_cb(GtkWidget *w, GType type, const guchar *data, EinaApplication *app);
static gboolean
player_action_activated_cb(EinaPlayer *player, GtkAction *action, EinaApplication *app);
static void
action_activated_cb(GtkAction *action, EinaPlayer *player);

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

static void
toplevel_notify_cb(GtkWindow *toplevel, GParamSpec *pspec, EinaPlayer *player)
{
	if (g_str_equal(pspec->name, "resizable"))
	{
		return;

		gboolean is_resizable = gtk_window_get_resizable(toplevel);
		gint req = is_resizable ? 20 : -1;
		g_debug("Resizable event: is %s resizable", is_resizable ? "" : "NOT");

		GtkWidget *label = gel_ui_generic_get_typed(player, GTK_WIDGET, "stream-title-label");
		gtk_widget_set_size_request(label, req, -1);
		label = gel_ui_generic_get_typed(player, GTK_WIDGET, "stream-artist-label");
		gtk_widget_set_size_request(label, req, -1);
	}
}

static void
player_hierarchy_changed_cb(GtkWidget *player, GtkWidget *previous_toplevel, EinaApplication *app)
{
	if (previous_toplevel)
		g_signal_handlers_disconnect_by_func(previous_toplevel, toplevel_notify_cb, player);

	GtkWidget *toplevel = gtk_widget_get_toplevel(player);
	if (toplevel)
		g_signal_connect(toplevel, "notify::resizable", (GCallback) toplevel_notify_cb, player);
}


static gboolean
eina_player_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPlayerPlugin      *plugin = EINA_PLAYER_PLUGIN(activatable);
	EinaPlayerPluginPrivate *priv = plugin->priv;

	GtkWidget *player = eina_player_new();
	g_object_set(player,
		"lomo-player",    eina_application_get_interface(app, "lomo"),
		"default-pixbuf", gdk_pixbuf_new_from_file(lomo_em_art_provider_get_default_cover_path(), NULL),
		NULL);
	gel_ui_widget_enable_drop(player, (GCallback) player_dnd_cb, app);

	g_object_set_data((GObject *) player, "eina-application", app);
	g_object_set_data((GObject *) player, "eina-plugin", plugin);

	// Connect actions
	g_signal_connect(player, "action-activated", (GCallback) player_action_activated_cb, app);
	g_signal_connect(player, "hierarchy-changed", (GCallback) player_hierarchy_changed_cb, app);

	// Export
	eina_application_set_interface(app, "player", player);

	// Pack and show
	g_object_set(G_OBJECT(player),
		"hexpand", TRUE,
		"vexpand", FALSE,
		NULL);

	priv->dock_tab = eina_application_add_dock_widget(app,
		"player", player, gtk_label_new(_("Player")), EINA_DOCK_FLAG_DEFAULT);

	// Attach menus
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
	GError *e = NULL;
	if ((priv->ui_merge_id = gtk_ui_manager_add_ui_from_string (ui_mng, ui_mng_xml, -1 , &e)) == 0)
	{
		g_warning(N_("Unable to add UI to window GtkUIManager: '%s'"), e->message);
		g_error_free(e);
	}
	GtkActionGroup *ag = gtk_action_group_new("player");
	gtk_action_group_add_actions(ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), player);
	gtk_ui_manager_insert_action_group(ui_mng, ag, G_MAXINT);

	gchar *datadir = peas_extension_base_get_data_dir((PeasExtensionBase *) plugin);
	gchar *prefs_ui_file = g_build_filename(datadir, "preferences.ui", NULL);
	GtkWidget *widget = gel_ui_generic_new_from_file(prefs_ui_file);
	g_free(datadir);
	g_free(prefs_ui_file);

	priv->prefs_tab = eina_preferences_tab_new();
	g_object_set(priv->prefs_tab,
		"label-text", N_("Player"),
		"widget", widget,
		NULL);

	GSettings *lomo_sets = eina_application_get_settings(app, EINA_LOMO_PREFERENCES_DOMAIN);
	eina_preferences_tab_bindv(priv->prefs_tab,
		lomo_sets, "repeat",       "repeat",       "active",
		lomo_sets, "random",       "random",       "active",
		lomo_sets, "auto-play",    "auto-play",    "active",
		lomo_sets, "auto-parse",   "auto-parse",   "active",
		lomo_sets, "gapless-mode", "gapless-mode", "active",
		eina_application_get_settings(app, EINA_APP_DOMAIN), "prefer-dark-theme", "prefer-dark-theme", "active",
		NULL);

	eina_application_add_preferences_tab(app, priv->prefs_tab);


	return TRUE;
}

static gboolean
eina_player_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPlayerPlugin      *plugin = EINA_PLAYER_PLUGIN(activatable);
	EinaPlayerPluginPrivate *priv = plugin->priv;

	GtkWidget *player = GTK_WIDGET(eina_application_get_player(app));
	if (!player || !EINA_IS_PLAYER(player))
		g_warn_if_fail(EINA_IS_PLAYER(player));
	else
	{
		EinaDock *dock = eina_application_get_dock(app);
		eina_dock_remove_widget(dock, priv->dock_tab);
	}

	if (priv->ui_merge_id > 0)
	{
		GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
		gtk_ui_manager_remove_ui(ui_mng, priv->ui_merge_id);
		priv->ui_merge_id = 0;
	}
	if (priv->prefs_tab)
	{
		eina_application_remove_preferences_tab(app, priv->prefs_tab);
		priv->prefs_tab = NULL;
	}
	return TRUE;
}

/**
 * eina_application_get_player:
 * @application: An #EinaApplication
 *
 * Get the #EinaPlayer object from the application
 *
 * Returns: (transfer none): The #EinaPlayer
 */
EinaPlayer*
eina_application_get_player(EinaApplication *application)
{
	return eina_application_get_interface(application, "player");
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
		"Luis Lopez <luis@cuarentaydos.com>",
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
	gchar *title = g_strdup_printf(_("About %s"), PACKAGE_NAME);

	gtk_show_about_dialog(
	    GTK_WINDOW(eina_application_get_window(EINA_APPLICATION(g_object_get_data((GObject *) player, "eina-application")))),
		"artists", artists,
		"authors", authors,
		"program-name", PACKAGE_NAME,
		"copyright", "Copyright © 2003-2011 Luis Lopez\nCopyright © 2003-2005 Luis Lopez and Daniel Ripolles",
		"comments", comments,
		"website", EINA_URL,
		"website-label", EINA_URL,
		"license", license,
		"version", PACKAGE_VERSION,
		"wrap-license", TRUE,
		"logo", logo_pb,
		"title", title,
		NULL);
	g_free(comments);
	g_free(title);
}

static void
player_dnd_cb(GtkWidget *w, GType type, const guchar *data, EinaApplication *app)
{
	g_return_if_fail(type == G_TYPE_STRING);

	gchar **uris = g_uri_list_extract_uris((gchar *) data);
	if (uris && uris[0])
	{
		lomo_player_clear(eina_application_get_lomo(app));
		eina_fs_load_uri_strv(app, (const gchar * const*) uris);
	}
	if (uris)
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
		eina_application_launch_for_uri(app, EINA_URL_HELP, NULL);

	else if (g_str_equal("bug-action", name))
		eina_application_launch_for_uri(app, EINA_URL_BUGS, NULL);

	else if (g_str_equal("about-action", name))
		about_show(player);

	else
		g_warning(N_("Unknow action: %s"), gtk_action_get_name(action));
}


