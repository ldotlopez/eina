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
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif
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

	#if HAVE_NOTIFY
	gboolean ntfy_enabled;
	LomoStream *ntfy_stream;
	ArtSearch  *ntfy_search;

	NotifyNotification *ntfy;
	gchar     *ntfy_summary;
	GdkPixbuf *ntfy_pixbuf;
	gchar     *ntfy_imgpath;
	#endif
} _EinaVogon;

static void
update_ui_manager(EinaVogon *self);

// --
// Lomo callbacks
// --
static void
lomo_state_change_cb(LomoPlayer *lomo, EinaVogon *self);

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

// ------------
// ------------
// Notify stuff
// ------------
// ------------
#if HAVE_NOTIFY
static void
ntfy_init(EinaVogon *self);
static void
ntfy_send(EinaVogon *self, LomoStream *stream);
static void
ntfy_update(EinaVogon *self);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaVogon *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaVogon *self);
static void
art_finish_cb(Art *art, ArtSearch *search, EinaVogon *self);
static gchar*
ntfy_str_parse_cb(gchar key, gpointer data);
#endif

/*
 * Init/Exit functions 
 */

static const gchar ui_def[] =
"<ui>"
"	<popup name='MainMenu'>"
"		<menuitem action='Play'  />"
"		<menuitem action='Pause' />"
// "		<menuitem action='Stop'  />"
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
	GtkActionGroup *ag;
	const GtkActionEntry ui_actions[] = {
		{ "Play",     GTK_STOCK_MEDIA_PLAY,     N_("Play"),     "<alt>x", "Play",            G_CALLBACK(action_activate_cb) },
		{ "Pause",    GTK_STOCK_MEDIA_PAUSE,    N_("Pause"),    "<alt>c", "Pause",           G_CALLBACK(action_activate_cb) },
		// { "Stop",     GTK_STOCK_MEDIA_STOP,     N_("Stop"),     "<alt>v", "Stop",            G_CALLBACK(action_activate_cb) },
		{ "Next",     GTK_STOCK_MEDIA_NEXT,     N_("Next"),     "<alt>b", "Next stream",     G_CALLBACK(action_activate_cb) },
		{ "Previous", GTK_STOCK_MEDIA_PREVIOUS, N_("Previous"), "<alt>z", "Previous stream", G_CALLBACK(action_activate_cb) },
		{ "Clear",    GTK_STOCK_CLEAR,          N_("C_lear"),   "<alt>l", "Clear playlist",  G_CALLBACK(action_activate_cb) },
		{ "Quit",     GTK_STOCK_QUIT,           N_("_Quit"),    "<alt>q", "Quit Eina",       G_CALLBACK(action_activate_cb) }
	};
	const GtkToggleActionEntry ui_toggle_actions[] = {
		{ "Shuffle", NULL, N_("Shuffle"), "<alt>s", "Shuffle playlist", G_CALLBACK(action_activate_cb) },
		{ "Repeat",  NULL, N_("Repeat"),  "<alt>r", "Repeat playlist",  G_CALLBACK(action_activate_cb) }
	};

	// Systray is broken on OSX using Quartz backend
	#if (defined(__APPLE__) || defined(__APPLE_CC__)) && defined __GDK_X11_H__
	g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_OSX_QUARTZ,
		N_("Gtk+ X11 backend is not supported"));
	return FALSE;
	#endif

	EinaVogon *self = g_new0(EinaVogon, 1);
	if (!eina_obj_init(EINA_OBJ(self), app, "vogon", EINA_OBJ_NONE, error))
		return FALSE;

	// Crete icon
	gchar *pathname = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, 
	#if defined(__APPLE__) || defined(__APPLE_CC__)
		"osx-systray-icon.png"
	#else
		"standard-systray-icon.png"
	#endif
		);
	self->icon = gtk_status_icon_new_from_file(pathname);
	g_free(pathname);
	if (!self->icon)
	{
		g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_NO_STATUS_ICON,
			N_("Cannot create status icon"));
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	if ((self->conf = GEL_APP_GET_SETTINGS(app)) == NULL)
	{
		g_object_unref(self->icon);
		eina_obj_fini(EINA_OBJ(self));
		g_set_error(error, vogon_quark(), EINA_VOGON_ERROR_NO_SETTINGS_OBJECT,
			N_("Object 'settings' not found"));
		return FALSE;
	}

	// Create menu
	self->ui_mng = gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_string(self->ui_mng, ui_def, -1, NULL);

	ag = gtk_action_group_new("default");
	gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
	gtk_action_group_add_toggle_actions(ag, ui_toggle_actions, G_N_ELEMENTS(ui_toggle_actions), self);

	gtk_ui_manager_insert_action_group(self->ui_mng, ag, 0);
	gtk_ui_manager_ensure_update(self->ui_mng);

	self->menu = gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu");

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat")),
		eina_conf_get_bool(self->conf, "/core/repeat", FALSE));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle")),
		eina_conf_get_bool(self->conf, "/core/random", FALSE));

	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);

	// Notify
	#if HAVE_NOTIFY
		ntfy_init(self);
	#endif

	g_signal_connect(self->icon, "popup-menu",      G_CALLBACK(popup_menu_cb), self);
	g_signal_connect(self->icon, "activate",        G_CALLBACK(status_icon_activate_cb), self);
	g_signal_connect(self->icon, "notify::destroy", G_CALLBACK(status_icon_destroy_cb), self);
	g_signal_connect(self->conf, "change",          G_CALLBACK(settings_change_cb), self);

	g_signal_connect(lomo, "play", G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(lomo, "pause", G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(lomo, "stop", G_CALLBACK(lomo_state_change_cb), self);

	if (GEL_APP_GET_PLAYER(app))
		eina_player_set_persistent(GEL_APP_GET_PLAYER(app), TRUE);

	#if defined(__APPLE__) || defined(__APPLE_CC__)
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
	g_signal_handlers_disconnect_by_func(GEL_APP_GET_LOMO(app), lomo_state_change_cb, self);
	#if HAVE_NOTIFY
	g_signal_handlers_disconnect_by_func(GEL_APP_GET_LOMO(app), lomo_change_cb, self);
	if (self->ntfy_stream)
		g_signal_handlers_disconnect_by_func(self->ntfy_stream, lomo_all_tags_cb, self);
	#endif

	// Free/unref objects
	gel_free_and_invalidate(self->icon,   NULL, g_object_unref);
	gel_free_and_invalidate(self->ui_mng, NULL, g_object_unref);
	#if HAVE_NOTIFY
	if (self->ntfy_search)
		art_cancel(GEL_APP_GET_ART(app), self->ntfy_search);
	gel_free_and_invalidate(self->ntfy, NULL, g_object_unref);
	gel_free_and_invalidate(self->ntfy_summary, NULL, g_free);
	gel_free_and_invalidate(self->ntfy_imgpath, NULL, g_free);
	gel_free_and_invalidate(self->ntfy_pixbuf, NULL, g_object_unref);
	#endif

	// Free self
	eina_obj_fini(EINA_OBJ(self));

	return TRUE;
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

static void
lomo_state_change_cb(LomoPlayer *lomo, EinaVogon *self)
{
	update_ui_manager(self);
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


#if HAVE_NOTIFY
// ------------------
// ------------------
// Notification stuff
// ------------------
// ------------------

// --
// Mini API
// --
static void
ntfy_init(EinaVogon *self)
{
	if (!notify_is_initted())
	{
		if (notify_init(PACKAGE_NAME))
			self->ntfy_enabled = TRUE;
		else
		{
			gel_error(N_("Cannot init notify library, notifications will be disabled"));
			self->ntfy_enabled = FALSE;
		}
	}
	else
		self->ntfy_enabled = TRUE;
	
	if (self->ntfy_enabled)
	{
		g_signal_connect(EINA_OBJ_GET_LOMO(self), "change",   G_CALLBACK(lomo_change_cb),   self);
		g_signal_connect(EINA_OBJ_GET_LOMO(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);
	}
}

// Sends a notification with minimal values
static void
ntfy_send(EinaVogon *self, LomoStream *stream)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail((stream != NULL) && LOMO_IS_STREAM(stream));

	if (!self->ntfy_enabled || !eina_conf_get_bool(self->conf, "/vogon/notifications", TRUE))
		return;

	// Invalidate previous resources
	if (self->ntfy_search)
		art_cancel(EINA_OBJ_GET_ART(self), self->ntfy_search);

	gel_free_and_invalidate(self->ntfy_summary, NULL, g_free);
	gel_free_and_invalidate(self->ntfy_imgpath, NULL, g_free);
	gel_free_and_invalidate(self->ntfy_pixbuf,  NULL, g_object_unref);

	// Set stream
	self->ntfy_stream = stream;

	// Set summary
	if (lomo_stream_get_all_tags_flag(stream))
		self->ntfy_summary = gel_str_parser("{%a - }%t", ntfy_str_parse_cb, stream);
	else
	{
		gchar *tmp = g_uri_unescape_string(lomo_stream_get_tag(stream, LOMO_TAG_URI), NULL);
		self->ntfy_summary = g_path_get_basename(tmp);
		g_free(tmp);
	}

	// Set cover to default and query for artwork
	self->ntfy_imgpath = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-default.png");
	self->ntfy_search  = art_search(EINA_OBJ_GET_ART(self), self->ntfy_stream, ART_FUNC(art_finish_cb), self);

	// Create and show notification
	ntfy_update(self);
}

static void
ntfy_update(EinaVogon *self)
{
	if (!self->ntfy)
		self->ntfy = notify_notification_new_with_status_icon(self->ntfy_summary, NULL, NULL, self->icon);

	if (self->ntfy_pixbuf)
		notify_notification_set_icon_from_pixbuf(self->ntfy, self->ntfy_pixbuf);
	else if (self->ntfy_imgpath)
		notify_notification_update(self->ntfy, self->ntfy_summary, NULL, self->ntfy_imgpath);

	GError *error = NULL;
	if (!notify_notification_show(self->ntfy, &error))
	{
		gel_error(N_("Cannot show notification for '%s': %s"), self->ntfy_summary, error->message);
		g_error_free(error);
	}
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaVogon *self)
{
	if (to == -1)
		return;

	ntfy_send(self, lomo_player_get_current_stream(lomo));
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaVogon *self)
{
	if (stream != self->ntfy_stream)
		return;

	gel_free_and_invalidate(self->ntfy_summary, NULL, g_free);
	self->ntfy_summary = gel_str_parser("{%a - }%t", ntfy_str_parse_cb, stream);
	ntfy_update(self);
}

static void
art_finish_cb(Art *art, ArtSearch *search, EinaVogon *self)
{
	self->ntfy_search = NULL;
	gpointer result = art_search_get_result(search);
	if (result == NULL)
		return;

	gel_free_and_invalidate(self->ntfy_pixbuf,  NULL, g_object_unref);
	gel_free_and_invalidate(self->ntfy_imgpath, NULL, g_free);

	if (GDK_IS_PIXBUF(result))
		self->ntfy_pixbuf = gdk_pixbuf_copy((GdkPixbuf *) result);
	else
		self->ntfy_imgpath = g_strdup((gchar*) result);
	
	ntfy_update(self);
}

static gchar*
ntfy_str_parse_cb(gchar key, gpointer data)
{
	LomoStream *stream = (LomoStream *) data;
	switch (key)
	{
	case 'a':
		return g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_ARTIST));
	case 't':
		return g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE));
	default:
		return NULL;
	}
}
#endif

G_MODULE_EXPORT GelPlugin vogon_plugin = {
	GEL_PLUGIN_SERIAL,
	"vogon", PACKAGE_VERSION, "lomo,settings,art",
	NULL, NULL,

	N_("Build-in systray and notification plugin"), NULL, NULL,

	vogon_init, vogon_fini,

	NULL, NULL, NULL
};

