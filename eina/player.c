#define GEL_DOMAIN "Eina::Player"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gmodule.h>
#include <gel/gel-ui.h>
#include "base.h"
#include "player.h"
#include "eina-seek.h"
#include "eina-volume.h"
#include "eina-cover.h"
#include "eina-file-chooser-dialog.h"
#include "playlist.h"
#include "settings.h"
#include "fs.h"

struct _EinaPlayer {
	EinaBase    parent;
	EinaCover  *cover;
	EinaSeek   *seek;
	EinaVolume *volume;

	EinaConf         *conf;

	GtkWidget *prev, *play_pause, *play_pause_image, *next, *open;

	gchar *stream_info_fmt;
};

// --
// Internal API
// --
static void
eina_player_set_info(EinaPlayer *self, LomoStream *stream);
static void
eina_player_switch_state_play(EinaPlayer *self);
static void
eina_player_switch_state_pause(EinaPlayer *self);

// --
// Backend for EinaFsFilter
// --
static EinaFsFilterAction
fs_filter(GFileInfo *info);

// --
// Lomo Callbacks
// --
static void on_lomo_play
(LomoPlayer *lomo, EinaPlayer *self);

static void on_lomo_pause
(LomoPlayer *lomo, EinaPlayer *self);

static void on_lomo_stop
(LomoPlayer *lomo, EinaPlayer *self);

static void on_lomo_clear
(LomoPlayer *lomo, EinaPlayer *self);

static void on_lomo_change
(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);

static void on_lomo_all_tags
(LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self);

// --
// UI Callbacks 
//--
static gboolean on_main_window_delete_event
(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);

static void on_any_button_clicked
(GtkWidget *w, EinaPlayer *self);

static void on_dock_expander_activate
(GtkWidget *wid, EinaPlayer *self);

static void on_ebox_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlayer       *self);

// --
// Callbacks for other eina componenets
// --
static void on_settings_change
(EinaConf *conf, gchar *key, EinaPlayer *self);

// --
// Signal definitions 
// --
static GelUISignalDef _player_signals[] = {
 	{ "play-pause-button", "clicked",
	G_CALLBACK(on_any_button_clicked) },
	{ "prev-button", "clicked",
	G_CALLBACK(on_any_button_clicked) },
	{ "next-button", "clicked",
	G_CALLBACK(on_any_button_clicked) },
	{ "open-button", "clicked",
	G_CALLBACK(on_any_button_clicked) },

	/* Misc */
	{ "player-ebox", "drag-data-received",
	G_CALLBACK(on_ebox_drag_data_received) },

	{ "dock-expander", "activate",
	G_CALLBACK(on_dock_expander_activate) },
	{ "main-window", "delete-event",
	G_CALLBACK(on_main_window_delete_event) },

	GEL_UI_SIGNAL_DEF_NONE
};

// --
// Init/Exit functions 
// --
G_MODULE_EXPORT gboolean
eina_player_init (GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlayer   *self;
	gchar *cover_loading;
	gchar *cover_default;

	self = g_new0(EinaPlayer, 1);
	if (!eina_base_init((EinaBase *) self, hub, "player", EINA_BASE_GTK_UI))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	// Load settings
	if (!gel_hub_load(HUB(self), "settings")) {
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
	self->volume = g_object_new(EINA_TYPE_VOLUME, "lomo-player", LOMO(self), NULL);
	gtk_container_foreach(GTK_CONTAINER(W(self, "volume-button-container")),
		(GtkCallback) gtk_widget_hide,
		NULL);
	gtk_box_pack_start(GTK_BOX(W(self, "volume-button-container")),
		 GTK_WIDGET(self->volume),
		 TRUE, TRUE, 0);
	gtk_widget_show_all(GTK_WIDGET(self->volume));

	// Insert cover 
	self->cover = eina_cover_new();
	gtk_container_foreach(GTK_CONTAINER(W(self, "cover-image-container")),
		(GtkCallback) gtk_widget_hide,
		NULL);
	gtk_box_pack_start(GTK_BOX(W(self, "cover-image-container")),
		 GTK_WIDGET(self->cover),
		 FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(self->cover));

	// Get references to the main widgets
	self->play_pause       = W(self, "play-pause-button");
	self->play_pause_image = W(self, "play-pause-image");
	self->prev             = W(self, "prev-button");
	self->next             = W(self, "next-button");
	self->open             = W(self, "open-button");

	// All widgets are complete at this point, setup them if needed

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
		eina_player_switch_state_play(self);
	else
		eina_player_switch_state_pause(self);

	// Update info from LomoStream
	eina_player_set_info(self, (LomoStream *) lomo_player_get_stream(LOMO(self)));

	// Make player-ebox a dropable widget
	/*
	if (!gtk_ext_make_widget_dropable(W(self, "player-ebox"), NULL))
		e_warn("Cannot make infobox widget dropable");
	*/

	// Setup signals
	g_signal_connect(LOMO(self), "play",
		G_CALLBACK(on_lomo_play), self);
	g_signal_connect(LOMO(self), "pause",
		G_CALLBACK(on_lomo_pause), self);
	g_signal_connect(LOMO(self), "stop",
		G_CALLBACK(on_lomo_stop), self);
	g_signal_connect(LOMO(self), "change",
		G_CALLBACK(on_lomo_change), self);
	g_signal_connect(LOMO(self), "clear",
		G_CALLBACK(on_lomo_clear), self);
	g_signal_connect(LOMO(self), "all-tags",
		G_CALLBACK(on_lomo_all_tags), self);
	g_signal_connect(self->conf, "change",
		G_CALLBACK(on_settings_change), self);
	gel_ui_signal_connect_from_def_multiple(UI(self), _player_signals, self, NULL);

	// Set LomoPlayer volume
	lomo_player_set_volume(LOMO(self), eina_conf_get_int(self->conf, "/core/volume", 50));

	gtk_widget_show(W(self, "main-window"));
	return TRUE;
}

G_MODULE_EXPORT gboolean eina_player_exit
(gpointer data)
{
	EinaPlayer *self = (EinaPlayer *) data;

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

// Change UI to reflect a play state
static void
eina_player_switch_state_play(EinaPlayer *self)
{
	gtk_image_set_from_stock(GTK_IMAGE(self->play_pause_image),
		"gtk-media-pause", GTK_ICON_SIZE_BUTTON);
}

// Change UI to reflect a pause/stop state
static void
eina_player_switch_state_pause(EinaPlayer *self)
{
	gtk_image_set_from_stock(GTK_IMAGE(self->play_pause_image),
		"gtk-media-play", GTK_ICON_SIZE_BUTTON);
}

static void
eina_player_set_info(EinaPlayer *self, LomoStream *stream)
{
	gchar *tmp;
	gchar *info_str = NULL;
	gchar *tag;
	gchar *markup;

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

	// Create a working copy of title
	tag = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE));
	if (tag == NULL)
	{
		tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		tag = g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
	}
	gtk_window_set_title(GTK_WINDOW(W(self, "main-window")), tag);

	tmp = g_markup_escape_text(tag, -1);
	g_free(tag);
	info_str = g_strdup_printf("<span size=\"x-large\" weight=\"bold\">%s</span>", tmp);
	g_free(tmp);

	// Idem for artist, lomo_stream_get_tag gets a reference, remember this.
	tag = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	if (tag == NULL)
	{
		tmp = info_str;
		info_str = g_strconcat(info_str, "\n<span size=\"x-large\" weight=\"normal\">\uFEFF</span>", NULL);
		g_free(tmp);
	}
	else
	{
		markup = g_markup_escape_text(tag, -1);
		tmp = info_str;
		info_str = g_strconcat(info_str, "\n<span size=\"x-large\" weight=\"normal\">", markup, "</span>", NULL);
		g_free(tmp);
		g_free(markup);
	}

	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info-label")), info_str);
	g_free(info_str);
}

static EinaFsFilterAction
fs_filter(GFileInfo *info)
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
// Callbacks
// --
static void
on_lomo_play (LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state_play(self);
}

static void
on_lomo_pause(LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state_pause(self);
}

static void
on_lomo_stop (LomoPlayer *lomo, EinaPlayer *self)
{
	eina_player_switch_state_pause(self);
}

static void
on_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self)
{
	eina_player_set_info(self, (LomoStream *) lomo_player_get_nth(LOMO(self), to));
}

static void
on_lomo_clear(LomoPlayer *lomo, EinaPlayer *self) 
{
	eina_player_set_info(self, NULL);
}

static void
on_lomo_all_tags (LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self) 
{
	if (stream == lomo_player_get_stream(lomo))
		eina_player_set_info(self, (LomoStream *) stream);
}

static gboolean
on_main_window_delete_event(GtkWidget *w, GdkEvent *ev, EinaPlayer *self)
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
on_any_button_clicked(GtkWidget *w, EinaPlayer *self)
{
	LomoState state;
	GError *err = NULL;

	/* 
	 * Keep 'next' on the top of this if-else-if list
	 * since is the most common case
	 */
	if (w == self->next) {
		state = lomo_player_get_state(LOMO(self));

		lomo_player_go_next(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY) {
			lomo_player_play(LOMO(self), NULL);
		}
	}
	
	else if (w == self->prev) {
		state = lomo_player_get_state(LOMO(self));

		lomo_player_go_prev(LOMO(self), NULL);
		if (state == LOMO_STATE_PLAY) {
			lomo_player_play(LOMO(self), NULL);
		}
	}
	
	else if (w == self->play_pause) {
		if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY)
			lomo_player_pause(LOMO(self), &err);
		else
			lomo_player_play(LOMO(self), &err);
	}

	else if (w == self->open) {
		EinaFileChooserDialog *picker;
		GSList* uris;
		gint    run = TRUE;

		picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
		gtk_widget_show_all(GTK_WIDGET(picker));
		do {
			switch (gtk_dialog_run(GTK_DIALOG(picker)))
			{
				case EINA_FILE_CHOOSER_RESPONSE_PLAY:
					uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
					if (uris) {
						lomo_player_clear(LOMO(self));
						eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter, NULL, NULL);
						lomo_player_play(LOMO(self), NULL);
						g_slist_free(uris);
					}
					run = FALSE;
					break;
						
				case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
					uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
					if (uris)
						eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, fs_filter, NULL, NULL);
					g_slist_free(uris);
					eina_file_chooser_dialog_set_msg(picker, EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, "test");
					break;

				default:
					run = FALSE;
			}
		} while(run);
		gtk_widget_destroy(GTK_WIDGET(picker));
	}

	if (err != NULL) {
		gel_error("Got error: %s\n", err->message);
		g_error_free(err);
	}
}

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
#if 0
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

	uri_filter = eina_fs_filter_filter(self->stream_filter, // Filter
		uri_scan,
		TRUE, FALSE, FALSE,
		FALSE, FALSE);

	lomo_player_clear(LOMO(self));
	lomo_player_add_uri_multi(LOMO(self), (GList *) uri_filter);

	g_slist_free(uri_filter);
	g_ext_slist_free(uri_scan, g_free);
#endif
}

static void
on_dock_expander_activate(GtkWidget *wid, EinaPlayer *self)
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

static void
on_settings_change(EinaConf *conf, gchar *key, EinaPlayer *self)
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

// --
// Connector 
// --
G_MODULE_EXPORT GelHubSlave player_connector = {
	"player",
	&eina_player_init,
	&eina_player_exit,
};

