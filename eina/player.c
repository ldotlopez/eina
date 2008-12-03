#define GEL_DOMAIN "Eina::Player"

#include <config.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <glib.h>
#include <gmodule.h>
#include <gdk/gdkkeysyms.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>
#include "base.h"
#include "lomo.h"
#include "player.h"
#include "settings.h"
#include "preferences.h"
#include "eina-cover.h"
#include "eina-seek.h"
#include "eina-volume.h"
#include "eina-file-chooser-dialog.h"
#include "playlist.h"
#include "plugins.h"
#include "fs.h"

struct _EinaPlayer {
	EinaBase parent;

	EinaConf  *conf;

	GtkWindow  *main_window;
	EinaCover  *cover;
	EinaSeek   *seek;
	EinaVolume *volume;

	GtkUIManager *ui_manager;

	GtkButton   *prev, *play_pause, *next, *open;
	GtkImage    *play_pause_image;

	gchar *stream_info_fmt;
};

typedef enum {
	EINA_PLAYER_MODE_PLAY,
	EINA_PLAYER_MODE_PAUSE
} EinaPlayerMode;

// API
static void
switch_state(EinaPlayer *self, EinaPlayerMode mode);
static void
set_info(EinaPlayer *self, LomoStream *stream);
static GtkWidget*
build_preferences_widget(EinaPlayer *self);
static void
update_sensitiviness(EinaPlayer *self);
static void
about_show(void);

// UI callbacks
static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);
static gboolean
main_window_window_state_event_cb(GtkWidget *w, GdkEventWindowState *event, EinaPlayer *self);
static gboolean
main_box_key_press_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);
static void
button_clicked_cb(GtkWidget *w, EinaPlayer *self);
static void
menu_activate_cb(GtkAction *action, EinaPlayer *self);
static void
cover_change_cb(EinaCover *cover, EinaPlayer *self);

// Lomo callbacks
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self);

// Callback for parse stream info string
static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream);

G_MODULE_EXPORT gboolean
eina_player_init (GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlayer *self = NULL;
	EinaConf   *conf = NULL;

	// Load some pre-requisites
	if (!gel_hub_load(hub, "settings"))
	{
		gel_error("Cannot load settings");
		return FALSE;
	}
	if ((conf = gel_hub_shared_get(hub, "settings")) == NULL)
	{
		gel_error("Cannot access settings");
		return FALSE;
	}

	// Initialize base class
	self = g_new0(EinaPlayer, 1);
	if (!eina_base_init((EinaBase *) self, hub, "player", EINA_BASE_GTK_UI))
	{
		gel_error("Cannot create EinaPlayer");
		g_free(self);
		return FALSE;
	}
	self->conf = conf;

	// Set stream-info-label: get from conf, get from UI, set from hardcode
	self->stream_info_fmt = g_strdup(eina_conf_get_str(self->conf, "/ui/player/stream-info-fmt", NULL));
	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = g_strdup(gtk_label_get_label(GTK_LABEL(W(self, "stream-info-label"))));
	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = g_strdup(
			"<span size=\"x-large\" weight=\"bold\">%t</span>"
			"<span size=\"x-large\" weight=\"normal\">{%a}</span>");

	self->main_window = W_TYPED(self, GTK_WINDOW, "main-window");
	self->prev = W_TYPED(self, GTK_BUTTON, "prev-button");
	self->next = W_TYPED(self, GTK_BUTTON, "next-button");
	self->open = W_TYPED(self, GTK_BUTTON, "open-button");
	self->play_pause       = W_TYPED(self, GTK_BUTTON, "play-pause-button");
	self->play_pause_image = W_TYPED(self, GTK_IMAGE,  "play-pause-image");

	if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
		switch_state(self, EINA_PLAYER_MODE_PLAY);
	else
		switch_state(self, EINA_PLAYER_MODE_PAUSE);

	set_info(self, (LomoStream *) lomo_player_get_stream(LOMO(self)));

	gtk_widget_realize(GTK_WIDGET(self->main_window));

	// Initialize volume
	self->volume = eina_volume_new();
	eina_volume_set_lomo_player(self->volume, EINA_BASE_GET_LOMO(self));
	gel_ui_container_replace_children(
		W_TYPED(self, GTK_CONTAINER, "volume-button-container"),
		GTK_WIDGET(self->volume));
	gtk_widget_show(GTK_WIDGET(self->volume));

	// Initialize seek
	self->seek = eina_seek_new();
	eina_seek_set_lomo_player(self->seek, EINA_BASE_GET_LOMO(self));
	eina_seek_set_current_label  (self->seek, W_TYPED(self, GTK_LABEL, "time-current-label"));
	eina_seek_set_remaining_label(self->seek, W_TYPED(self, GTK_LABEL, "time-remaining-label"));
	eina_seek_set_total_label    (self->seek, W_TYPED(self, GTK_LABEL, "time-total-label"));
	gel_ui_container_replace_children(
		W_TYPED(self, GTK_CONTAINER, "seek-hscale-container"),
		GTK_WIDGET(self->seek));
	gtk_widget_show(GTK_WIDGET(self->seek));

	// Initialize cover
	gchar *default_cover_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-default.png");
	gchar *loading_cover_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-loading.png");
	self->cover = eina_cover_new();
	gtk_widget_set_size_request(GTK_WIDGET(self->cover), W(self,"cover-image-container")->allocation.height, W(self,"cover-image-container")->allocation.height);
	gtk_widget_show(GTK_WIDGET(self->cover));
	gel_ui_container_replace_children(
		W_TYPED(self, GTK_CONTAINER, "cover-image-container"),
		GTK_WIDGET(self->cover));
	g_signal_connect(self->cover, "change",  G_CALLBACK(cover_change_cb),     self);

	eina_cover_set_default_cover(self->cover, default_cover_path);
	eina_cover_set_loading_cover(self->cover, loading_cover_path);
	eina_cover_set_lomo_player  (self->cover, LOMO(self));
	g_free(default_cover_path);
	g_free(loading_cover_path);

	// Initialize UI Manager
	GError *err = NULL;
	GtkActionGroup *ag;
	GtkActionEntry ui_actions[] = {
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


		{ "Help", GTK_STOCK_HELP, N_("Help"),
		NULL, "About", G_CALLBACK(menu_activate_cb) },
		{ "About", GTK_STOCK_ABOUT, N_("About"),
		NULL, "About", G_CALLBACK(menu_activate_cb) }
	};
	gchar *ui_manager_file = gel_app_resource_get_pathname(GEL_APP_RESOURCE_UI, "player-menu.ui");
	if (ui_manager_file != NULL)
	{
		self->ui_manager = gtk_ui_manager_new();
		if (gtk_ui_manager_add_ui_from_file(self->ui_manager, ui_manager_file, &err) == 0)
		{
			gel_warn("Error adding UI to UI Manager: '%s'", err->message);
			g_error_free(err);
		}
		else
		{
			ag = gtk_action_group_new("default");
			gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
			gtk_ui_manager_insert_action_group(self->ui_manager, ag, 0);
			gtk_ui_manager_ensure_update(self->ui_manager);
			gtk_box_pack_start(
				GTK_BOX(W(self,"main-box")), 
				gtk_ui_manager_get_widget(self->ui_manager, "/MainMenuBar"),
				FALSE,FALSE, 0);
			gtk_box_reorder_child(GTK_BOX(W(self,"main-box")), gtk_ui_manager_get_widget(self->ui_manager, "/MainMenuBar"), 0);
			gtk_widget_show_all(gtk_ui_manager_get_widget(self->ui_manager, "/MainMenuBar"));
		}
		g_free(ui_manager_file);
		gtk_window_add_accel_group(
			self->main_window,
			gtk_ui_manager_get_accel_group(self->ui_manager));
	}

	// Connect signals
	GelUISignalDef ui_signals[] = {
		{ "main-window",       "delete-event",       G_CALLBACK(main_window_delete_event_cb) },
		{ "main-window",       "window-state-event", G_CALLBACK(main_window_window_state_event_cb) },
		{ "main-box",          "key-press-event",    G_CALLBACK(main_box_key_press_event_cb) },
		{ "prev-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "next-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "play-pause-button", "clicked", G_CALLBACK(button_clicked_cb) },
		{ "open-button",       "clicked", G_CALLBACK(button_clicked_cb) } ,
		GEL_UI_SIGNAL_DEF_NONE
	};
	gel_ui_signal_connect_from_def_multiple(UI(self), ui_signals, self, NULL);
	g_signal_connect(LOMO(self), "play",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "pause",    G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "stop",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "change",   G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(LOMO(self), "clear",    G_CALLBACK(lomo_clear_cb), self);
	g_signal_connect(LOMO(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);
	g_signal_connect_swapped(LOMO(self), "del",    G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(LOMO(self), "add",    G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(LOMO(self), "repeat", G_CALLBACK(update_sensitiviness), self);
	g_signal_connect_swapped(LOMO(self), "random", G_CALLBACK(update_sensitiviness), self);

	// Preferences is attached to us (like dock) but this is less than optimal
	if (!gel_hub_load(hub, "preferences"))
	{
		gel_warn("Cannot load preferences component");
	}
	else
	{
		EinaPreferencesDialog *prefs = EINA_PREFERENCES_DIALOG(gel_hub_shared_get(hub, "preferences"));
		eina_preferences_dialog_add_tab(prefs,
			GTK_IMAGE(gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON)),
			GTK_LABEL(gtk_label_new(_("Player"))),
			build_preferences_widget(self));
	}

	// Show it
	gtk_widget_show(GTK_WIDGET(self->main_window));

	return TRUE;
}

G_MODULE_EXPORT gboolean
eina_player_exit (gpointer data)
{
	EinaPlayer *self = (EinaPlayer *) data;

	g_free(self->stream_info_fmt);
	gel_hub_unload(HUB(self), "settings");
	eina_base_fini((EinaBase *) self);
	return TRUE;
}

EinaCover *
eina_player_get_cover(EinaPlayer *self)
{
	return self->cover;
}

GtkUIManager *
eina_player_get_ui_manager(EinaPlayer *self)
{
	return self->ui_manager;
}

GtkWindow *
eina_player_get_main_window(EinaPlayer *self)
{
	return self->main_window;
}

void
eina_player_add_widget(EinaPlayer* self, GtkWidget *widget)
{
	gtk_box_pack_start(W_TYPED(self, GTK_BOX, "widgets-box"), widget,
		TRUE, TRUE, 0);
}

void
eina_player_remove_widget(EinaPlayer* self, GtkWidget *widget)
{
	 gtk_container_remove(W_TYPED(self, GTK_CONTAINER, "widgets-box"), widget);
}

static void
switch_state(EinaPlayer *self, EinaPlayerMode mode)
{
	gchar *tooltip = NULL;
	gchar *stock   = NULL;

	switch (mode)
	{
	case EINA_PLAYER_MODE_PLAY:
		tooltip = _("Pause current song");
		stock = "gtk-media-pause";
		break;

	case EINA_PLAYER_MODE_PAUSE:
		tooltip = _("Play current song");
		stock = "gtk-media-play";
		break;

	default:
		gel_warn("Unknow EinaPlayerMode %d", mode);
		return;
	}
	gtk_widget_set_tooltip_text(GTK_WIDGET(self->play_pause), tooltip);
	gtk_image_set_from_stock(self->play_pause_image,
		stock, GTK_ICON_SIZE_BUTTON);
}

static void
set_info(EinaPlayer *self, LomoStream *stream)
{
	gchar *tmp, *title, *stream_info;

	// No stream, reset
	if (stream == NULL)
	{
		gtk_window_set_title(
			GTK_WINDOW(W(self, "main-window")),
			_("Eina music player"));
		gtk_label_set_markup(
			GTK_LABEL(W(self, "stream-info-label")),
			_("<span size=\"x-large\" weight=\"bold\">Eina music player</span>\n<span size=\"x-large\" weight=\"normal\">\u200B</span>")
			);
		gtk_label_set_selectable(GTK_LABEL(W(self, "stream-info-label")), FALSE);
		return;
	}

	stream_info = gel_str_parser(self->stream_info_fmt, (GelStrParserFunc) stream_info_parser_cb, stream);
	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info-label")), stream_info);
	g_free(stream_info);
	gtk_label_set_selectable(GTK_LABEL(W(self, "stream-info-label")), TRUE);

	if ((title = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE))) == NULL)
	{
		tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		title =  g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
	}

	gtk_window_set_title(self->main_window, title);
	g_free(title);
}

static GtkWidget*
build_preferences_widget(EinaPlayer *self)
{
	return gtk_label_new(":)");
}

static void
update_sensitiviness(EinaPlayer *self)
{
	gtk_widget_set_sensitive(GTK_WIDGET(self->prev),
		(lomo_player_get_prev(LOMO(self)) >= 0));
	gtk_widget_set_sensitive(GTK_WIDGET(self->next),
		(lomo_player_get_next(LOMO(self)) >= 0));
	gtk_widget_set_sensitive(GTK_WIDGET(self->play_pause),
		(lomo_player_get_current(LOMO(self)) >= 0));
}
#if 0
static void
file_chooser_load_files(EinaPlayer *self)
{
	EinaFileChooserDialog *picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);

	gboolean run = TRUE;
	GSList *uris;

	do
	{
		run = FALSE;
		gint response = eina_file_chooser_dialog_run(picker);

		switch (response)
		{
		case EINA_FILE_CHOOSER_RESPONSE_PLAY:
			uris = eina_file_chooser_dialog_get_uris(picker);
			if (uris == NULL) // Shortcircuit
			{
				run = TRUE; // Keep alive
				break;
			}
			lomo_player_clear(LOMO(self));
			lomo_player_add_uri_multi(LOMO(self), (GList *) uris);
			lomo_player_play(LOMO(self), NULL);
			run = FALSE; // Destroy
			break;

		case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
			uris = eina_file_chooser_dialog_get_uris(picker);
			if (uris == NULL)
			{
				run = TRUE; // Keep alive
				break;
			}
			gchar *msg = g_strdup_printf(_("Loaded %d streams"), g_slist_length(uris));
			eina_file_chooser_dialog_set_msg(picker, EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, msg);
			g_free(msg);

			lomo_player_add_uri_multi(LOMO(self), (GList *) uris);
			run = TRUE; // Keep alive
			break;

		default:
			run = FALSE; // Destroy
			break;
		}
	} while (run);

	gtk_widget_destroy(GTK_WIDGET(picker));
}
#endif

static void
about_show(void)
{
	GtkBuilder     *ui;
	GtkAboutDialog *about;

	gchar *logo_path;
	GdkPixbuf  *pb;

	static gchar *release_name = "Black and blue";
	gchar *tmp;

	GError *err = NULL;

	if ((ui = gel_ui_load_resource("about", &err)) == NULL)
	{
		gel_error("Cannot load ui for about: '%s'", err->message);
		g_error_free(err);
		return;
	}
	if ((about = GTK_ABOUT_DIALOG(gtk_builder_get_object(ui, "about-window"))) == NULL)
	{
		gel_error("Interface definition doesnt have a about-window widget");
		return;
	}
	g_object_unref(ui);

	logo_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "logo.png");
	if ((pb = gdk_pixbuf_new_from_file(logo_path, &err)) == NULL)
	{
		gel_warn("Cannot find logo.png: '%s'", err->message);
		g_error_free(err);
	}
	else
	{
		g_object_set(G_OBJECT(about), "logo", pb, NULL);
	}
	gel_free_and_invalidate(logo_path, NULL, g_free);

	g_object_set(G_OBJECT(about), "version", PACKAGE_VERSION, NULL);

	tmp = g_strconcat("(", release_name, ")\n\n",
		gtk_about_dialog_get_comments(about), NULL);
	gtk_about_dialog_set_comments(about, tmp);
	g_free(tmp);

	g_signal_connect(about, "response",
	(GCallback) gtk_widget_destroy, NULL);

	gtk_widget_show(GTK_WIDGET(about));
}

static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self)
{
    gint x, y, width, height;

	gtk_window_get_position(self->main_window, &x, &y);
	gtk_window_get_size(self->main_window, &width, &height);

	eina_conf_set_int(self->conf, "/ui/pos_x", x);
	eina_conf_set_int(self->conf, "/ui/pos_y", y);
	eina_conf_set_int(self->conf, "/ui/size_w", width);
	eina_conf_set_int(self->conf, "/ui/size_h", height);

	g_object_unref(HUB(self));

	return FALSE;
}

static gboolean
main_window_window_state_event_cb(GtkWidget *w, GdkEventWindowState *event, EinaPlayer *self)
{
	// GdkWindowState mask = GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_MAXIMIZED;
	return FALSE;
}

static gboolean
main_box_key_press_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self)
{
	GError *err = NULL;
	if (ev->type != GDK_KEY_PRESS)
		return FALSE;

	switch (ev->key.keyval)
	{
	case GDK_Insert:
		eina_fs_file_chooser_load_files(LOMO(self));
		// file_chooser_load_files(self);
		break;
	case GDK_z:
	case GDK_Page_Up:
		lomo_player_go_prev(LOMO(self), &err);
		break;
	case GDK_x:
		if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(LOMO(self), &err);
		else
			lomo_player_play(LOMO(self), &err);
		break;
	case GDK_c:
		lomo_player_stop(LOMO(self), &err);
		break;
	case GDK_v:
	case GDK_Page_Down:
		lomo_player_go_next(LOMO(self), &err);
		break;
	case GDK_KP_Left:
		lomo_player_seek_time(LOMO(self),
			lomo_player_tell_time(LOMO(self) - 10));
		break;
	case GDK_KP_Right:
		lomo_player_seek_time(LOMO(self),
			lomo_player_tell_time(LOMO(self) + 10));
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
	if (w == GTK_WIDGET(self->next))
	{
		state = lomo_player_get_state(LOMO(self));
		lomo_player_go_next(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY) 
			lomo_player_play(LOMO(self), NULL);
	}
	
	else if (w == GTK_WIDGET(self->prev))
	{
		state = lomo_player_get_state(LOMO(self));
		lomo_player_go_prev(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY)
			lomo_player_play(LOMO(self), NULL);
	}
	
	else if (w == GTK_WIDGET(self->play_pause))
	{
		if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(LOMO(self), &err);
		else
			lomo_player_play(LOMO(self), &err);
	}

	else if (w == GTK_WIDGET(self->open))
	{
		eina_fs_file_chooser_load_files(LOMO(self));
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
		eina_fs_file_chooser_load_files(LOMO(self));
	}

	else if (g_str_equal(name, "Help"))
	{
		g_spawn_command_line_async("gnome-open http://eina.sourceforge.net/help", NULL);
	}
	else if (g_str_equal(name, "About"))
	{
		about_show();
	}
	else if (g_str_equal(name, "Quit"))
	{
		g_object_unref(HUB(self));
	}
	else
	{
		gel_warn("Unhandled action %s", name);
	}
}

static void
cover_change_cb(EinaCover *cover, EinaPlayer *self)
{
	if (gtk_image_get_storage_type(GTK_IMAGE(cover)) != GTK_IMAGE_PIXBUF)
	{
		gel_warn("Unhandled storage type for image");
		return;
	}
	gtk_window_set_icon(self->main_window, gtk_image_get_pixbuf(GTK_IMAGE(cover)));
}

static void
lomo_state_change_cb (LomoPlayer *lomo, EinaPlayer *self)
{
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

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self)
{
	update_sensitiviness(self);
	set_info(self, (LomoStream *) lomo_player_get_nth(LOMO(self), to));
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self) 
{
	set_info(self, NULL);
	update_sensitiviness(self);
}

static void
lomo_all_tags_cb (LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self) 
{
	if (stream == lomo_player_get_stream(lomo))
		set_info(self, (LomoStream *) stream);
}

// fs_filter_cb: Filters supported types from EinaFileChooserDialog
#if 0
static EinaFsFilterAction
fs_filter_cb(GFileInfo *info)
{
	static const gchar *suffixes[] = {".mp3", ".ogg", ".wma", ".aac", ".flac", NULL };
	gchar *lc_name;
	gint i;
	EinaFsFilterAction ret = EINA_FS_FILTER_REJECT;

	if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
		return  EINA_FS_FILTER_ACCEPT;

	lc_name = g_utf8_strdown(g_file_info_get_name(info), -1);
	for (i = 0; suffixes[i] != NULL; i++) {
		if (g_str_has_suffix(lc_name, suffixes[i])) {
			ret = EINA_FS_FILTER_ACCEPT;
			break;
		}
	}

	g_free(lc_name);
	return ret;
}
#endif

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
// Connector 
// --
G_MODULE_EXPORT GelHubSlave player_connector = {
	"player",
	&eina_player_init,
	&eina_player_exit,
};
