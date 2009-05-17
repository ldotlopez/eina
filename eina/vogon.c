/*
 * eina/vogon.c
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

#define GEL_DOMAIN "Eina::Vogon"

#include <config.h>
#include <string.h>
#include <gmodule.h>
#include <gel/gel-ui.h>
#include <eina/vogon.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>

struct _EinaVogon {
	EinaObj        parent;

	EinaConf      *conf;
	GtkStatusIcon *icon;
	GtkUIManager  *ui_mng;
	GtkWidget     *menu;

	gint player_x, player_y;

	gboolean hold;
} _EinaVogon;

// --
// Lomo callbacks
// --
// static void
// lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaVogon *self);

// --
// UI callbacks
// --
static gboolean
status_icon_activate_cb(GtkWidget *w , EinaVogon *self);
static void
action_activate_cb(GtkAction *action, EinaVogon *self);
static void
popup_menu_cb(GtkWidget *w, guint button, guint activate_time, EinaVogon *self);
static gboolean
status_icon_destroy_cb(GtkWidget *w, EinaVogon *self);
#if 0
static void
on_eina_vogon_drag_data_received
    (GtkWidget       *widget,
    GdkDragContext   *drag_context,
    gint              x,
    gint              y,
    GtkSelectionData *data,
    guint             info,
    guint             time,
	EinaVogon *self);
#endif

// --
// Other callbacks
// --
static void
settings_change_cb(EinaConf *conf, const gchar *key, EinaVogon *self);

/*
 * Init/Exit functions 
 */

static const gchar ui_def[] =
"<ui>"
"	<popup name='MainMenu'>"
"		<menuitem action='Play'  />"
"		<menuitem action='Pause' />"
"		<menuitem action='Stop'  />"
"		<separator />"
"		<menuitem action='Previous'  />"
"		<menuitem action='Next' />"
"		<separator />"
"		<menuitem action='Repeat' />"
"		<menuitem action='Shuffle' />"
"		<separator />"
"		<menuitem action='Clear' />"
"		<menuitem action='Quit' />"
"	</popup>"
"</ui>";

static GQuark
vogon_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-vogon");
	return ret;
}

static gboolean
vogon_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaVogon *self;
	GdkPixbuf *pixbuf  = NULL;
	GelUIImageDef img_def = {
#if defined(__APPLE__) || defined(__APPLE_CC__)
		NULL, "osx-systray-icon.png",
#else
		NULL, "standard-systray-icon.png",
#endif
		24, 24
	};

	GtkActionGroup *ag;

	const GtkActionEntry ui_actions[] = {
		/* Menu actions */
		{ "Play", GTK_STOCK_MEDIA_PLAY, N_("Play"),
		"<alt>x", "Play", G_CALLBACK(action_activate_cb) },
		{ "Pause", GTK_STOCK_MEDIA_PAUSE, N_("Pause"),
		"<alt>c", "Pause", G_CALLBACK(action_activate_cb) },
		{ "Stop", GTK_STOCK_MEDIA_STOP, N_("Stop"),
		"<alt>v", "Stop", G_CALLBACK(action_activate_cb) },
		{ "Next", GTK_STOCK_MEDIA_NEXT, N_("Next"),
		"<alt>b", "Next stream", G_CALLBACK(action_activate_cb) },
		{ "Previous", GTK_STOCK_MEDIA_PREVIOUS, N_("Previous"),
		"<alt>z", "Previous stream", G_CALLBACK(action_activate_cb) },
		{ "Clear", GTK_STOCK_CLEAR, N_("C_lear"),
		"<alt>l", "Clear playlist", G_CALLBACK(action_activate_cb) },
		{ "Quit", GTK_STOCK_QUIT, N_("_Quit"),
		"<alt>q", "Quit Eina", G_CALLBACK(action_activate_cb) }
	};
	const GtkToggleActionEntry ui_toggle_actions[] = {
		{ "Shuffle", NULL, N_("Shuffle"),
		"<alt>s", "Shuffle playlist", G_CALLBACK(action_activate_cb) },
		{ "Repeat", NULL , N_("Repeat"),
		"<alt>r", "Repeat playlist", G_CALLBACK(action_activate_cb) }
	};

	// Systray is broken on OSX using Quartz backend
#ifdef __GDK_QUARTZ_H__
	g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_OSX_QUARTZ, N_("Gtk+ with Quartz backend is not supported"));
	return FALSE;
#endif

	self = g_new0(EinaVogon, 1);
	if (!eina_obj_init(EINA_OBJ(self), app, "vogon", EINA_OBJ_NONE, error))
		return FALSE;

	// Crete icon
	self->icon = gtk_status_icon_new();
	if (!self->icon)
	{
		g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_CANNOT_EMBED, N_("Cannot embed GtkStatusIcon, aborting."));
		eina_obj_fini((EinaObj *) self);
		return FALSE;
	}

	pixbuf = gel_ui_load_pixbuf_from_imagedef(img_def, error);
	if (pixbuf == NULL)
	{
		g_object_unref(self->icon);
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}
	gtk_status_icon_set_from_pixbuf(self->icon, pixbuf);

	// Create menu
	self->ui_mng = gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_string(self->ui_mng, ui_def, -1, NULL);

	ag = gtk_action_group_new("default");
	gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
	gtk_action_group_add_toggle_actions(ag, ui_toggle_actions, G_N_ELEMENTS(ui_toggle_actions), self);

	gtk_ui_manager_insert_action_group(self->ui_mng, ag, 0);
	gtk_ui_manager_ensure_update(self->ui_mng);

	self->menu = gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu");

	g_signal_connect(self->icon, "popup-menu",
		G_CALLBACK(popup_menu_cb), self);
	g_signal_connect(
		G_OBJECT(self->icon), "activate",
		G_CALLBACK(status_icon_activate_cb), self);

	// Load settings 
	if ((self->conf = GEL_APP_GET_SETTINGS(app)) == NULL)
	{
		g_object_unref(self->ui_mng);
		g_object_unref(self->icon);
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat")),
		eina_conf_get_bool(self->conf, "/core/repeat", FALSE));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle")),
		eina_conf_get_bool(self->conf, "/core/random", FALSE));

	g_signal_connect(self->conf, "change", G_CALLBACK(settings_change_cb), self);

#if defined(__APPLE__) || defined(__APPLE_CC__)
	gel_warn(GEL_DOMAIN " is buggy on OSX. You have been warned, dont file any bugs about this.");
#endif

	if (GEL_APP_GET_PLAYER(app))
		eina_player_set_persistent(GEL_APP_GET_PLAYER(app), TRUE);

	// Prepare destroy
	g_signal_connect(self->icon, "notify::destroy",
	G_CALLBACK(status_icon_destroy_cb), self);

	return TRUE;
}

static gboolean
vogon_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaVogon *self = gel_app_shared_get(app, "vogon");
	gel_free_and_invalidate(self->icon,   NULL, g_object_unref);
	gel_free_and_invalidate(self->ui_mng, NULL, g_object_unref);

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

// --
// Implement UI Callbacks 
// --
static void
popup_menu_cb (GtkWidget *w, guint button, guint activate_time, EinaVogon *self)
{
    gtk_menu_popup(
        GTK_MENU(self->menu),
        NULL, NULL, NULL, NULL,
        button,
        activate_time
    );
}

static void
action_activate_cb(GtkAction *action, EinaVogon *self)
{
	GError *error = NULL;
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "Play"))
		lomo_player_play(eina_obj_get_lomo(self), &error);

	else if (g_str_equal(name, "Pause"))
		lomo_player_pause(eina_obj_get_lomo(self), &error);
	
	else if (g_str_equal(name, "Stop")) 
		lomo_player_stop(eina_obj_get_lomo(self), &error);

	else if (g_str_equal(name, "Previous"))
		lomo_player_go_prev(eina_obj_get_lomo(self), NULL);

	else if (g_str_equal(name, "Next"))
		lomo_player_go_next(eina_obj_get_lomo(self), NULL);

	else if (g_str_equal(name, "Repeat"))
	{
		lomo_player_set_repeat(
			eina_obj_get_lomo(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
		eina_conf_set_bool(
			self->conf, "/core/repeat",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
	}

	else if (g_str_equal(name, "Shuffle"))
	{
		lomo_player_set_random(
			eina_obj_get_lomo(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
		eina_conf_set_bool(
			self->conf, "/core/random",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
	}

	else if (g_str_equal(name, "Clear"))
		lomo_player_clear(eina_obj_get_lomo(self));
	
	else if (g_str_equal(name, "Quit"))
		g_object_unref(eina_obj_get_app(self));

	else
		gel_warn("Unknow action: %s", name);

	if (error != NULL)
	{
		gel_error("%s", error->message);
		g_error_free(error);
	}
}

static gboolean
status_icon_destroy_cb(GtkWidget *w, EinaVogon *self)
{
	GtkWidget *window = eina_obj_get_widget(self, "main-window");

	if (!GTK_WIDGET_VISIBLE(window))
		gtk_widget_show(window);

	vogon_fini(eina_obj_get_app(self), gel_app_shared_get(eina_obj_get_app(self), "vogon"), NULL);
	return FALSE;
}

static gboolean
status_icon_activate_cb
(GtkWidget *w, EinaVogon *self)
{
	EinaPlayer *player;
	GtkWindow  *window;
	if (!gel_app_get_plugin_by_name(eina_obj_get_app(self), "player"))
		return FALSE;

	player = GEL_APP_GET_PLAYER(eina_obj_get_app(self));
	window = GTK_WINDOW(eina_obj_get_widget(player, "main-window"));

	if (GTK_WIDGET_VISIBLE(window))
	{
		gtk_window_get_position(window,
			&self->player_x,
			&self->player_y);
		gtk_widget_hide((GtkWidget *) window);
	}
	else
	{
		gtk_window_move(window,
			self->player_x,
			self->player_y);
		gtk_widget_show((GtkWidget *) window);
		if (!gtk_window_is_active(window))
			gtk_window_activate_default(window);
	}
	return FALSE;
}

#if 0
static void
on_eina_vogon_drag_data_received
    (GtkWidget       *widget,
    GdkDragContext  *drag_context,
    gint             x,
    gint             y,
    GtkSelectionData *data,
    guint            info,
    guint            time,
    EinaVogon     *self)
{
	gint i;
	gchar *filename;
	gchar **uris;
	GSList *uri_list = NULL, *uri_scan, *uri_filter;

	uris = g_uri_list_extract_uris((gchar *) data->data);
    for ( i = 0; uris[i] != NULL; i++ ) {
        filename = g_filename_from_uri(uris[i], NULL, NULL);
        uri_list = g_slist_prepend(uri_list, g_strconcat("file://", filename, NULL));
        g_free(filename);
    }
    g_strfreev(uris);
    uri_list = g_slist_reverse(uri_list);

    uri_scan = eina_fs_scan(uri_list); //  Scan
    g_ext_slist_free(uri_list, g_free);

    uri_filter = eina_fs_filter_filter(self->stream_filter, // Filter
        uri_scan,
        TRUE, FALSE, FALSE,
        TRUE, FALSE);

    lomo_player_clear(eina_obj_get_lomo(self));
    lomo_player_append_uri_multi(eina_obj_get_lomo(self), (GList *) uri_filter);

    g_slist_free(uri_filter);
    g_ext_slist_free(uri_scan, g_free);
}
#endif

/*
 * Other callbacks
 */
static void
settings_change_cb (EinaConf *conf, const gchar *key, EinaVogon *self)
{
	if (g_str_equal(key, "/core/repeat"))
	    gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat")),
			eina_conf_get_bool(conf, "/core/repeat", TRUE));
	
	else if (g_str_equal(key, "/core/random"))
	    gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle")),
			eina_conf_get_bool(conf, "/core/random", TRUE));
}

G_MODULE_EXPORT GelPlugin vogon_plugin = {
	GEL_PLUGIN_SERIAL,
	"vogon", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in vogon plugin"), NULL, NULL,

	vogon_init, vogon_fini,

	NULL, NULL, NULL
};

