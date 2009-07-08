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
#define EINA_PLUGIN_DATA_TYPE EinaVogon

#include <config.h>
#include <string.h>
#include <gmodule.h>
#include <gel/gel-ui.h>
#include <eina/vogon.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>

#define OSX_SYSTEM (defined(__APPLE__) || defined(__APPLE_CC__))

struct _EinaVogon {
	EinaObj        parent;

	EinaConf      *conf;
	GtkStatusIcon *icon;
	GtkUIManager  *ui_mng;
	GtkWidget     *menu;

	gint player_x, player_y;

	gboolean hold;
} _EinaVogon;

static void
update_ui_manager(EinaVogon *self);

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
"		<separator />"
"		<menuitem action='Previous'  />"
"		<menuitem action='Next' />"
"		<separator />"
"		<menuitem action='Repeat' />"
"		<menuitem action='Shuffle' />"
"		<separator />"
"		<placeholder name='PluginsPlaceholder' />"
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
	GtkActionGroup *ag;
	const GtkActionEntry ui_actions[] = {
		{ "Play",     GTK_STOCK_MEDIA_PLAY,     N_("Play"),     "<alt>x", N_("Play"),            G_CALLBACK(action_activate_cb) },
		{ "Pause",    GTK_STOCK_MEDIA_PAUSE,    N_("Pause"),    "<alt>c", N_("Pause"),           G_CALLBACK(action_activate_cb) },
		{ "Next",     GTK_STOCK_MEDIA_NEXT,     N_("Next"),     "<alt>b", N_("Next stream"),     G_CALLBACK(action_activate_cb) },
		{ "Previous", GTK_STOCK_MEDIA_PREVIOUS, N_("Previous"), "<alt>z", N_("Previous stream"), G_CALLBACK(action_activate_cb) },
		{ "Clear",    GTK_STOCK_CLEAR,          N_("C_lear"),   "<alt>l", N_("Clear playlist"),  G_CALLBACK(action_activate_cb) },
		{ "Quit",     GTK_STOCK_QUIT,           N_("_Quit"),    "<alt>q", N_("Quit Eina"),       G_CALLBACK(action_activate_cb) }
	};
	const GtkToggleActionEntry ui_toggle_actions[] = {
		{ "Shuffle", NULL, N_("Shuffle"), "<alt>s", N_("Shuffle playlist"), G_CALLBACK(action_activate_cb) },
		{ "Repeat",  NULL, N_("Repeat"),  "<alt>r", N_("Repeat playlist"),  G_CALLBACK(action_activate_cb) }
	};

	// Systray is broken on OSX using Quartz backend
	#if OSX_SYSTEM && defined __GDK_X11_H__
	g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_OSX_QUARTZ,
		N_("Gtk+ X11 backend is not supported"));
	return FALSE;
	#endif

	//
	// Getting other modules
	//
	EinaVogon *self = g_new0(EinaVogon, 1);
	if (!eina_obj_init(EINA_OBJ(self), app, "vogon", EINA_OBJ_NONE, error))
		return FALSE;

	if ((self->conf = GEL_APP_GET_SETTINGS(app)) == NULL)
	{
		eina_obj_fini(EINA_OBJ(self));
		g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_NO_SETTINGS_OBJECT,
			N_("Object 'settings' not found"));
		return FALSE;
	}

	#if !OSX_SYSTEM
	if (GEL_APP_GET_PLAYER(app))
		eina_player_set_persistent(GEL_APP_GET_PLAYER(app), TRUE);
	#endif

	self->icon = gtk_status_icon_new_from_stock(EINA_STOCK_STATUS_ICON);
	if (!self->icon)
	{
		g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_NO_STATUS_ICON,
			N_("Cannot create status icon"));
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	self->ui_mng = gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_string(self->ui_mng, ui_def, -1, NULL);

	ag = gtk_action_group_new("default");
	gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
	gtk_action_group_add_toggle_actions(ag, ui_toggle_actions, G_N_ELEMENTS(ui_toggle_actions), self);

	gtk_ui_manager_insert_action_group(self->ui_mng, ag, 0);
	gtk_ui_manager_ensure_update(self->ui_mng);
	self->menu = gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu");
	update_ui_manager(self);

	// Setup properties on UI
	g_object_set(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat"),
		"active", eina_conf_get_bool(self->conf, "/core/repeat", FALSE),
		NULL);
	g_object_set(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle"),
		"active", eina_conf_get_bool(self->conf, "/core/random", FALSE),
		NULL);

	// Connect signals
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	g_signal_connect(self->icon, "popup-menu",      G_CALLBACK(popup_menu_cb), self);
	g_signal_connect(self->icon, "activate",        G_CALLBACK(status_icon_activate_cb), self);
	g_signal_connect(self->icon, "notify::destroy", G_CALLBACK(status_icon_destroy_cb), self);
	g_signal_connect(self->conf, "change",          G_CALLBACK(settings_change_cb), self);

	g_signal_connect_swapped(lomo, "play",  G_CALLBACK(update_ui_manager), self);
	g_signal_connect_swapped(lomo, "stop",  G_CALLBACK(update_ui_manager), self);
	g_signal_connect_swapped(lomo, "pause", G_CALLBACK(update_ui_manager), self);

	// Done, just a warning before
	#if OSX_SYSTEM
	gel_warn(N_("Systray implementation is buggy on OSX. You have been warned, dont file any bugs about this."));
	#endif

	plugin->data = self;
	return TRUE;
}

static gboolean
vogon_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaVogon *self = EINA_PLUGIN_DATA(plugin);

	// Disconnect signals
	g_signal_handlers_disconnect_by_func(self->conf, settings_change_cb, self);
	g_signal_handlers_disconnect_by_func(GEL_APP_GET_LOMO(app), update_ui_manager, self);

	// Free/unref objects
	gel_free_and_invalidate(self->icon,   NULL, g_object_unref);
	gel_free_and_invalidate(self->ui_mng, NULL, g_object_unref);

	// Free self
	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

GtkStatusIcon*
eina_vogon_get_status_icon(EinaVogon *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	return self->icon;
}

GtkUIManager*
eina_vogon_get_ui_manager(EinaVogon *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	return self->ui_mng;
}

static void
update_ui_manager(EinaVogon *self)
{
	gchar *hide = NULL;
	gchar *show = NULL;

	LomoPlayer *lomo = eina_obj_get_lomo(EINA_OBJ(self));
	g_return_if_fail(lomo != NULL);

	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		hide = "/MainMenu/Play";
		show = "/MainMenu/Pause";
		break;
	case LOMO_STATE_PAUSE:
	case LOMO_STATE_STOP:
	default:
		hide = "/MainMenu/Pause";
		show = "/MainMenu/Play";
		break;
	}
	gtk_widget_hide(gtk_ui_manager_get_widget(self->ui_mng, hide));
	gtk_widget_show(gtk_ui_manager_get_widget(self->ui_mng, show));
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
	"vogon", PACKAGE_VERSION, "lomo,settings,art",
	NULL, NULL,

	N_("Build-in systray and notification plugin"), NULL, NULL,

	vogon_init, vogon_fini,

	NULL, NULL, NULL
};

