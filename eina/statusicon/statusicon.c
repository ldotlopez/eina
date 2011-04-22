/*
 * eina/statusicon/statusicon.c
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

#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>

typedef struct {
	GelPlugin *plugin;

	GtkStatusIcon  *icon;
	GtkUIManager   *ui_mng;
	guint           ui_mng_merge_id;
	GtkActionGroup *ag;
	GtkWidget      *menu;
	gint            player_x, player_y;
	gboolean        window_persistant;
} StatusIconPlugin;

static void
update_ui_manager(StatusIconPlugin *self);

// --
// UI callbacks
// --
static void
status_icon_activate_cb(GtkWidget *w , StatusIconPlugin *self);
static void
action_activate_cb(GtkAction *action, StatusIconPlugin *self);
static void
status_icon_popup_menu_cb(GtkWidget *w, guint button, guint activate_time, StatusIconPlugin *self);
static gboolean
status_icon_destroy_cb(GtkWidget *w, StatusIconPlugin *self);

enum {
	STATUS_ICON_NO_ERROR = 0,
	STATUS_ICON_ERROR_NO_STATUS_ICON
};

static const gchar ui_def[] =
"<ui>"
"	<popup name='Main'>"
"		<menuitem name='Show' action='show-action'  />"
"		<menuitem name='Hide' action='hide-action'  />"
"		<separator />"
"		<menuitem name='Play'  action='play-action'  />"
"		<menuitem name='Pause' action='pause-action'  />"
"		<separator />"
"		<menuitem name='Next'     action='next-action'  />"
"		<menuitem name='Previous' action='prev-action'  />"
"		<separator />"
"		<menuitem name='Add'   action='open-action'/>"
"		<menuitem name='Clear' action='clear-action'/>"
"		<separator />"
"		<menuitem name='Repeat' action='repeat-action'/>"
"		<menuitem name='Random' action='random-action'/>"
"		<separator />"
"		<placeholder name='PluginsPlaceholder' />"
"		<separator />"
"		<menuitem name='Quit'  action='quit-action'/>"
"	</popup>"
"</ui>";

static const GtkActionEntry ui_actions[] = {
	{ "show-action",  NULL,     N_("Show Eina"), NULL,     N_("Show Eina"),       G_CALLBACK(action_activate_cb) },
	{ "hide-action",  NULL,     N_("Hide Eina"), NULL,     N_("Hide Eina"),       G_CALLBACK(action_activate_cb) },
	{ "play-action",  GTK_STOCK_MEDIA_PLAY,     N_("Play"),      "<alt>x", N_("Play"),            G_CALLBACK(action_activate_cb) },
	{ "pause-action", GTK_STOCK_MEDIA_PAUSE,    N_("Pause"),     "<alt>c", N_("Pause"),           G_CALLBACK(action_activate_cb) },
	{ "next-action",  GTK_STOCK_MEDIA_NEXT,     N_("Next"),      "<alt>b", N_("Next stream"),     G_CALLBACK(action_activate_cb) },
	{ "prev-action",  GTK_STOCK_MEDIA_PREVIOUS, N_("Previous"),  "<alt>z", N_("Previous stream"), G_CALLBACK(action_activate_cb) },
	{ "open-action",  GTK_STOCK_OPEN,           N_("_Open"),     "<alt>o", N_("Add or queue stream(s)"),  G_CALLBACK(action_activate_cb) },
	{ "clear-action", GTK_STOCK_CLEAR,          N_("C_lear"),    "<alt>l", N_("Clear playlist"),  G_CALLBACK(action_activate_cb) },
	{ "quit-action",  GTK_STOCK_QUIT,           N_("_Quit"),     "<alt>q", N_("Quit Eina"),       G_CALLBACK(action_activate_cb) }
};

static const GtkToggleActionEntry ui_toggle_actions[] = {
	{ "random-action", NULL, N_("Random"), "<alt>s", N_("Random playlist"), NULL },
	{ "repeat-action", NULL, N_("Repeat"), "<alt>r", N_("Repeat playlist"), NULL }
};

GEL_DEFINE_QUARK_FUNC(status_icon)

static void
status_icon_data_destroy(StatusIconPlugin *data)
{
	g_return_if_fail(data);

	gel_free_and_invalidate(data->icon, NULL, g_object_unref);
	gel_free_and_invalidate(data->ag, NULL, g_object_unref);
	gel_plugin_set_data(data->plugin, NULL);

	EinaApplication *app = eina_plugin_get_application(data->plugin);

	// Disconnect signals
	g_signal_handlers_disconnect_by_func(eina_plugin_get_lomo(data->plugin), update_ui_manager, data);
	g_signal_handlers_disconnect_by_func(eina_application_get_window(app), update_ui_manager, data);

	// Free/unref objects
	gel_free_and_invalidate(data->ui_mng, NULL, g_object_unref);
	gel_free_and_invalidate(data->icon,   NULL, g_object_unref);
	gel_free_and_invalidate(data->ag,     NULL, g_object_unref);

	g_free(data);
}

gboolean
statusicon_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	// Systray is broken on OSX using Quartz backend
	#if OSX_SYSTEM && defined __GDK_X11_H__
	g_set_error(error, status_icon_quark(), STATUS_ICON_ERROR_OSX_QUARTZ,
		N_("Gtk+ X11 backend is not supported"));
	return FALSE;
	#endif

	StatusIconPlugin *data= g_new0(StatusIconPlugin, 1);
	data->plugin = plugin;

	if (!(data->icon = gtk_status_icon_new_from_stock(EINA_STOCK_STATUS_ICON)))
	{
		g_set_error(error, status_icon_quark(), STATUS_ICON_ERROR_NO_STATUS_ICON,
			N_("Cannot create status icon"));
		status_icon_data_destroy(data);
		return FALSE;
	}

	data->ui_mng = gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_string(data->ui_mng, ui_def, -1, NULL);

	data->ag = gtk_action_group_new("default");
	gtk_action_group_add_actions(data->ag, ui_actions, G_N_ELEMENTS(ui_actions), data);
	gtk_action_group_add_toggle_actions(data->ag, ui_toggle_actions, G_N_ELEMENTS(ui_toggle_actions), data);

	// Setup properties on UI
	LomoPlayer *lomo = eina_plugin_get_lomo(plugin);
	g_object_bind_property(
		lomo, "repeat",
		gtk_action_group_get_action(data->ag, "repeat-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	g_object_bind_property(
		lomo, "random",
		gtk_action_group_get_action(data->ag, "random-action"), "active",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	// Check return
	gtk_ui_manager_insert_action_group(data->ui_mng, data->ag, 0);
	gtk_ui_manager_ensure_update(data->ui_mng);
	data->menu = gtk_ui_manager_get_widget(data->ui_mng, "/Main");
	update_ui_manager(data);

	// Connect signals
	g_signal_connect(data->icon, "popup-menu",      G_CALLBACK(status_icon_popup_menu_cb), data);
	g_signal_connect(data->icon, "activate",        G_CALLBACK(status_icon_activate_cb), data);
	g_signal_connect(data->icon, "notify::destroy", G_CALLBACK(status_icon_destroy_cb), data);

	g_signal_connect_swapped(lomo, "play",  G_CALLBACK(update_ui_manager), data);
	g_signal_connect_swapped(lomo, "stop",  G_CALLBACK(update_ui_manager), data);
	g_signal_connect_swapped(lomo, "pause", G_CALLBACK(update_ui_manager), data);

	GtkWindow *window = (GtkWindow *) eina_application_get_window(eina_plugin_get_application(plugin));
	g_signal_connect_swapped(window, "show",  G_CALLBACK(update_ui_manager), data);
	g_signal_connect_swapped(window, "hide",  G_CALLBACK(update_ui_manager), data);

	data->window_persistant = eina_window_get_persistant(EINA_WINDOW(window));
	eina_window_set_persistant(EINA_WINDOW(window), TRUE);

	// Done, just a warning before
	#if OSX_SYSTEM
	gel_warn(N_("Systray implementation is buggy on OSX. You have been warned, dont file any bugs about this."));
	#endif

	gel_plugin_set_data(plugin, data);
	return TRUE;
}

gboolean
statusicon_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	StatusIconPlugin *data = (StatusIconPlugin *) gel_plugin_get_data(plugin);

	EinaWindow *window = eina_application_get_window(app);
	eina_window_set_persistant(window, data->window_persistant);

	// Disconnect signals
	g_signal_handlers_disconnect_by_func(eina_plugin_get_lomo(plugin), update_ui_manager, data);
	g_signal_handlers_disconnect_by_func(window, update_ui_manager, data);

	// Free/unref objects
	gel_free_and_invalidate(data->ui_mng, NULL, g_object_unref);
	gel_free_and_invalidate(data->icon,   NULL, g_object_unref);

	// Free self
	g_free(data); 

	return TRUE;
}

static void
update_ui_manager(StatusIconPlugin *self)
{
	gchar *hide = NULL;
	gchar *show = NULL;

	LomoPlayer *lomo = eina_plugin_get_lomo(self->plugin);
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		hide = "/Main/Play";
		show = "/Main/Pause";
		break;
	case LOMO_STATE_PAUSE:
	case LOMO_STATE_STOP:
	default:
		hide = "/Main/Pause";
		show = "/Main/Play";
		break;
	}
	gtk_widget_hide(gtk_ui_manager_get_widget(self->ui_mng, hide));
	gtk_widget_show(gtk_ui_manager_get_widget(self->ui_mng, show));

	EinaApplication *app = eina_plugin_get_application(self->plugin);
	GtkWindow *window = (GtkWindow *) eina_application_get_window(app);
	if (gtk_widget_get_visible(GTK_WIDGET(window)))
	{
		hide = "/Main/Show";
		show = "/Main/Hide";
	}
	else
	{
		show = "/Main/Show";
		hide = "/Main/Hide";
	}
	gtk_widget_hide(gtk_ui_manager_get_widget(self->ui_mng, hide));
	gtk_widget_show(gtk_ui_manager_get_widget(self->ui_mng, show));
}

// --
// Implement UI Callbacks 
// --
static void
status_icon_popup_menu_cb (GtkWidget *w, guint button, guint activate_time, StatusIconPlugin *self)
{
    gtk_menu_popup(
        GTK_MENU(self->menu),
        NULL, NULL, NULL, NULL,
        button,
        activate_time
    );
}

static void
action_activate_cb(GtkAction *action, StatusIconPlugin *self)
{
	GError *error = NULL;
	const gchar *name = gtk_action_get_name(action);

	EinaApplication *app = eina_plugin_get_application(self->plugin);

	if (g_str_equal(name, "show-action"))
		gtk_widget_show(GTK_WIDGET(eina_application_get_window(app)));

	else if (g_str_equal(name, "hide-action"))
		gtk_widget_hide(GTK_WIDGET(eina_application_get_window(app)));

	else if (g_str_equal(name, "play-action"))
		lomo_player_play(eina_plugin_get_lomo(self->plugin), &error);

	else if (g_str_equal(name, "pause-action"))
		lomo_player_pause(eina_plugin_get_lomo(self->plugin), &error);
	
	else if (g_str_equal(name, "stop-action")) 
		lomo_player_stop(eina_plugin_get_lomo(self->plugin), &error);

	else if (g_str_equal(name, "prev-action"))
		lomo_player_go_previous(eina_plugin_get_lomo(self->plugin), NULL);

	else if (g_str_equal(name, "next-action"))
		lomo_player_go_next(eina_plugin_get_lomo(self->plugin), NULL);

	else if (g_str_equal(name, "open-action"))
		eina_fs_load_from_default_file_chooser(app);

	else if (g_str_equal(name, "clear-action"))
		lomo_player_clear(eina_plugin_get_lomo(self->plugin));
	
	else if (g_str_equal(name, "quit-action"))
		g_application_release(G_APPLICATION(eina_plugin_get_application(self->plugin)));
		// gtk_application_quit(eina_plugin_get_application(self->plugin));

	else
		g_warning("Unknow action: %s", name);

	if (error != NULL)
	{
		g_warning(N_("Error invoking action '%s': %s"), name, error->message);
		g_error_free(error);
	}
}

static gboolean
status_icon_destroy_cb(GtkWidget *w, StatusIconPlugin *self)
{
	EinaApplication *app = eina_plugin_get_application(self->plugin);
	GtkWidget *window = GTK_WIDGET(eina_application_get_window(app));

	if (!gtk_widget_get_visible(window))
		gtk_widget_show(window);

	g_warning("Fixme");
	// vogon_plugin_fini(eina_obj_get_app(self), gel_app_shared_get(eina_obj_get_app(self), "vogon"), NULL);
	return FALSE;
}

static void
status_icon_activate_cb
(GtkWidget *w, StatusIconPlugin *self)
{
	EinaApplication *app = eina_plugin_get_application(self->plugin);
	GtkWindow *window = GTK_WINDOW(eina_application_get_window(app));

	if (!window)
		return;

	if (gtk_widget_get_visible(GTK_WIDGET(window)))
	{
		gtk_window_get_position(window,
			&self->player_x,
			&self->player_y);
		gtk_widget_hide((GtkWidget *) window);
	}
	else
	{
		if (!gtk_widget_get_visible(GTK_WIDGET(window)))
			gtk_window_move(window, self->player_x, self->player_y);
		gtk_window_present(window);
	}
}

