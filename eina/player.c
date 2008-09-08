#define GEL_DOMAIN "Eina::Player"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gmodule.h>
#include <gel/gel-ui.h>
#include "base.h"
#include "player.h"
#include "eina-player-seek.h"
#include "eina-player-volume.h"
#include "eina-file-chooser-dialog.h"
#include "playlist.h"
#include "settings.h"
#include "fs.h"

struct _EinaPlayer {
	EinaBase          parent;
	EinaPlayerSeek   *seek;
	EinaPlayerVolume *volume;

	EinaConf         *conf;

	GtkWidget *prev, *play, *pause, *next, *open;
};

EinaFsFilterAction eina_player_fs_filter(GFileInfo *info);

/* * * * * * * * * */
/* Lomo Callbacks  */
/* * * * * * * * * */
void on_lomo_play
(LomoPlayer *lomo, EinaPlayer *self);

void on_lomo_pause
(LomoPlayer *lomo, EinaPlayer *self);

void on_lomo_stop
(LomoPlayer *lomo, EinaPlayer *self);

void on_lomo_clear
(LomoPlayer *lomo, EinaPlayer *self);

void on_lomo_change
(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self);

void on_lomo_all_tags
(LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self);

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
gboolean on_main_window_delete_event
(GtkWidget *w, GdkEvent *ev, EinaPlayer *self);

void on_player_any_button_clicked
(GtkWidget *w, EinaPlayer *self);

void on_dock_expander_activate
(GtkWidget *wid, EinaPlayer *self);

void on_player_ebox_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlayer       *self);

/* * * * * * * * * */
/* Misc callbacks  */
/* * * * * * * * * */
void on_player_settings_change
(EinaConf *conf, gchar *key, EinaPlayer *self);

/* Signal definitions */
GelUISignalDef _player_signals[] = {
	/* Main buttons signals */
 	{ "play", "clicked",
	G_CALLBACK(on_player_any_button_clicked) },

	{ "pause", "clicked",
	G_CALLBACK(on_player_any_button_clicked) },

	{ "prev", "clicked",
	G_CALLBACK(on_player_any_button_clicked) },

	{ "next", "clicked",
	G_CALLBACK(on_player_any_button_clicked) },

	{ "open-files", "clicked",
	G_CALLBACK(on_player_any_button_clicked) },

	/* Misc */
	{ "player-ebox", "drag-data-received",
	G_CALLBACK(on_player_ebox_drag_data_received) },

	{ "dock-expander", "activate",
	G_CALLBACK(on_dock_expander_activate) },

	{ "main-window", "delete-event",
	G_CALLBACK(on_main_window_delete_event) },

	GEL_UI_SIGNAL_DEF_NONE
};

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean eina_player_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlayer   *self;

	/* Load ourselves */
	self = g_new0(EinaPlayer, 1);
	if (!eina_base_init((EinaBase *) self, hub, "player", EINA_BASE_GTK_UI)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	/* Insert seek */
	self->seek = (EinaPlayerSeek *) eina_player_seek_new();
	eina_player_seek_set_lomo(self->seek, LOMO(self));
	eina_player_seek_set_label(self->seek, GTK_LABEL(W(self, "time-info")));
	gtk_box_pack_start(GTK_BOX(W(self, "seek-volume-box")), GTK_WIDGET(self->seek),
		TRUE, TRUE, 0);

	/* Insert volume */
	self->volume = eina_player_volume_new(LOMO(self));
	gtk_box_pack_start(GTK_BOX(W(self, "seek-volume-box")), eina_player_volume_get_widget(self->volume),
		FALSE, TRUE, 0);

	gtk_widget_show_all(W(self, "seek-volume-box"));

	/* Insert cover */
	if (!gel_hub_load(HUB(self), "cover")) {
		gel_warn("Cannot load cover");
		gtk_widget_hide(W(self, "cover-button"));
	}

	/* Load settings */
	if (!gel_hub_load(HUB(self), "settings")) {
		gel_warn("Cannot load settings");
		return FALSE;
	}
	self->conf = gel_hub_shared_get(HUB(self), "settings");


	/*
	 * Make player-ebox a dropable widget
	 */
/*
	if (!gtk_ext_make_widget_dropable(W(self, "player-ebox"), NULL))
		e_warn("Cannot make infobox widget dropable");
*/
	/*
	 * Get references to the main widgets
	 */
	self->play      = W(self, "play");
	self->pause     = W(self, "pause");
	self->prev      = W(self, "prev");
	self->next      = W(self, "next");
	self->open      = W(self, "open-files");

	/* Setup lomo signals */
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

	/* Setup some widgets */
	eina_player_set_info(self, (LomoStream *) lomo_player_get_stream(LOMO(self)));

	if (lomo_player_get_state(LOMO(self)) == LOMO_STATE_PLAY) {
		eina_player_switch_state_play(self);
	} else {
		eina_player_switch_state_pause(self);
	}

	/* Setup filter subsystem */
	/*
	self->stream_filter = eina_fs_filter_new();
	eina_fs_filter_add_suffix(self->stream_filter, ".mp3");
	eina_fs_filter_add_suffix(self->stream_filter, ".ogg");
	eina_fs_filter_add_suffix(self->stream_filter, ".wav");
	eina_fs_filter_add_suffix(self->stream_filter, ".flac");
	*/
	
	lomo_player_set_volume(LOMO(self),
		eina_conf_get_int(self->conf, "/core/volume", 50));

	/* Playlist, hide widget on failure, setup on success */	
	goto player_show;

player_show:
	gtk_widget_show_all(W(self, "open-files"));
	gel_ui_signal_connect_from_def_multiple(UI(self), _player_signals, self, NULL);
	g_signal_connect(self->conf, "change", G_CALLBACK(on_player_settings_change), self);
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
	gel_hub_unload(HUB(self), "cover");
	gel_hub_unload(HUB(self), "settings");

	// Free base
	eina_base_fini((EinaBase *) self);

	return TRUE;
}

/*
 * Functions
 */

/* Change UI to reflect a play state */
void eina_player_switch_state_play(EinaPlayer *self) {
	gtk_widget_hide(W(self, "play"));
	gtk_widget_show(W(self, "pause"));
}

/* Change UI to reflect a pause/stop state */
void eina_player_switch_state_pause(EinaPlayer *self) {
	gtk_widget_hide(W(self, "pause"));
	gtk_widget_show(W(self, "play"));
}

void eina_player_set_info(EinaPlayer *self, LomoStream *stream) {
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
			GTK_LABEL(W(self, "stream-info")),
			_("<b>Eina music player</b>\n\uFEFF"));
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
	info_str = g_strdup_printf("<b>%s</b>", tmp);
	g_free(tmp);

	// Idem for artist, lomo_stream_get_tag gets a reference, remember this.
	tag = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	if (tag == NULL)
	{
		tmp = info_str;
		info_str = g_strconcat(info_str, "\n\uFEFF", NULL);
		g_free(tmp);
	}
	else
	{
		markup = g_markup_escape_text(tag, -1);
		tmp = info_str;
		info_str = g_strconcat(info_str, "\n ", _("by"), " ", "<i>", markup, "</i>", NULL);
		g_free(tmp);
		g_free(markup);
	}

	// Idem for album, lomo_stream_get_tag gets a reference, remember this.
	tag = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	if (tag != NULL)
	{
		tmp = info_str;
		markup = g_markup_escape_text(tag, -1);
		info_str = g_strconcat(info_str, " ", _("in"), " ", "<i>", markup, "</i>", NULL);
		g_free(tmp);
		g_free(markup);
	}

	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info")), info_str);
	g_free(info_str);
#if 0
	// Got streama
	else 
	{
		title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
		album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
		artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);

		// Fix things in case of failure
		if (title == NULL)
		{
			tmp   = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
			tmp2  = g_uri_unescape_string(tmp);
			g_free(tmp);

			title = g_markup_escape_text(tmp2);
			g_free(tmp2);
		}
		tmp = g_


		if (artist == NULL)
		{
			artist = "\uFEFF";
		}

		if (album == NULL)
		{
			album = "\uFEFF";
		}

	}


	gtk_window_set_title(GTK_WINDOW(W(self, "main-window")), title_simple);
	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info")), info_str);

	/* Set title */
	if (stream == NULL ) {
		str = g_strdup(_("<b>Eina Player</b>"));
		// eina_player_seek_disable_widget(self->seek);
		gtk_widget_set_sensitive(GTK_WIDGET(self->seek), FALSE);
		window_title = g_strdup("Eina Player");
		goto set_label;
	}

	else {
		// Get title, album, artist
		title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
		album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
		artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
		if (title)  title  = g_markup_escape_text(title, -1);
		if (album)  album  = g_markup_escape_text(album, -1);
		if (artist) artist = g_markup_escape_text(artist, -1);
		if (GTK_IS_WIDGET(self->seek))
			gtk_widget_set_sensitive(GTK_WIDGET(self->seek), TRUE);
		else
			gel_error("seek widget is not a widget");
	}

	if (title == NULL) {
		str = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		window_title = g_strconcat(str, " - ", "Eina Player", NULL);
	} else {
		str = g_strconcat("<b>", title, "</b>", NULL);
		window_title = g_strconcat(title, " - ", "Eina Player", NULL);

		if ((artist != NULL) || (album != NULL)) {
			tmp = str;
			str = g_strconcat(str, "\n", NULL);
			g_free(tmp);
		}

		if ( artist ) {
			tmp = str;
			str = g_strconcat(str, " ", _("by"), " ", "<i>", artist, "</i>", NULL);
			g_free(tmp);
		} else {
			str = "\uFEFF";
		}

		if ( album ) {
			tmp = str;
			str = g_strconcat(str, " ", _("in"), " ", "<i>", album, "</i>", NULL);
			g_free(tmp);
		}
	}
	g_free(title);
	g_free(album);
	g_free(artist);

set_label:
	gtk_label_set_markup(GTK_LABEL(W(self, "stream-info")), str);
	gtk_window_set_title(GTK_WINDOW(W(self, "main-window")), window_title);
	g_free(window_title);
	g_free(str);
#endif
}


EinaFsFilterAction eina_player_fs_filter(GFileInfo *info)
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

/*
 * Callbacks
 */
void on_lomo_play (LomoPlayer *lomo, EinaPlayer *self) {
	eina_player_switch_state_play(self);
}

void on_lomo_pause(LomoPlayer *lomo, EinaPlayer *self) {
	eina_player_switch_state_pause(self);
}

void on_lomo_stop (LomoPlayer *lomo, EinaPlayer *self) {
	eina_player_switch_state_pause(self);
}

void on_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaPlayer *self) {
	eina_player_set_info(self, (LomoStream *) lomo_player_get_nth(LOMO(self), to));
}

void on_lomo_clear(LomoPlayer *lomo, EinaPlayer *self) {
	eina_player_set_info(self, NULL);
}

void on_lomo_all_tags (LomoPlayer *lomo, const LomoStream *stream, EinaPlayer *self) {
	if (stream == lomo_player_get_stream(lomo)) {
		eina_player_set_info(self, (LomoStream *) stream);
	}
}

gboolean on_main_window_delete_event(GtkWidget *w, GdkEvent *ev, EinaPlayer *self) {
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

void on_player_any_button_clicked(GtkWidget *w, EinaPlayer *self) {
	LomoState state;
	GError *err = NULL;

	/* 
	 * Keep 'next' on the top of this if-else-if list
	 * since is the most common case
	 */
	if (w == self->next) {
		state = lomo_player_get_state(LOMO(self));

		lomo_player_go_next(LOMO(self));
		if (state == LOMO_STATE_PLAY) {
			lomo_player_play(LOMO(self), NULL);
		}
	}
	
	else if (w == self->prev) {
		state = lomo_player_get_state(LOMO(self));

		lomo_player_go_prev(LOMO(self));
		if (state == LOMO_STATE_PLAY) {
			lomo_player_play(LOMO(self), NULL);
		}
	}
	
	else if (w == self->play) {
		lomo_player_play(LOMO(self), &err);
	}

	else if ( w == self->pause ) {
		lomo_player_pause(LOMO(self), NULL);
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
						eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, eina_player_fs_filter, NULL, NULL);
						lomo_player_play(LOMO(self), NULL);
						g_slist_free(uris);
					}
					run = FALSE;
					break;
						
				case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
					uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
					if (uris)
						eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, eina_player_fs_filter, NULL, NULL);
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
		printf("Got error: %s\n", err->message);
		g_error_free(err);
	}
}

void on_player_ebox_drag_data_received(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlayer       *self) {
    
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

void on_dock_expander_activate(GtkWidget *wid, EinaPlayer *self) {
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

void on_player_settings_change(EinaConf *conf, gchar *key, EinaPlayer *self) {
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

/* * * * * * */
/* Connector */
/* * * * * * */
G_MODULE_EXPORT GelHubSlave player_connector = {
	"player",
	&eina_player_init,
	&eina_player_exit,
};

