/*
 * eina/player.c
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

#define GEL_DOMAIN "Eina::Player"

// Stds
#include <config.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gmodule.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>

// Modules
#include <eina/eina-plugin.h>
#include <eina/player.h>

// Widgets
#include <eina/ext/eina-seek.h>
#include <eina/ext/eina-volume.h>

enum {
	WIDGET_PLAPAU = 0,
	WIDGET_PLAPAU_IMG,
	WIDGET_NEXT,
	WIDGET_PREV,
	WIDGET_OPEN,
	WIDGET_LABEL,
	WIDGET_COVER_CONTAINER,
	WIDGET_N_ELEMENTS
};

typedef enum {
	EINA_PLAYER_MODE_PLAY,
	EINA_PLAYER_MODE_PAUSE
} EinaPlayerMode;

struct _EinaPlayer {
	EinaObj parent;

	GtkWidget *widgets[WIDGET_N_ELEMENTS];
	// GtkButton   *prev, *play_pause, *next, *open;
	// GtkImage    *play_pause_image;

	gchar       *stream_info_fmt;

	// Preferences widgets and data
	GtkWidget *prefs_widget;
	GtkBox    *prefs_custom_box, *prefs_tips_box;
};



#if 0
static gchar *preferences_fmt_values[][2] = {
	{ // Title - Artist
		"<span size=\"x-large\" weight=\"bold\">%t</span>\n"
		"<span size=\"x-large\" weight=\"normal\">{%a}</span>",

		"<span size=\"x-large\" weight=\"bold\">Title</span>\n"
		"<span size=\"x-large\" weight=\"normal\">Artist</span>"
	},

	{ // Title - Artist (Album)
		"<span size=\"x-large\" weight=\"bold\">%t</span>\n"
		"<span size=\"x-large\" weight=\"normal\">{%a}{ (%b)}</span>",

		"<span size=\"x-large\" weight=\"bold\">Title</span>\n"
		"<span size=\"x-large\" weight=\"normal\">Artist (Album)</span>"
	},

	{ // Title
		"<span size=\"x-large\" weight=\"bold\">%t</span>\n"
		"<span size=\"x-large\" weight=\"normal\">\u200B</span>",

		"<span size=\"x-large\" weight=\"bold\">Title</span>"
		"<span size=\"x-large\" weight=\"normal\">\u200B</span>"
	},

	{ // Custom
		NULL,
		"<span size=\"x-large\" weight=\"bold\">Custom</span>\n"
		"<span size=\"x-large\" weight=\"normal\">Your own format</span>"
	},

	{ NULL, NULL }
};
#endif

// API
/*
static void
switch_state(EinaPlayer *self, EinaPlayerMode mode);
*/
static void
set_info(EinaPlayer *self, LomoStream *stream);

static void
update_state (EinaPlayer *self);
static void
update_sensitiviness(EinaPlayer *self);
static void
about_show(EinaPlayer *self);
static void
setup_dnd(EinaPlayer *self);

// UI callbacks
static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);
static void
button_clicked_cb(GtkWidget *w, EinaPlayer *self);
static void
menu_activate_cb(GtkAction *action, EinaPlayer *self);
static void
volume_value_change_cb(GtkWidget *w, gdouble value, EinaPlayer *self);
static gboolean
window_keybind_handler(GdkEvent *ev, EinaPlayer *self);

// Lomo callbacks
/*
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlayer *self); */
static void
lomo_change_cb (LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);
static void
lomo_clear_cb (LomoPlayer *lomo, EinaPlayer *self);
static void
lomo_all_tags_cb (LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self);

// Callback for parse stream info string
static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream);

static EinaWindowKeyBind keybindings[] = {
	// Add
	{ GDK_Insert,    (EinaWindowKeyBindHandler) window_keybind_handler },
	// Prev
	{ GDK_z,         (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_Page_Up,   (EinaWindowKeyBindHandler) window_keybind_handler },
	// Next
	{ GDK_v,         (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_Page_Down, (EinaWindowKeyBindHandler) window_keybind_handler },
	// Pause
	{ GDK_x,         (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_space,     (EinaWindowKeyBindHandler) window_keybind_handler },
	// Stop
	{ GDK_c,         (EinaWindowKeyBindHandler) window_keybind_handler },
	// Seek
	{ GDK_KP_Left,   (EinaWindowKeyBindHandler) window_keybind_handler },
	{ GDK_KP_Right,  (EinaWindowKeyBindHandler) window_keybind_handler }
};

static GtkActionEntry ui_actions[] = {
	// Menu
	{ "FileMenu", NULL, N_("_File"),
	"<alt>f", NULL, NULL},
	{ "EditMenu", NULL, N_("_Edit"),
	"<alt>e", NULL, NULL},
	{ "HelpMenu", NULL, N_("_Help"),
	"<alt>h", NULL, NULL},

	// Menu item actions 
	{ "Open", GTK_STOCK_OPEN, N_("Load"),
	"<control>o", NULL, G_CALLBACK(menu_activate_cb) },
	{ "Quit", GTK_STOCK_QUIT, N_("Quit"),
	"<control>q", NULL, G_CALLBACK(menu_activate_cb) },

	// Help item actions
	{ "Help", GTK_STOCK_HELP, N_("Help"),
	NULL, "About", G_CALLBACK(menu_activate_cb) },
	{ "Bug", EINA_STOCK_BUG , N_("Report a bug"),
	NULL, "Bug", G_CALLBACK(menu_activate_cb) },
	{ "About", GTK_STOCK_ABOUT, N_("About"),
	NULL, "About", G_CALLBACK(menu_activate_cb) }
};

static gboolean
player_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = NULL;

	// Initialize base class
	self = g_new0(EinaPlayer, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "player", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}

	self->widgets[WIDGET_PLAPAU]     = eina_obj_get_widget(self, "play-pause-button");
	self->widgets[WIDGET_PLAPAU_IMG] = eina_obj_get_widget(self, "play-pause-image");
	self->widgets[WIDGET_NEXT]       = eina_obj_get_widget(self, "next-button");
	self->widgets[WIDGET_PREV]       = eina_obj_get_widget(self, "prev-button");
	self->widgets[WIDGET_OPEN]       = eina_obj_get_widget(self, "open-button");
	self->widgets[WIDGET_LABEL]      = eina_obj_get_widget(self, "stream-info-label");
	self->widgets[WIDGET_COVER_CONTAINER] = eina_obj_get_widget(self, "cover-container");

	// Connect to settings
	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);

	// Get stream info format
	self->stream_info_fmt = (gchar *) eina_conf_get_str(conf, "/player/stream-info-fmt", NULL);

	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = (gchar*) gtk_label_get_label(eina_obj_get_typed(self, GTK_LABEL, "stream-info-label"));
	
	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = 
			"<span size=\"x-large\" weight=\"bold\">%t</span>"
			"<span size=\"x-large\" weight=\"normal\">{%a}</span>";
	self->stream_info_fmt = g_strdup(self->stream_info_fmt);


	// Set state from LomoPlayer
	update_state(self);
	/*
	LomoState state = lomo_player_get_state(eina_obj_get_lomo(self));
	switch_state(self, (state == LOMO_STATE_PLAY) ?  EINA_PLAYER_MODE_PLAY : EINA_PLAYER_MODE_PAUSE); */

	// Update info from LomoPlayer
	set_info(self, lomo_player_get_stream(eina_obj_get_lomo(self)));

	// Initialize volume
	EinaVolume *volume = eina_volume_new();
	eina_volume_set_lomo_player(volume, eina_obj_get_lomo(self));
	gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "volume-button-container"),
		GTK_WIDGET(volume));
	gtk_widget_show(GTK_WIDGET(volume));
	lomo_player_set_volume(
		EINA_OBJ_GET_LOMO(self),
		eina_conf_get_int(EINA_OBJ_GET_SETTINGS(self), "/player/volume", 50));
	gtk_scale_button_set_value((GtkScaleButton *) volume, 
		((gdouble) eina_conf_get_int(EINA_OBJ_GET_SETTINGS(self), "/player/volume", 50)) / 100);
	g_signal_connect(volume, "value-changed", (GCallback) volume_value_change_cb, self);

	// Initialize seek, XXX: Replace _set_*_lable with a g_object_set ?
	EinaSeek *seek = eina_seek_new();
	eina_seek_set_lomo_player(seek, eina_obj_get_lomo(self));
	eina_seek_set_current_label  (seek, eina_obj_get_typed(self, GTK_LABEL, "time-current-label"));
	eina_seek_set_remaining_label(seek, eina_obj_get_typed(self, GTK_LABEL, "time-remaining-label"));
	eina_seek_set_total_label    (seek, eina_obj_get_typed(self, GTK_LABEL, "time-total-label"));
	gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "seek-hscale-container"),
		GTK_WIDGET(seek));
	gtk_widget_show(GTK_WIDGET(seek));

	// Initialize UI Manager
	GError *err = NULL;
	GtkActionGroup *ag;

	gchar *ui_manager_file = gel_plugin_get_resource(plugin, GEL_RESOURCE_UI, "player-menu.ui");
	if (ui_manager_file != NULL)
	{
		GtkUIManager *ui_manager = eina_window_get_ui_manager(EINA_OBJ_GET_WINDOW(self));
		if (gtk_ui_manager_add_ui_from_file(ui_manager, ui_manager_file, &err) == 0)
		{
			gel_warn("Error adding UI to UI Manager: '%s'", err->message);
			g_error_free(err);
		}
		else
		{
			ag = gtk_action_group_new("default");
			gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
			gtk_ui_manager_insert_action_group(ui_manager, ag, 0);
			gtk_ui_manager_ensure_update(ui_manager);
			gtk_box_pack_start(
				eina_obj_get_typed(self, GTK_BOX, "main-box"),
				gtk_ui_manager_get_widget(ui_manager, "/MainMenuBar"),
				FALSE,FALSE, 0);
			gtk_box_reorder_child(
				eina_obj_get_typed(self, GTK_BOX, "main-box"),
				gtk_ui_manager_get_widget(ui_manager, "/MainMenuBar"),
				0);

			gtk_widget_show_all(gtk_ui_manager_get_widget(ui_manager, "/MainMenuBar"));
		}
		g_free(ui_manager_file);
		gtk_window_add_accel_group(
			(GtkWindow *) EINA_OBJ_GET_WINDOW(self),
			// self->main_window,
			gtk_ui_manager_get_accel_group(ui_manager));
	}

	// Connect signals
	GelUISignalDef ui_signals[] = {
		{ "main-window",       "delete-event",       G_CALLBACK(main_window_delete_event_cb) },
		{ "prev-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "next-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "play-pause-button", "clicked", G_CALLBACK(button_clicked_cb) },
		{ "open-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		GEL_UI_SIGNAL_DEF_NONE
	};
	gel_ui_signal_connect_from_def_multiple(eina_obj_get_ui(self), ui_signals, self, NULL);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	/*
	g_signal_connect(eina_obj_get_lomo(self), "play",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "pause",    G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "stop",     G_CALLBACK(lomo_state_change_cb), self);
	*/
	g_signal_connect_swapped(lomo, "play",  G_CALLBACK(update_state), self);
	g_signal_connect_swapped(lomo, "pause", G_CALLBACK(update_state), self);
	g_signal_connect_swapped(lomo, "stop",  G_CALLBACK(update_state), self);

	g_signal_connect(eina_obj_get_lomo(self), "change",   G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "clear",    G_CALLBACK(lomo_clear_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);
	g_signal_connect_swapped(eina_obj_get_lomo(self), "insert", G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(eina_obj_get_lomo(self), "remove", G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(eina_obj_get_lomo(self), "repeat", G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(eina_obj_get_lomo(self), "random", G_CALLBACK(update_sensitiviness), self);

	// Preferences is attached to us (like dock) but this is less than optimal
	/*
	EinaPreferences *prefs = GEL_APP_GET_PREFERENCES(app);
	if (prefs == NULL)
	{
		gel_warn("Cannot load preferences component");
	}
	else
	{
		GtkWidget *prefs_widget = build_preferences_widget(self);
		if (prefs_widget != NULL)
			eina_preferences_add_tab(prefs,
				GTK_IMAGE(gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON)),
				GTK_LABEL(gtk_label_new(_("Player"))),
				prefs_widget);
	}
	*/

	setup_dnd(self);

	// Show it
	// 
	GtkWidget *widget = eina_obj_get_typed(self, GTK_WIDGET, "main-box");
	g_object_ref(widget);
	gtk_container_remove((GtkContainer *)  gtk_widget_get_parent(widget), widget);
	gtk_widget_show(widget);
	eina_window_add_widget(EINA_OBJ_GET_WINDOW(self), widget, FALSE, TRUE, 0);
	eina_window_register_key_bindings(EINA_OBJ_GET_WINDOW(self), G_N_ELEMENTS(keybindings), keybindings, (gpointer) self);

	eina_obj_strip((EinaObj *) self, EINA_OBJ_GTK_UI);

	// gtk_widget_show(GTK_WIDGET(self->main_window));

	// Due some bug on OSX main window needs to be resizable on creation, but
	// without dock or other attached widgets to main window it must not be
	// resizable.
	// gtk_window_set_resizable(self->main_window, FALSE);

	return TRUE;
}

static gboolean
player_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = gel_app_shared_get(app, "player");

	if (!self)
		return FALSE;

	eina_window_unregister_key_bindings(EINA_OBJ_GET_WINDOW(self), G_N_ELEMENTS(keybindings), keybindings, self);
	g_free(self->stream_info_fmt);

	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
}

GtkContainer *
eina_player_get_cover_container(EinaPlayer* self)
{
	return (GtkContainer *) self->widgets[WIDGET_COVER_CONTAINER];
}

static void
update_state(EinaPlayer *self)
{
	LomoState state = lomo_player_get_state(eina_obj_get_lomo((EinaObj *) self));
	g_return_if_fail(state != LOMO_STATE_INVALID);

	gtk_widget_set_tooltip_text(self->widgets[WIDGET_PLAPAU],
		state == LOMO_STATE_PLAY ? N_("Pause current song") : N_("Play current song"));
	gtk_image_set_from_stock((GtkImage *) self->widgets[WIDGET_PLAPAU_IMG],
		state == LOMO_STATE_PLAY ? "gtk-media-pause" : "gtk-media-play",
		GTK_ICON_SIZE_BUTTON);
}

static void
set_info(EinaPlayer *self, LomoStream *stream)
{
	GtkWindow *window = (GtkWindow *) EINA_OBJ_GET_WINDOW(self);
	GtkLabel  *widget = (GtkLabel *) self->widgets[WIDGET_LABEL];
	gchar *tmp, *title, *stream_info;
	
	// No stream, reset
	if (stream == NULL)
	{
		gtk_window_set_title(window, N_("Eina music player"));
		/*
		gtk_label_set_markup(label,
			N_("<span size=\"x-large\" weight=\"bold\">Eina music player</span>\n<span size=\"x-large\" weight=\"normal\">\u200B</span>")
			);
		gtk_label_set_selectable((GtkLabel *) self->widgets[WIDGET_LABEL], FALSE)
		*/
		g_object_set(widget,
			"selectable", FALSE,
			"use-markup", TRUE,
			"label", N_("<span size=\"x-large\" weight=\"bold\">Eina music player</span>\n<span size=\"x-large\" weight=\"normal\">\u200B</span>"),
			NULL);
		return;
	}

	stream_info = gel_str_parser(self->stream_info_fmt, (GelStrParserFunc) stream_info_parser_cb, stream);
	g_object_set(widget,
		"selectable", TRUE,
		"use-markup", TRUE,
		"label", stream_info,
		NULL);
	/*
	gtk_label_set_markup(GTK_LABEL(eina_obj_get_widget(self, "stream-info-label")), stream_info);
	gtk_label_set_selectable(GTK_LABEL(eina_obj_get_widget(self, "stream-info-label")), TRUE);
	*/
	g_free(stream_info);

	if ((title = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE))) == NULL)
	{
		tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		title =  g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
	}

	// gtk_window_set_title(self->main_window, title);
	gtk_window_set_title((GtkWindow *) EINA_OBJ_GET_WINDOW(self), title);
	g_free(title);
}

/*
static gchar*
parse_example_str_cb(gchar key, gpointer data)
{
	switch (key)
	{
	case 'a':
		return g_strdup(N_("Artist"));
	case 'b':
		return g_strdup(N_("Album"));
	case 't':
		return g_strdup(N_("Title"));
	case 'n':
		return g_strdup(N_("#01"));
	default:
		return NULL;
	}
}
*/

#if 0
static GtkWidget*
build_preferences_widget(EinaPlayer *self)
{
	GtkWidget     *ret = NULL;
	GtkWidget     *parent = NULL;
	GtkComboBox   *combo = NULL;
	GtkTextView   *textview = NULL;
	GtkBox        *custom_format_box = NULL, *tips_box = NULL;
	GtkLinkButton *link_button = NULL;

	//
	// Create preferences widget from a gtkui file
	//
	GtkBuilder *ui = NULL;
	GError *err = NULL;
	gchar *ui_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_UI, "player-preferences.ui");
	if (ui_path == NULL)
	{
		gel_error("Cannot find ui file player-preferences.ui");
		goto build_preferences_widget_fail;
	}

	ui = gtk_builder_new();
	if (gtk_builder_add_from_file(ui, ui_path, &err) == 0)
	{
		gel_error("Cannot load player-preferences.ui: %s", err->message);
		goto build_preferences_widget_fail;
	}
	g_free(ui_path);

	if ((ret = GTK_WIDGET(gtk_builder_get_object(ui, "main-widget"))) == NULL)
	{
		gel_error("Widget main-widget is not present in player-preferences.ui");
		goto build_preferences_widget_fail;
	}

	parent = gtk_widget_get_parent(ret);
	g_object_ref((GObject *) ret);
	gtk_container_remove((GtkContainer*) parent, ret);
	gtk_widget_destroy(parent);

	combo             = GTK_COMBO_BOX(gtk_builder_get_object(ui, "preset-format-combo"));
	textview          = GTK_TEXT_VIEW(gtk_builder_get_object(ui, "custom-format-text"));
	custom_format_box = GTK_BOX(gtk_builder_get_object(ui, "custom-format-box"));
	tips_box          = GTK_BOX(gtk_builder_get_object(ui, "tips-box"));
	link_button       = GTK_LINK_BUTTON(gtk_builder_get_object(ui, "link-button"));
	if (!combo || !textview || !custom_format_box || !tips_box || !link_button)
	{
		gel_error("Some widgets cannot be found:"
			" preset-format-combo(%p),"
			" custom-format-text(%p),"
			" custom-format-box(%p),"
			" tips-box(%p),",
			" link-button(%p)",
			combo, textview, custom_format_box, tips_box, link_button
			);
		goto build_preferences_widget_fail;
	}
	g_object_unref(ui);
	self->prefs_custom_box = custom_format_box;
	self->prefs_tips_box = tips_box;

	//
	// Build and fill tree model
	//
	GtkListStore *model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(combo, (GtkTreeModel *) model);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
		"markup", 1,
		NULL);

	// Insert into model and set the active one
	gint active = -1;
	gint i = 0;
	GtkTreeIter iter;
	for (i = 0; preferences_fmt_values[i][1] != NULL; i++)
	{
		/*
		gchar *l1 = gel_str_parser(preferences_fmt_values[i][0], parse_example_str_cb, NULL);
		gchar *l2 = gel_str_parser(preferences_fmt_values[i][1], parse_example_str_cb, NULL);
		gchar *str = g_str_concat(l1, "\n", l2, NULL);
		g_free(l1);
		g_free(l2);

		gchar *fmt = g_str_concat(
		*/
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter,
			0, preferences_fmt_values[i][0],
			1, preferences_fmt_values[i][1],
			-1);

		if ((active == -1) &&  preferences_fmt_values[i][0] && g_str_equal(self->stream_info_fmt,  preferences_fmt_values[i][0]))
			active = i;
	}
	// Not match, format is customized
	if (active == -1)
	{
		active = i - 1;
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(textview), self->stream_info_fmt, -1);
		gtk_widget_show((GtkWidget *) custom_format_box);
		gtk_widget_show((GtkWidget *) tips_box);
	}
	else
	{
		gtk_widget_hide((GtkWidget *) custom_format_box);
		gtk_widget_hide((GtkWidget *) tips_box);
	}
	gtk_combo_box_set_active(combo, active);

	// Signals
	g_signal_connect(combo, "changed", (GCallback) preferences_combo_box_changed_cb, self);


	return ret;

build_preferences_widget_fail:
	gel_free_and_invalidate(err, NULL, g_error_free);
	gel_free_and_invalidate(ui_path, NULL, g_free);
	gel_free_and_invalidate(ui, NULL, g_object_unref);
	gel_free_and_invalidate(ret, NULL, g_object_unref);
	gel_free_and_invalidate(parent, NULL, g_object_unref);
	return NULL;
}
#endif

static void
update_sensitiviness(EinaPlayer *self)
{
	LomoPlayer *lomo = eina_obj_get_lomo(self);

	gtk_widget_set_sensitive(self->widgets[WIDGET_PREV],   lomo_player_get_prev(lomo) >= 0);
	gtk_widget_set_sensitive(self->widgets[WIDGET_NEXT],   lomo_player_get_next(lomo) >= 0);
	gtk_widget_set_sensitive(self->widgets[WIDGET_PLAPAU], lomo_player_get_current(lomo) >= 0);
}

static void
about_show(EinaPlayer *self)
{
	static gchar *release_name = "They dont' believe";

	gchar *ui_path = gel_plugin_get_resource(eina_obj_get_plugin(self), GEL_RESOURCE_UI, "about.ui");
	if (!ui_path)
	{
		gel_warn(N_("Cannot locate resource '%s'"), "about.ui");
		return;
	}

	GtkBuilder *ui  = gtk_builder_new();
	GError     *err = NULL;
	if (gtk_builder_add_from_file(ui, ui_path, &err) == 0)
	{
		gel_warn(N_("Cannot load resource '%s': '%s'"), ui_path, err->message);
		g_error_free(err);
		g_free(ui_path);
		g_object_unref(ui);
		return;
	}
	g_free(ui_path);

	GtkAboutDialog *about;
	if ((about = GTK_ABOUT_DIALOG(gtk_builder_get_object(ui, "about-window"))) == NULL)
	{
		gel_warn(N_("Cannot get object '%s' from interface '%s'"), "about-window", "about.ui");
		g_object_unref(ui);
		return;
	}

	GdkPixbuf *pb;
	gchar *logo_path = gel_plugin_get_resource(eina_obj_get_plugin(self), GEL_RESOURCE_IMAGE, "eina.svg");
	if ((pb = gdk_pixbuf_new_from_file_at_size(logo_path, 128, 128, &err)) == NULL)
	{
		gel_warn(N_("Cannot locate resource '%s': '%s'"), "eina.svg", err->message);
		g_error_free(err);
	}
	else
		g_object_set(G_OBJECT(about), "logo", pb, NULL);

	gel_free_and_invalidate(logo_path, NULL, g_free);

	g_object_set(G_OBJECT(about), "version", PACKAGE_VERSION, NULL);

	gchar *tmp = g_strconcat("(", release_name, ")\n\n", gtk_about_dialog_get_comments(about), NULL);
	gtk_about_dialog_set_comments(about, tmp);
	g_free(tmp);

	g_signal_connect(about, "response", (GCallback) gtk_widget_destroy, NULL);
	gtk_widget_show(GTK_WIDGET(about));
}

static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self)
{
    gint x, y, width, height;

/*
	if (eina_player_get_persistent(self))
	{
		gtk_widget_hide((GtkWidget *) w);
		return TRUE;
	}
	else
	{ */
		EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);
		gtk_window_get_position((GtkWindow *)  w, &x, &y);
		gtk_window_get_size((GtkWindow *) w, &width, &height);

		eina_conf_set_int(conf, "/player/pos_x", x);
		eina_conf_set_int(conf, "/player/pos_y", y);
		eina_conf_set_int(conf, "/player/size_w", width);
		eina_conf_set_int(conf, "/player/size_h", height);

		g_object_unref(eina_obj_get_app(EINA_OBJ(self)));
		return FALSE;
/*	} */
}

static gboolean
window_keybind_handler(GdkEvent *ev, EinaPlayer *self)
{
	GError *err = NULL;
	if (ev->type != GDK_KEY_PRESS)
		return FALSE;

	switch (ev->key.keyval)
	{
	case GDK_Insert:
		eina_fs_file_chooser_load_files(eina_obj_get_lomo(self));
		break;
	case GDK_z:
	case GDK_Page_Up:
		lomo_player_go_prev(eina_obj_get_lomo(self), &err);
		break;
	case GDK_x:
	case GDK_space:
		if (lomo_player_get_state(eina_obj_get_lomo(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(eina_obj_get_lomo(self), &err);
		else
			lomo_player_play(eina_obj_get_lomo(self), &err);
		break;
	case GDK_c:
		lomo_player_stop(eina_obj_get_lomo(self), &err);
		break;
	case GDK_v:
	case GDK_Page_Down:
		lomo_player_go_next(eina_obj_get_lomo(self), &err);
		break;
	case GDK_KP_Left:
		lomo_player_seek_time(eina_obj_get_lomo(self),
			lomo_player_tell_time(eina_obj_get_lomo(self) - 10));
		break;
	case GDK_KP_Right:
		lomo_player_seek_time(eina_obj_get_lomo(self),
			lomo_player_tell_time(eina_obj_get_lomo(self) + 10));
		break;
	default:
		return FALSE;
	}
	if (err != NULL)
	{
		gel_warn("Failure in action associated with key %d: '%s'", ev->key.keyval, err->message);
		g_error_free(err);
	}
	return TRUE;
}

static void
button_clicked_cb(GtkWidget *w, EinaPlayer *self)
{
	LomoState state;
	GError *err = NULL;

	// Keep 'next' on the top of this if-else-if list since is the most common case
	if (w == self->widgets[WIDGET_NEXT])
	{
		state = lomo_player_get_state(eina_obj_get_lomo(self));
		lomo_player_go_next(eina_obj_get_lomo(self), NULL);
		if (state == LOMO_STATE_PLAY) 
			lomo_player_play(eina_obj_get_lomo(self), NULL);
	}
	
	else if (w == self->widgets[WIDGET_PREV])
	{
		state = lomo_player_get_state(eina_obj_get_lomo(self));
		lomo_player_go_prev(eina_obj_get_lomo(self), NULL);
		if (state == LOMO_STATE_PLAY)
			lomo_player_play(eina_obj_get_lomo(self), NULL);
	}
	
	else if (w == self->widgets[WIDGET_PLAPAU])
	{
		if (lomo_player_get_state(eina_obj_get_lomo(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(eina_obj_get_lomo(self), &err);
		else
			lomo_player_play(eina_obj_get_lomo(self), &err);
	}

	else if (w == self->widgets[WIDGET_OPEN])
	{
		eina_fs_file_chooser_load_files(eina_obj_get_lomo(self));
	}

	if (err != NULL)
	{
		gel_error("Got error: %s\n", err->message);
		g_error_free(err);
	}
}

static void
menu_activate_cb(GtkAction *action, EinaPlayer *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "Open"))
	{
		eina_fs_file_chooser_load_files(eina_obj_get_lomo(self));
	}

	else if (g_str_equal(name, "Help"))
	{
		GError *error = NULL;
		if (!gtk_show_uri(NULL, "http://answers.launchpad.net/eina", GDK_CURRENT_TIME, &error))
		{
			gel_error(N_("Cannot open URI http://answers.launchpad.net/eina: %s"), error->message);
			g_error_free(error);
		}
	}
	else if (g_str_equal(name, "Bug"))
	{
		GError *error = NULL;
		if (!gtk_show_uri(NULL, "https://bugs.launchpad.net/eina/+filebug", GDK_CURRENT_TIME, &error))
		{
			gel_error(N_("https://bugs.launchpad.net/eina/+filebug: %s"), error->message);
			g_error_free(error);
		}
	}
	else if (g_str_equal(name, "About"))
	{
		about_show(self);
	}
	else if (g_str_equal(name, "Quit"))
	{
		g_object_unref(eina_obj_get_app(EINA_OBJ(self)));
	}
	else
	{
		gel_warn("Unhandled action %s", name);
	}
}

#if 0
static void
preferences_combo_box_changed_cb(GtkWidget *w, EinaPlayer *self)
{
	gint idx = gtk_combo_box_get_active((GtkComboBox *) w);
	gchar *v[2] = { preferences_fmt_values[idx][0], preferences_fmt_values[idx][1] };

	if (v[0] == NULL)
	{
		gtk_widget_show((GtkWidget *) self->prefs_custom_box);
		gtk_widget_show((GtkWidget *) self->prefs_tips_box);
	}
	else
	{
		gtk_widget_hide((GtkWidget *) self->prefs_custom_box);
		gtk_widget_hide((GtkWidget *) self->prefs_tips_box);
	}

}
#endif

static void
volume_value_change_cb(GtkWidget *w, gdouble value, EinaPlayer *self)
{
	eina_conf_set_int(EINA_OBJ_GET_SETTINGS(self), "/player/volume", (value*100)/ 1);
}

/*
static void
lomo_state_change_cb (LomoPlayer *lomo, EinaPlayer *self)
{
	update_state
	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
		case LOMO_STATE_PLAY:
			switch_state(self, EINA_PLAYER_MODE_PLAY);
			break;
		case LOMO_STATE_PAUSE:
		case LOMO_STATE_STOP:
			switch_state(self, EINA_PLAYER_MODE_PAUSE);
			break;
		default:
			break;
	}
}
*/

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self)
{
	update_sensitiviness(self);
	set_info(self, lomo_player_nth_stream(lomo, to));
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self) 
{
	set_info(self, NULL);
	update_sensitiviness(self);
}

static void
lomo_all_tags_cb (LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self) 
{
	if (stream != lomo_player_get_stream(lomo))
		return;

	set_info(self, stream);
}

// stream_info_parser_cb: Helps to format stream info for display
static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream)
{
	gchar *ret = NULL;
	gchar *tag_str = lomo_stream_get_tag_by_id(stream, key);

	if (tag_str != NULL)
	{
		ret = g_markup_escape_text(tag_str, -1);
		g_free(tag_str);
	}

	if ((key == 't') && (ret == NULL))
	{
		const gchar *tmp = lomo_stream_get_tag(stream, LOMO_TAG_URI);
		gchar *tmp2 = g_uri_unescape_string(tmp, NULL);
		ret = g_path_get_basename(tmp2);
		g_free(tmp2);
	}
	return ret;
}

// --
// DnD implementation
// --
enum {
	TARGET_INT32,
	TARGET_STRING,
	TARGET_ROOTWIN
};

/* datatype (string), restrictions on DnD
 * (GtkTargetFlags), datatype (int) */
static GtkTargetEntry target_list[] = {
	// { "INTEGER",    0, TARGET_INT32 },
	{ "STRING",     0, TARGET_STRING },
	{ "text/plain", 0, TARGET_STRING },
	// { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (target_list);

static void
list_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data)
{
	EinaPlayer *self = EINA_PLAYER(data);
	GList  *uris;
	GSList *filter;

	if (!(uris = gel_io_op_result_get_object_list(res)))
	{
		gel_io_op_unref(op);
		return;
	}
	filter = gel_list_filter(uris, (GelFilterFunc) eina_fs_is_supported_file, NULL);
	if (!filter)
	{
		gel_io_op_unref(op);
		return;
	}

	GList  *lomofeed = NULL;
	GSList *iter = filter;
	while (iter)
	{
		lomofeed = g_list_prepend(lomofeed, g_file_get_uri(G_FILE(iter->data)));
		iter = iter->next;
	}
	lomofeed = g_list_reverse(lomofeed);

	g_slist_free(filter);
	gel_io_op_unref(op);

	lomo_player_clear(eina_obj_get_lomo(self));
	lomo_player_append_uri_multi(eina_obj_get_lomo(self), lomofeed);
	gel_list_deep_free(lomofeed, g_free);
}

static void
list_read_error_cb(GelIOOp *op, GFile *source, GError *err, gpointer data)
{
	gel_warn("ERror");
}

static void
drag_data_received_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	GtkSelectionData *selection_data, guint target_type, guint time,
	gpointer data)
{
	EinaPlayer *self = EINA_PLAYER(data);
	gchar *string = NULL;

	// Check for data
	if ((selection_data == NULL) || (selection_data->length < 0))
	{
		gel_warn("Got invalid selection from DnD");
		return gtk_drag_finish (context, FALSE, FALSE, time);
	}

	if (context->action != GDK_ACTION_COPY)
	{
		gel_warn("Got invalid action from DnD");
		return gtk_drag_finish (context, FALSE, FALSE, time);
	}

	/*
	if (context-> action == GDK_ACTION_ASK)
	{
		// Ask the user to move or copy, then set the context action.
	}

	if (context-> action == GDK_ACTION_MOVE)
		delete_selection_data = TRUE;
	*/

	/* Check that we got the format we can use */
	switch (target_type)
	{
	case TARGET_STRING:
		string = (gchar*) selection_data-> data;
		break;
	default:
		gel_warn("Unknow target type in DnD");
	}

	if (string == NULL)
		return gtk_drag_finish (context, FALSE, FALSE, time);


	// Parse
	gint i;
	gchar **uris = g_uri_list_extract_uris(string);
	gtk_drag_finish (context, TRUE, FALSE, time);
	GSList *files = NULL;
	for (i = 0; uris[i] && uris[i][0]; i++)
		files = g_slist_prepend(files, g_file_new_for_uri(uris[i]));

	g_strfreev(uris);
	files = g_slist_reverse(files);

	gel_io_list_read(files, "standard::*",
		list_read_success_cb, list_read_error_cb,
		self);
}

void setup_dnd(EinaPlayer *self)
{
	GtkWidget *well_dest = eina_obj_get_widget(self, "main-box");
	gtk_drag_dest_set(well_dest,
		GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_MOTION, // motion or highlight can do c00l things
		target_list,            /* lists of target to support */
		n_targets,              /* size of list */
		GDK_ACTION_COPY);
	g_signal_connect (well_dest, "drag-data-received",
		G_CALLBACK(drag_data_received_cb), self);
/*
	g_signal_connect (well_dest, "drag-leave",
		G_CALLBACK (drag_leave_handl), NULL);
	g_signal_connect (well_dest, "drag-motion",
		G_CALLBACK (drag_motion_handl), NULL);
	g_signal_connect (well_dest, "drag-drop",
		G_CALLBACK (drag_drop_handl), NULL);
*/
}

// --
// Connector 
// --
G_MODULE_EXPORT GelPlugin player_plugin = {
	GEL_PLUGIN_SERIAL,

	"player", PACKAGE_VERSION, "window" /* "preferences" */,
	NULL, NULL,

	N_("Build-in player plugin"), NULL, NULL,

	player_init, player_fini,
	NULL, NULL, NULL
};
