#define GEL_DOMAIN "Eina::Player"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gmodule.h>
#include <gel/gel-ui.h>
#include <config.h>
#include "base.h"
#include "lomo.h"
#include "player.h"
#include "settings.h"
#include "eina-cover.h"
#include "eina-seek.h"
#include "eina-volume.h"

#if 1
#include "eina-file-chooser-dialog.h"
#include "playlist.h"
#include "plugins.h"
#include "fs.h"
#endif

#if 1
struct _EinaPlayer {
	EinaBase parent;

	EinaConf  *conf;

	GtkWindow  *main_window;
	EinaCover  *cover;
	EinaSeek   *seek;
	EinaVolume *volume;

	GtkUIManager *ui_manager;

	GtkButton *prev, *play_pause, *next, *open;
	GtkImage  *play_pause_image;

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
static void
file_chooser_load_files(EinaPlayer *self);
static void
about_show(void);

// UI callbacks
static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);
static void
button_clicked_cb(GtkWidget *w, EinaPlayer *self);
static void
menu_activate_cb(GtkAction *action, EinaPlayer *self);
static void
cover_change_cb(EinaCover *cover, EinaPlayer *self);

// Lomo callbacks
static void lomo_state_change_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self);

// Other callbacks
static EinaFsFilterAction
fs_filter_cb(GFileInfo *info);
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
	// gtk_widget_realize(GTK_WIDGET(self->cover));
	gel_ui_container_replace_children(
		W_TYPED(self, GTK_CONTAINER, "cover-image-container"),
		GTK_WIDGET(self->cover));

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
		
		{ "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"),
	  	"<Control>p", NULL, G_CALLBACK(menu_activate_cb) },

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
		{ "main-window",       "delete-event", G_CALLBACK(main_window_delete_event_cb) },
		{ "prev-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "next-button",       "clicked", G_CALLBACK(button_clicked_cb) },
		{ "play-pause-button", "clicked", G_CALLBACK(button_clicked_cb) },
		{ "open-button",       "clicked", G_CALLBACK(button_clicked_cb) } ,
		GEL_UI_SIGNAL_DEF_NONE
	};
	gel_ui_signal_connect_from_def_multiple(UI(self), ui_signals, self, NULL);
	g_signal_connect(self->cover, "change",  G_CALLBACK(cover_change_cb),     self);
	g_signal_connect(LOMO(self), "play",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "pause",    G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "stop",     G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(LOMO(self), "change",   G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(LOMO(self), "clear",    G_CALLBACK(lomo_clear_cb), self);
	g_signal_connect(LOMO(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);

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

static void
file_chooser_load_files(EinaPlayer *self)
{
	EinaFileChooserDialog *picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	gboolean run = TRUE;
	GSList *uris;

	gtk_widget_show(GTK_WIDGET(picker));
	do
	{
		switch (gtk_dialog_run(GTK_DIALOG(picker)))
		{
		case EINA_FILE_CHOOSER_RESPONSE_PLAY:
			uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
			if (uris)
			{
				lomo_player_clear(LOMO(self));
				eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter_cb, NULL, NULL);
				lomo_player_play(LOMO(self), NULL);
				g_slist_free(uris);
			}
			run = FALSE;
			break;
					
		case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
			uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
			if (uris)
				eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter_cb, NULL, NULL);
			g_slist_free(uris);
			eina_file_chooser_dialog_set_msg(picker, EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, _("Added N streams"));
			break;

		default:
			run = FALSE;
		}

	} while (run);

	gtk_widget_destroy(GTK_WIDGET(picker));
}

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
		file_chooser_load_files(self);
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
		file_chooser_load_files(self);
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
	set_info(self, (LomoStream *) lomo_player_get_nth(LOMO(self), to));
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self) 
{
	set_info(self, NULL);
}

static void
lomo_all_tags_cb (LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self) 
{
	if (stream == lomo_player_get_stream(lomo))
		set_info(self, (LomoStream *) stream);
}

// fs_filter_cb: Filters supported types from EinaFileChooserDialog
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
#else
struct _EinaPlayer {
	EinaBase        parent;
	GtkAboutDialog *about;
	GtkUIManager   *ui_manager;
	EinaConf       *conf;
	EinaCover      *cover;
	EinaPlugins    *plugins_mng;
	EinaSeek       *seek;
	EinaVolume     *volume;

	GtkWidget *prev, *play_pause, *play_pause_image, *next, *open;
	gchar *stream_info_fmt;
};

typedef enum {
	EINA_PLAYER_MODE_PLAY,
	EINA_PLAYER_MODE_PAUSE
} EinaPlayerMode;

// --
// Internal API
// --
static void
eina_player_switch_state(EinaPlayer *self, EinaPlayerMode mode);
static void
eina_player_set_info(EinaPlayer *self, LomoStream *stream);
static void
file_chooser_load_files(EinaPlayer *self);

// --
// Backend for EinaFsFilter
// --
static EinaFsFilterAction
fs_filter_cb(GFileInfo *info);

// --
// Lomo Callbacks
// --
static void lomo_play_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_pause_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_stop_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaPlayer *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self);

// --
// UI Callbacks 
//--
static gboolean main_window_delete_event_cb
(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);
static void action_button_clicked_cb
(GtkWidget *w, EinaPlayer *self);
static void dock_expander_active_cb
(GtkWidget *wid, EinaPlayer *self);
static void
menu_activate_cb(GtkAction *action, EinaPlayer *self);

// --
// Callbacks for other eina componenets
// --
static void cover_change_cb
(GtkWidget *w, EinaPlayer *self);
static void settings_change_cb
(EinaConf *conf, gchar *key, EinaPlayer *self);

// --
// Actions definitions
// --
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

	{ "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"),
  	"<Control>p", NULL, G_CALLBACK(menu_activate_cb) },

	{ "Help", GTK_STOCK_HELP, N_("Help"),
	NULL, "About", G_CALLBACK(menu_activate_cb) },

	{ "About", GTK_STOCK_ABOUT, N_("About"),
	NULL, "About", G_CALLBACK(menu_activate_cb) }
};

// --
// Signal definitions 
// --
static GelUISignalDef _player_signals[] = {
 	{ "play-pause-button", "clicked",
	G_CALLBACK(action_button_clicked_cb) },
	{ "prev-button", "clicked",
	G_CALLBACK(action_button_clicked_cb) },
	{ "next-button", "clicked",
	G_CALLBACK(action_button_clicked_cb) },
	{ "open-button", "clicked",
	G_CALLBACK(action_button_clicked_cb) },

	{ "dock-expander", "activate",
	G_CALLBACK(dock_expander_active_cb) },
	{ "main-window", "delete-event",
	G_CALLBACK(main_window_delete_event_cb) },

	GEL_UI_SIGNAL_DEF_NONE
};

// --
// Init/Exit functions 
// --
G_MODULE_EXPORT gboolean
eina_player_init (GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlayer   *self;
	gchar *window_icon;
	gchar *cover_loading;
	gchar *cover_default;

	GtkActionGroup *ag;
	gchar *ui_manager_file;

	self = g_new0(EinaPlayer, 1);
	if (!eina_base_init((EinaBase *) self, hub, "player", EINA_BASE_GTK_UI))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	// Load settings
	if (!gel_hub_load(HUB(self), "settings"))
	{
		gel_warn("Cannot load settings");
		return FALSE;
	}
	self->conf = gel_hub_shared_get(HUB(self), "settings");

	// Insert seek 
	self->seek = (EinaSeek *) eina_seek_new();
	gtk_container_foreach(GTK_CONTAINER(W(self, "seek-hscale-container")),
		(GtkCallback) gtk_widget_hide,
		NULL);
	gtk_box_pack_start(GTK_BOX(W(self, "seek-hscale-container")),
		 GTK_WIDGET(self->seek),
		 TRUE, TRUE, 0);
	gtk_widget_show_all(GTK_WIDGET(self->seek));

	// Insert volume
	self->volume = (EinaVolume *) eina_volume_new();
	g_object_set(self->volume, "lomo-player", LOMO(self), NULL);
	gtk_container_foreach(GTK_CONTAINER(W(self, "volume-button-container")),
		(GtkCallback) gtk_widget_destroy,
		NULL);
	gtk_box_pack_start(GTK_BOX(W(self, "volume-button-container")),
		 GTK_WIDGET(self->volume),
		 TRUE, TRUE, 0);
	gtk_widget_show_all(GTK_WIDGET(self->volume));

	// Insert cover 
	self->cover = eina_cover_new();
	gtk_container_foreach(GTK_CONTAINER(W(self, "cover-image-container")),
		(GtkCallback) gtk_widget_destroy,
		NULL);
	gtk_box_pack_start(GTK_BOX(W(self, "cover-image-container")),
		 GTK_WIDGET(self->cover),
		 FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(self->cover));
	g_signal_connect(self->cover, "change",
	(GCallback) cover_change_cb, (gpointer) self);

	// Menu
	ui_manager_file = gel_app_resource_get_pathname(GEL_APP_RESOURCE_UI, "player-menu.ui");
	if (ui_manager_file != NULL)
	{
		self->ui_manager = gtk_ui_manager_new();
		if (gtk_ui_manager_add_ui_from_file(self->ui_manager, ui_manager_file, NULL) == 0)
		{
			gel_warn("Error adding UI");
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
			gtk_box_reorder_child(GTK_BOX(W(self,"main-box")),gtk_ui_manager_get_widget(self->ui_manager, "/MainMenuBar"), 0);
			gtk_widget_show_all(gtk_ui_manager_get_widget(self->ui_manager, "/MainMenuBar"));
		}
		g_free(ui_manager_file);
		gtk_window_add_accel_group(GTK_WINDOW(W(self, "main-window")),
			gtk_ui_manager_get_accel_group(self->ui_manager));
	}
	else
	{
		gel_warn("Cannot load UI Manager definition");
	}

	// Plugins Manager
	if (!gel_hub_load(hub, "plugins") || ((self->plugins_mng = gel_hub_shared_get(hub, "plugins")) == NULL))
	{
		gel_warn("Cannot load plugin manager");
	}

	// About
	{
	GtkBuilder *tmp  = gel_ui_load_resource("about", NULL);
	self->about = GTK_ABOUT_DIALOG(gtk_builder_get_object(tmp, "about-window"));
	if (self->about != NULL)
	{
		GError *err = NULL;
		gchar *logo_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "logo.png");
		GdkPixbuf *pb;
		gchar *release_name = "Codename: The return of the plugins", *tmp;

		if ((pb = gdk_pixbuf_new_from_file(logo_path, &err)) == NULL)
		{
			g_free(logo_path);
			gel_warn("Cannot find logo.png: '%s'", err->message);
			g_error_free(err);
		}
		else
		{
			g_object_set(G_OBJECT(self->about), "logo", pb, NULL);
		}

		g_object_set(G_OBJECT(self->about),
			"version", PACKAGE_VERSION,
			NULL);

		tmp = g_strconcat("(", release_name, ")\n\n",
			gtk_about_dialog_get_comments(self->about), NULL);
		gtk_about_dialog_set_comments(self->about, tmp);
		g_free(tmp);

		g_signal_connect(self->about, "delete-event",
		(GCallback) gtk_widget_hide_on_delete, NULL);
		g_signal_connect(self->about, "response",
		(GCallback) gtk_widget_hide, NULL);
	}
	else
	{
		gel_warn("Cannot load about UI");
	}
	}

	// Get references to the main widgets
	self->play_pause       = W(self, "play-pause-button");
	self->play_pause_image = W(self, "play-pause-image");
	self->prev             = W(self, "prev-button");
	self->next             = W(self, "next-button");
	self->open             = W(self, "open-button");

	// All widgets are complete at this point, setup them if needed

	// Set stream-info-label: get from conf, get from UI, set from hardcode
	self->stream_info_fmt = g_strdup(eina_conf_get_str(self->conf, "/ui/player/stream-info-fmt", NULL));
	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = g_strdup(gtk_label_get_label(GTK_LABEL(W(self, "stream-info-label"))));
	if (self->stream_info_fmt == NULL)
		self->stream_info_fmt = g_strdup(
			"<span size=\"x-large\" weight=\"bold\">%t</span>"
			"<span size=\"x-large\" weight=\"normal\">{%a}</span>");
	gel_info("Stream info label format: '%s'", self->stream_info_fmt);

	// Set seek parameters
	g_object_set(G_OBJECT(self->seek),
		"lomo-player",     LOMO(self),
		"current-label",   GTK_LABEL(W(self, "time-current-label")),
		"remaining-label", GTK_LABEL(W(self, "time-remaining-label")),
		"total-label",     GTK_LABEL(W(self, "time-total-label")),
		NULL);

	// Set cover parameters
	cover_default = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-default.png");
	cover_loading = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-loading.png");
	if ((cover_default == NULL) || (cover_loading == NULL))
		gel_warn("Some cover resources cannot be found.");

	g_object_set(G_OBJECT(self->cover),
		"lomo-player", LOMO(self),
		"default-cover", cover_default,
		"loading-cover", cover_loading,
		NULL);
	gel_free_and_invalidate(cover_default, NULL, g_free);
	gel_free_and_invalidate(cover_loading, NULL, g_free);

	// Set play/pause state
	if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
		eina_player_switch_state(self, EINA_PLAYER_MODE_PLAY);
	else
		eina_player_switch_state(self, EINA_PLAYER_MODE_PAUSE);

	// Update info from LomoStream
	window_icon = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "icon.png");
	gtk_window_set_default_icon_from_file(window_icon, NULL);
	g_free(window_icon);

	eina_player_set_info(self, (LomoStream *) lomo_player_get_stream(LOMO(self)));

	// Make player-ebox a dropable widget
	/*
	if (!gtk_ext_make_widget_dropable(W(self, "player-ebox"), NULL))
		e_warn("Cannot make infobox widget dropable");
	*/

	// Setup signals
	g_signal_connect(LOMO(self), "play",
		G_CALLBACK(lomo_play_cb), self);
	g_signal_connect(LOMO(self), "pause",
		G_CALLBACK(lomo_pause_cb), self);
	g_signal_connect(LOMO(self), "stop",
		G_CALLBACK(lomo_stop_cb), self);
	g_signal_connect(LOMO(self), "change",
		G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(LOMO(self), "clear",
		G_CALLBACK(lomo_clear_cb), self);
	g_signal_connect(LOMO(self), "all-tags",
		G_CALLBACK(lomo_all_tags_cb), self);
	g_signal_connect(self->conf, "change",
		G_CALLBACK(settings_change_cb), self);
	gel_ui_signal_connect_from_def_multiple(UI(self), _player_signals, self, NULL);

	// Set LomoPlayer volume
	lomo_player_set_volume(LOMO(self), eina_conf_get_int(self->conf, "/core/volume", 50));

	gtk_widget_show(W(self, "main-window"));

	// Accelerators
	GtkAccelGroup *accel_group = NULL;
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(W(self, "main-window")), accel_group);

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_player_exit
(gpointer data)
{
	EinaPlayer *self = (EinaPlayer *) data;

	g_free(self->stream_info_fmt);

	// Save settings
	eina_conf_set_int(self->conf,
		"/core/volume",
		lomo_player_get_volume(LOMO(self)));

	// Unref components
	gel_hub_unload(HUB(self), "settings");

	// Free base
	eina_base_fini((EinaBase *) self);

	return TRUE;
}

// --
// External API
// --
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
	return GTK_WINDOW(W(self, "main-window"));
}

// --
// Internal API
// --
static void
eina_player_switch_state(EinaPlayer *self, EinaPlayerMode mode)
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
	gtk_widget_set_tooltip_text(W(self, "play-pause-button"), tooltip);
	gtk_image_set_from_stock(GTK_IMAGE(self->play_pause_image),
		stock, GTK_ICON_SIZE_BUTTON);
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

static void
eina_player_set_info(EinaPlayer *self, LomoStream *stream)
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
			_("<span size=\"x-large\" weight=\"bold\">Eina music Player</span>")
			);
		return;
	}

	stream_info = gel_str_parser(self->stream_info_fmt, (GelStrParserFunc) stream_info_parser_cb, stream);
	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info-label")), stream_info);
	g_free(stream_info);

	if ((title = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE))) == NULL)
	{
		tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		title =  g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
	}

	gtk_window_set_title(GTK_WINDOW(W(self,"main-window")), title);
	g_free(title);
}

static void
file_chooser_load_files(EinaPlayer *self)
{
	EinaFileChooserDialog *picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	gboolean run = TRUE;
	GSList *uris;

	gtk_widget_show(GTK_WIDGET(picker));
	do
	{
		switch (gtk_dialog_run(GTK_DIALOG(picker)))
		{
		case EINA_FILE_CHOOSER_RESPONSE_PLAY:
			uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
			if (uris)
			{
				lomo_player_clear(LOMO(self));
				eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter_cb, NULL, NULL);
				lomo_player_play(LOMO(self), NULL);
				g_slist_free(uris);
			}
			run = FALSE;
			break;
					
		case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
			uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
			if (uris)
				eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter_cb, NULL, NULL);
			g_slist_free(uris);
			eina_file_chooser_dialog_set_msg(picker, EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, _("Added N streams"));
			break;

		default:
			run = FALSE;
		}

	} while (run);

	gtk_widget_destroy(GTK_WIDGET(picker));
}

// --
// Callbacks
// --

// fs_filter_cb: Filters supported types from EinaFileChooserDialog
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

// --
// Lomo callbacks
// --
static void
lomo_play_cb (LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state(self, EINA_PLAYER_MODE_PLAY);
}

static void
lomo_pause_cb(LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state(self, EINA_PLAYER_MODE_PAUSE);
}

static void
lomo_stop_cb (LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state(self, EINA_PLAYER_MODE_PAUSE);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self)
{
	eina_player_set_info(self, (LomoStream *) lomo_player_get_nth(LOMO(self), to));
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self) 
{
	eina_player_set_info(self, NULL);
}

static void
lomo_all_tags_cb (LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self) 
{
	if (stream == lomo_player_get_stream(lomo))
		eina_player_set_info(self, (LomoStream *) stream);
}

// --
// UI Callbacks
// --
static gboolean
main_window_delete_event_cb(GtkWidget *w, GdkEvent *ev, EinaPlayer *self)
{
	GtkWindow *window;
    gint x, y, width, height;

	window = GTK_WINDOW(W(self, "main-window"));

	gtk_window_get_position(window, &x, &y);
	gtk_window_get_size(window, &width, &height);

	eina_conf_set_int(self->conf, "/ui/pos_x", x);
	eina_conf_set_int(self->conf, "/ui/pos_y", y);
	eina_conf_set_int(self->conf, "/ui/size_w", width);
	eina_conf_set_int(self->conf, "/ui/size_h", height);

	g_object_unref(HUB(self));
	return FALSE;
}

static void
action_button_clicked_cb(GtkWidget *w, EinaPlayer *self)
{
	LomoState state;
	GError *err = NULL;

	// Keep 'next' on the top of this if-else-if list since is the most common case
	if (w == self->next)
	{
		state = lomo_player_get_state(LOMO(self));
		lomo_player_go_next(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY) 
			lomo_player_play(LOMO(self), NULL);
	}
	
	else if (w == self->prev)
	{
		state = lomo_player_get_state(LOMO(self));
		lomo_player_go_prev(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY)
			lomo_player_play(LOMO(self), NULL);
	}
	
	else if (w == self->play_pause)
	{
		if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(LOMO(self), &err);
		else
			lomo_player_play(LOMO(self), &err);
	}

	else if (w == self->open)
	{
		file_chooser_load_files(self);
	}

	if (err != NULL)
	{
		gel_error("Got error: %s\n", err->message);
		g_error_free(err);
	}
}

static void
dock_expander_active_cb(GtkWidget *wid, EinaPlayer *self)
{
	GtkWindow *win;
	gint w, h;
	gboolean expanded;

	expanded = !gtk_expander_get_expanded(GTK_EXPANDER(W(self, "dock-expander")));

	win = GTK_WINDOW(W(self, "main-window"));
	gtk_window_get_size(win, &w, &h);
	gel_info("Current size: %dx%d", w, h);
	if (expanded) {
		gtk_window_set_resizable(win, TRUE);
	}
	else {
		gtk_window_set_resizable(win, FALSE);
	}
	eina_conf_set_bool(self->conf, "/player/playlist_expanded", expanded);
}

void
menu_activate_cb(GtkAction *action, EinaPlayer *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "Open"))
	{
		file_chooser_load_files(self);
	}

	else if (g_str_equal(name, "Help"))
	{
		g_spawn_command_line_async("gnome-open http://eina.sourceforge.net/help", NULL);
	}
	else if (g_str_equal(name, "About") && self->about)
	{
		gtk_widget_show(GTK_WIDGET(self->about));
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

#if 0
static void on_ebox_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlayer       *self)
{
    
	gel_info("Got DnD");

	if (TRUE) return;
	gchar **uris;
	gint i;
	gchar *filename;
	GSList *uri_list = NULL, *uri_scan, *uri_filter;

	/* Get the uri list from DnD and convert it to a
	 * file:///path/to/filename GSList
	 *
	 * This list (uri_list) has strings on a newly allocated memory
	 */
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

	uri_filter = eina_fs_filter_cb_filter(self->stream_filter, // Filter
		uri_scan,
		TRUE, FALSE, FALSE,
		FALSE, FALSE);

	lomo_player_clear(LOMO(self));
	lomo_player_add_uri_multi(LOMO(self), (GList *) uri_filter);

	g_slist_free(uri_filter);
	g_ext_slist_free(uri_scan, g_free);
}
#endif

// --
// Cover callbacks
// --
static void
cover_change_cb(GtkWidget *w, EinaPlayer *self)
{
	gchar *iconfile;

	if (gtk_image_get_storage_type(GTK_IMAGE(w)) == GTK_IMAGE_PIXBUF)
		gtk_window_set_icon(GTK_WINDOW(W(self, "main-window")), gtk_image_get_pixbuf(GTK_IMAGE(w)));
	else
	{
		iconfile = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "icon.png");
		gtk_window_set_icon_from_file(GTK_WINDOW(W(self, "main-window")), iconfile, NULL);
		g_free(iconfile);
	}
}

// --
// Settings callbacks
// --
static void
settings_change_cb(EinaConf *conf, gchar *key, EinaPlayer *self)
{
	GtkWindow *win;
	gint x, y;

	win = GTK_WINDOW(W(self, "main-window"));
	if (g_str_equal(key, "/ui/pos_x")) {
		gtk_window_get_position(win, &x, &y);
		gtk_window_move(win, eina_conf_get_int(conf, key, 0), y);
	}
	else if (g_str_equal(key, "/ui/pos_y")) {
		gtk_window_get_position(win, &x, &y);
		gtk_window_move(win, x, eina_conf_get_int(conf, key, 0));
	}
}
#endif
// --
// Connector 
// --
G_MODULE_EXPORT GelHubSlave player_connector = {
	"player",
	&eina_player_init,
	&eina_player_exit,
};

