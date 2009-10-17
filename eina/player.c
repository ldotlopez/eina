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
#define EINA_PLUGIN_DATA_TYPE EinaPlayer

// Modules
#include <eina/eina-plugin.h>
#include <eina/ext/eina-seek.h>
#include <eina/ext/eina-volume.h>
#include <eina/player.h>

#define CODENAME "They don't believe"
#define HELP_URI "http://answers.launchpad.net/eina"
#define BUGS_URI "https://bugs.launchpad.net/eina/+filebug"

struct _EinaPlayer {
	EinaObj parent;
	GtkActionGroup *action_group;
	guint merge_id;
};

static void
player_update_state(EinaPlayer *self);
static void
player_update_information(EinaPlayer *self);

static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream);

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self);
static void
volume_value_changed_cb(GtkScaleButton *w, gdouble value, EinaPlayer *self);
static void
action_activated_cb(GtkAction *action, EinaPlayer *self);

static gchar *ui_xml = 
"<ui>"
"<menubar name='Main'>"
"  <menu name='File' action='FileMenu'>"
"    <menuitem name='Open' action='open-action' />"
"    <menuitem name='Quit' action='quit-action' />"
"  </menu>"
"  <menu name='Help' action='HelpMenu'>"
"    <menuitem name='Help'  action='help-action'  />"
"    <menuitem name='Bug'   action='bug-action'   />"
"    <menuitem name='About' action='about-action' />"
"  </menu>"
"</menubar>"
"</ui>"
;

static gboolean
player_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = NULL;

	// Initialize base class
	self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "player", EINA_OBJ_GTK_UI, error))
	{
		g_free(self);
		return FALSE;
	}
	plugin->data = self;

	//
	// Setup UI bits
	//

	// Seek widget
    EinaSeek *seek = eina_seek_new();
    g_object_set(seek,
	    "lomo-player",     eina_obj_get_lomo(self),
	    "current-label",   eina_obj_get_typed(self, GTK_LABEL, "time-current-label"),
	    "remaining-label", eina_obj_get_typed(self, GTK_LABEL, "time-remaining-label"),
	    "total-label",     eina_obj_get_typed(self, GTK_LABEL, "time-total-label"),
	    NULL);

    gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "seek-container"),
		GTK_WIDGET(seek));
		gtk_widget_show(GTK_WIDGET(seek));

	// Volume widget
	EinaVolume *volume = eina_volume_new();
	eina_volume_set_lomo_player(volume, eina_obj_get_lomo(self));
	gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "volume-button-container"),
		GTK_WIDGET(volume));
		gtk_widget_show(GTK_WIDGET(volume));

	// Enable actions
	gint i;
	self->action_group = gtk_action_group_new("player");
	const gchar *actions[] = {
		"play-action", "pause-action",
		"next-action", "prev-action",
		"open-action", "quit-action",
		"help-action", "bug-action", "about-action" };
	const gchar *accels[] = {
		"", "",
		"Page_Down", "Page_Up",
		"<Control>o", "<Control>q",
		"F1", "", "" };
	for (i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GtkAction *action = eina_obj_get_typed(self, GTK_ACTION, actions[i]);
		gtk_action_group_add_action_with_accel(self->action_group, action, accels[i]);
		g_signal_connect((GObject *) action, "activate", (GCallback) action_activated_cb, self);
	
	}
	GtkUIManager *ui_mng = eina_window_get_ui_manager(eina_obj_get_window(self));
	gtk_ui_manager_insert_action_group(ui_mng, self->action_group, G_MAXINT);

	// UI Manager
	GError *err = NULL;
	if ((self->merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_xml, -1, &err)) == 0)
	{
		gel_warn(N_("Cannot merge with UI Manager: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}
	else
		gtk_ui_manager_ensure_update(ui_mng);

	// play/pause state
	player_update_state(self);

	// Information
	player_update_information(self);

	// Connect lomo signals
	g_signal_connect(volume, "value-changed", (GCallback) volume_value_changed_cb, self);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_signal_connect_swapped(lomo, "play",   (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "pause",  (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "stop",   (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "change", (GCallback) player_update_information, self);
	g_signal_connect(lomo, "all-tags", (GCallback) lomo_all_tags_cb, self);

	// Add to main window
	GtkWidget *main_widget = eina_obj_get_typed(self, GTK_WIDGET, "main-widget");
	GtkWidget *main_window = eina_obj_get_typed(self, GTK_WIDGET, "main-window");

	g_object_ref(main_widget);
	gtk_widget_unparent(main_widget);
	g_object_unref(main_window);

	eina_window_add_widget(eina_obj_get_window(self), main_widget, FALSE, TRUE, 0);
	gtk_widget_show(main_widget);
	g_object_unref(main_widget);

	return TRUE;
}

static gboolean
player_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = EINA_PLUGIN_DATA(plugin);

	eina_window_remove_widget(eina_obj_get_window(self), eina_obj_get_typed(self, GTK_WIDGET, "main-widget"));
	eina_obj_fini(EINA_OBJ(plugin->data));

	return TRUE;
}

GtkContainer *
eina_player_get_cover_container(EinaPlayer* self)
{
	return eina_obj_get_typed(self, GTK_CONTAINER, "cover-container");
}

// --
// Internal API
// --
static void
player_update_state(EinaPlayer *self)
{
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_return_if_fail(lomo != NULL);

	GtkActivatable *button = eina_obj_get_typed(self, GTK_ACTIVATABLE, "play-pause-button");
	GtkImage       *image  = eina_obj_get_typed(self, GTK_IMAGE,       "play-pause-image");

	const gchar *action = NULL;
	const gchar *stock = NULL;

	if (lomo_player_get_state(lomo) == LOMO_STATE_PLAY)
	{
		action = "pause-action";
		stock  = "gtk-media-pause";
	}
	else
	{
		action = "play-action";
		stock  = "gtk-media-play";
	}
	gtk_activatable_set_related_action(button, eina_obj_get_typed(self, GTK_ACTION, action));
	gtk_image_set_from_stock(image, stock, GTK_ICON_SIZE_BUTTON);
}

static void
player_update_information(EinaPlayer *self)
{
	gchar *info   = "<span size=\"x-large\" weight=\"bold\">Eina music player</span>\n<span size=\"x-large\" weight=\"normal\">\u200B</span>";
	gchar *markup = "<span size=\"x-large\" weight=\"bold\">%t</span>\n<span size=\"x-large\" weight=\"normal\">{%a}</span>";

	GtkWidget *label = eina_obj_get_typed(self, GTK_WIDGET, "stream-info-label");

	LomoPlayer *lomo;
	LomoStream *stream;

	if (!(lomo = eina_obj_get_lomo(self)) || !(stream = lomo_player_get_current_stream(lomo)))
	{
		g_object_set(label,
			"selectable", FALSE,
			"use-markup", TRUE,
			"label", info,
			NULL);
		gtk_window_set_title((GtkWindow *) eina_obj_get_window(self), "Eina player");
		return;
	}

	info = gel_str_parser(markup, (GelStrParserFunc) stream_info_parser_cb, stream);
	g_object_set(label,
		"selectable", TRUE,
		"use-markup", TRUE,
		"label", info,
		NULL);
	g_free(info);

	gchar *title = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE));
	if (title == NULL)
	{
		gchar *tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		title =  g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
   }

   gtk_window_set_title((GtkWindow *) eina_obj_get_window(self), title);
   g_free(title);
}

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
// Signals
// --
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self)
{
	if (stream == lomo_player_get_current_stream(lomo))
		player_update_information(self);
}

static void
volume_value_changed_cb(GtkScaleButton *w, gdouble value, EinaPlayer *self)
{
	lomo_player_set_volume(eina_obj_get_lomo(self), (gint) (value * 100));
}

static void
action_activated_cb(GtkAction *action, EinaPlayer *self)
{
	const gchar *name = gtk_action_get_name(action);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_return_if_fail(lomo != NULL);

	GError *error = NULL;
	if (g_str_equal(name, "play-action"))
		lomo_player_play(lomo, &error);

	else if (g_str_equal(name, "pause-action"))
		lomo_player_pause(lomo, &error);

	else if (g_str_equal(name, "next-action"))
		lomo_player_go_next(lomo, &error);

	else if (g_str_equal(name, "prev-action"))
		lomo_player_go_prev(lomo, &error);

	else if (g_str_equal(name, "volume-action"))
	{
	}
	
	else if (g_str_equal(name, "open-action"))
		eina_fs_file_chooser_load_files(lomo);

	else if (g_str_equal(name, "quit-action"))
		g_object_unref(eina_obj_get_app(self));

	else if (g_str_equal(name, "help-action"))
		gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, &error);

	else if (g_str_equal(name, "bug-action"))
		gtk_show_uri(NULL, BUGS_URI, GDK_CURRENT_TIME, &error);

	else if (g_str_equal(name, "about-action"))
	{
		gel_implement("About dialog is not anymore in EinaPlayer");
	}

	else
	{
		gel_warn(N_("Unknow action: %s"), name);
		return;
	}

	if (error != NULL)
	{
		gel_error(N_("Unable to complete action '%s': %s"), name, error->message);
		g_error_free(error);
	}
}
		
// --
// Connector 
// --
EINA_PLUGIN_SPEC (player,
	PACKAGE_VERSION,
	NULL,
	NULL,
	"window",

	N_("Build-in player plugin"),
	NULL,
	NULL,

	player_init, player_fini
);
