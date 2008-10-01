#define GEL_DOMAIN "Eina::Playlist"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <lomo/util.h>
#include <gel/gel-ui.h>
#include "config.h"
#include "base.h"
#include "playlist.h"
#include "settings.h"
#include "fs.h"
#include "eina-file-chooser-dialog.h"
#include <eina/iface.h>

struct _EinaPlaylist
{
	EinaBase parent;

	EinaConf     *conf;

	GtkWidget    *dock;
	GtkTreeView  *tv;
	GtkListStore *model;
	const gchar  *title_fmtstr;
};

typedef enum
{
	PLAYLIST_COLUMN_STATE,
	PLAYLIST_COLUMN_URI,
	PLAYLIST_COLUMN_DURATION,
	PLAYLIST_COLUMN_RAW_TITLE,
	PLAYLIST_COLUMN_TITLE,

	PLAYLIST_N_COLUMNS
} EinaPlaylistColumn;

/*
 * Declarations
 */
// API
gboolean playlist_init
(GelHub *hub, gint *argc, gchar ***argv);
gboolean playlist_exit
(gpointer data);
GtkWidget *
eina_playlist_dock_init(EinaPlaylist *self, GtkTreeView **treeview, GtkListStore **model);

void
eina_playlist_update_item2(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...);
#define eina_playlist_update_current_item(self,...) \
	eina_playlist_update_item2(self, NULL,lomo_player_get_current(LOMO(self)),__VA_ARGS__)

gboolean
__eina_playlist_search_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, EinaPlaylist *self);

EinaFsFilterAction
eina_playlist_fs_filter(GFileInfo *info);

// UI Callbacks
void
on_pl_row_activated(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self);
void on_pl_add_button_clicked
(GtkWidget *w, EinaPlaylist *self);
gint // XXX
__eina_playlist_sort_int_desc(gconstpointer a, gconstpointer b);
void
on_pl_remove_button_clicked(GtkWidget *w, EinaPlaylist *self);
void // for swapped
on_pl_clear_button_clicked (GtkWidget *w, EinaPlaylist *self);
void
on_pl_random_button_toggled(GtkWidget *w, EinaPlaylist *self);
void
on_pl_repeat_button_toggled(GtkWidget *w, EinaPlaylist *self);

// Lomo callbacks
void on_pl_lomo_add
(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self);
void on_pl_lomo_del
(LomoPlayer *lomo, gint pos, EinaPlaylist *self);
void on_pl_lomo_change
(LomoPlayer *lomo, gint old, gint new, EinaPlaylist *self);
void on_pl_lomo_state_change
(LomoPlayer *lomo, EinaPlaylist *self);
void on_pl_lomo_random
(LomoPlayer *lomo, gboolean val, EinaPlaylist *self);
void on_pl_lomo_clear
(LomoPlayer *lomo, EinaPlaylist *self);
void on_pl_lomo_eos
(LomoPlayer *lomo, EinaPlaylist *self);
void on_pl_lomo_all_tags
(LomoPlayer *lomo, LomoStream *stream, EinaPlaylist *self);

void on_pl_row_activated
(GtkWidget *w,
 	GtkTreePath *path,
	GtkTreeViewColumn *column,
	EinaPlaylist *self);
void on_pl_add_button_clicked
(GtkWidget *w, EinaPlaylist *self);
void on_pl_clear_button_clicked
(GtkWidget *w, EinaPlaylist *self);
void on_pl_remove_button_clicked
(GtkWidget *w, EinaPlaylist *self);
void on_pl_random_button_toggled
(GtkWidget *w, EinaPlaylist *self);
void on_pl_repeat_button_toggled
(GtkWidget *w, EinaPlaylist *self);
void on_pl_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self);
void on_pl_drag_data_get
(GtkWidget *w,
	GdkDragContext   *drag_context,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self);

void on_pl_settings_change
(EinaConf *conf, const gchar *key, gpointer data);

/* Signal definitions */
GelUISignalDef _playlist_signals[] = {
	{ "playlist-treeview",   "row-activated",
		G_CALLBACK(on_pl_row_activated) },

	{ "playlist-repeat-button", "toggled",
	G_CALLBACK(on_pl_repeat_button_toggled) },
	{ "playlist-random-button", "toggled",
	G_CALLBACK(on_pl_random_button_toggled) },
	
	{ "playlist-add-button",  "clicked",
	G_CALLBACK(on_pl_add_button_clicked) },
	{ "playlist-clear-button",  "clicked",
	G_CALLBACK(on_pl_clear_button_clicked) },
	{ "playlist-remove-button", "clicked",
	G_CALLBACK(on_pl_remove_button_clicked) },
	
	{ "playlist-treeview",   "drag-data-received",
	G_CALLBACK(on_pl_drag_data_received) },
	{ "playlist-treeview",   "drag-data-get",
	G_CALLBACK(on_pl_drag_data_get) },
	
	GEL_UI_SIGNAL_DEF_NONE
};

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean playlist_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlaylist *self;
	gint i;
	gchar *tmp;
	gchar *buff;
	gchar **uris;
	EinaIFace *iface;

	static const gchar *rr_files[] = {
		"random.png",
		"repeat.png",
		NULL
	};
	static const gchar *rr_widgets[] = {
		"playlist-random-image",
		"playlist-repeat-image",
		NULL
	};
	GdkPixbuf *pb = NULL;
	GError *err = NULL;
	gboolean random, repeat;

	self = g_new0(EinaPlaylist, 1);
	if (!eina_base_init((EinaBase *) self, hub, "playlist", EINA_BASE_GTK_UI)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	/* Load and/or ref settings module */
	if (gel_hub_load(HUB(self), "settings")) {
		self->conf = gel_hub_shared_get(HUB(self), "settings");
		g_signal_connect(self->conf, "change",
			G_CALLBACK(on_pl_settings_change), self);
	} else {
		gel_error("Cannot load component settings");
		eina_base_fini((EinaBase *) self);
		return FALSE;
	}

	// Setup internal values from settings
	repeat = eina_conf_get_bool(self->conf, "/core/repeat", FALSE);
	random = eina_conf_get_bool(self->conf, "/core/random", FALSE);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W(self, "playlist-repeat-button")), repeat);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W(self, "playlist-random-button")), random);
	lomo_player_set_repeat(LOMO(self), repeat);
	lomo_player_set_random(LOMO(self), random);

	self->title_fmtstr = eina_conf_get_str(self->conf, "/ui/playlist/fmt", "{%a - }%t");

	/* Setup stock icons */
	for (i = 0; rr_files[i] != NULL; i++) {
		if ((tmp = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, (gchar *) rr_files[i])) == NULL) {
			gel_warn("Cannot find %s pixmap", rr_files[i]);
			continue;
		}

		if ((pb = gdk_pixbuf_new_from_file_at_scale(tmp, 24, 24, TRUE, &err)) == NULL) {
			gel_warn("Cannot load %s into pixbuf: %s", tmp, err->message);
			g_error_free(err);
			g_free(tmp);
			continue;
		}
		gtk_image_set_from_pixbuf(GTK_IMAGE(W(self, rr_widgets[i])), pb);
		g_object_unref(pb);
		g_free(tmp);
	}

	self->dock = eina_playlist_dock_init(self, &(self->tv), &(self->model));

	/* Connect some UI signals */
	gel_ui_signal_connect_from_def_multiple(
		UI(self),
		_playlist_signals,
		self,
		NULL);

	/* Connect lomo signals */
	g_signal_connect(LOMO(self), "add",      G_CALLBACK(on_pl_lomo_add),      self);
	g_signal_connect(LOMO(self), "del",      G_CALLBACK(on_pl_lomo_del),      self);
	g_signal_connect(LOMO(self), "change",   G_CALLBACK(on_pl_lomo_change),   self);
	g_signal_connect(LOMO(self), "clear",    G_CALLBACK(on_pl_lomo_clear),    self);
	g_signal_connect(LOMO(self), "play",     G_CALLBACK(on_pl_lomo_state_change), self);
	g_signal_connect(LOMO(self), "stop",     G_CALLBACK(on_pl_lomo_state_change), self);
	g_signal_connect(LOMO(self), "pause",    G_CALLBACK(on_pl_lomo_state_change), self);
	g_signal_connect(LOMO(self), "random",   G_CALLBACK(on_pl_lomo_random),   self);
	g_signal_connect(LOMO(self), "eos",      G_CALLBACK(on_pl_lomo_eos),      self);
	g_signal_connect(LOMO(self), "all-tags", G_CALLBACK(on_pl_lomo_all_tags), self);

	/* Load playlist and go to last session's current */
	tmp = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "playlist", NULL);
	if (g_file_get_contents(tmp, &buff, NULL, NULL))
	{
		uris = g_uri_list_extract_uris((const gchar *) buff);
		lomo_player_add_uri_strv(LOMO(self), uris);
		g_strfreev(uris);
	}
	g_free(tmp);
	g_free(buff);
	lomo_player_go_nth(
		LOMO(self), 
		eina_conf_get_int(self->conf, "/playlist/last_current", 0),
		NULL);

	if ((iface = gel_hub_shared_get(hub, "iface")) == NULL) {
		gel_error("Cannot get EinaIFace");
		return FALSE;
	}

	gtk_widget_show(self->dock);
	return eina_iface_dock_add_item(iface, "playlist",
		gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU), self->dock);
}

G_MODULE_EXPORT gboolean playlist_exit
(gpointer data)
{
	EinaPlaylist *self = (EinaPlaylist *) data;
	const GList *pl;
	GList *l;
	gchar *file;
	gchar **out;
	gchar *buff;
	gint i = 0;
	gint fd;

	pl = lomo_player_get_playlist(LOMO(self));
	l  = (GList *) pl;

	out = g_new0(gchar*, g_list_length(l) + 1);
	while (l) {
		out[i++] = lomo_stream_get_tag(
			LOMO_STREAM(l->data), LOMO_TAG_URI);
		l = l->next;
	}
	buff = g_strjoinv("\n", out);
	g_free(out);

	file = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "playlist", NULL);
	fd = g_open(file, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR);
	write(fd, buff, strlen(buff));
	close(fd);
	g_free(file);

	i = lomo_player_get_current(LOMO(self));
	eina_conf_set_int(self->conf, "/playlist/last_current", i);

	gel_hub_unload(HUB(self), "settings");

	eina_iface_dock_remove_item(gel_hub_shared_get(HUB(self), "iface"), "playlist");
	eina_base_fini((EinaBase *) self);
	return TRUE;
}

GtkWidget *
eina_playlist_dock_init(EinaPlaylist *self, GtkTreeView **treeview, GtkListStore **model)
{
	GtkWidget    *ret;
	GtkTreeView  *_tv;
	GtkListStore *_model;
	GtkTreeViewColumn *state_col, *title_col, *duration_col;
	GtkCellRenderer *title_renderer;
	GtkTreeSelection  *selection;

	_tv = GTK_TREE_VIEW(W(self, "playlist-treeview"));

	/* Setup treeview step 1: build renderers and attach to colums and columns to treeview*/
	state_col = gtk_tree_view_column_new_with_attributes(NULL,
		gtk_cell_renderer_pixbuf_new(), "stock-id", PLAYLIST_COLUMN_STATE, NULL);

	title_renderer = gtk_cell_renderer_text_new();
	title_col = gtk_tree_view_column_new_with_attributes(_("Title"),
		title_renderer,  "markup", PLAYLIST_COLUMN_TITLE, NULL);

	duration_col  = gtk_tree_view_column_new_with_attributes("Duration",
		gtk_cell_renderer_text_new(), "markup", PLAYLIST_COLUMN_DURATION, NULL);

	gtk_tree_view_append_column(_tv, state_col);
	gtk_tree_view_append_column(_tv, title_col);
	gtk_tree_view_append_column(_tv, duration_col);

	/* Setup treeview step 2: set model for treeview */
	_model = gtk_list_store_new(PLAYLIST_N_COLUMNS,
		G_TYPE_STRING, // State
		G_TYPE_STRING, // URI
		G_TYPE_STRING, // Duration
		G_TYPE_STRING, // Raw title
		G_TYPE_STRING  // Title
		);
	gtk_tree_view_set_model(_tv, GTK_TREE_MODEL(_model));

	/* Setup treeview step 3: set some properties */
	g_object_set(G_OBJECT(title_renderer),
		"ellipsize-set", TRUE,
		"ellipsize", PANGO_ELLIPSIZE_END,
		NULL);
	g_object_set(G_OBJECT(state_col),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(title_col),
		"visible",   TRUE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(duration_col),
		"visible",   FALSE,
		"resizable", FALSE,
		NULL);
	g_object_set(G_OBJECT(_tv),
		// "search-column", -1,
		"headers-clickable", FALSE,
		"headers-visible", TRUE,
		NULL);
	gtk_tree_view_set_search_column(_tv, PLAYLIST_COLUMN_TITLE);
	gtk_tree_view_set_search_equal_func(_tv, (GtkTreeViewSearchEqualFunc) __eina_playlist_search_func, self, NULL);

	selection = gtk_tree_view_get_selection(_tv);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	ret = W(self, "playlist-main-box");
	g_object_ref(ret);
	if (ret->parent != NULL)
		gtk_widget_unparent(ret);

	if (treeview != NULL)
		*treeview = _tv;
	if (model != NULL)
		*model = _model;

	return ret;
}

void
__eina_playlist_update_item_state(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_state, gint current_item)
{
	LomoState state = LOMO_STATE_INVALID;
	gchar *new_state = NULL;

	// If item is not the active one, new_state is NULL
	if (item == current_item)
	{
		state = lomo_player_get_state(LOMO(self));
		switch (state)
		{
			case LOMO_STATE_INVALID:
				// leave stock-id as NULL
				break;
			case LOMO_STATE_PLAY:
				new_state = GTK_STOCK_MEDIA_PLAY;
				break;
			case LOMO_STATE_PAUSE:
				new_state = GTK_STOCK_MEDIA_PAUSE;
				break;
			case LOMO_STATE_STOP:
				new_state = GTK_STOCK_MEDIA_STOP;
				break;
			default:
				eina_iface_warn("Unknow state: %d", state);
				break;
		}
	}

	// Update only if needed
	if ((current_state != NULL) && (new_state != NULL))
		if (g_str_equal(new_state, current_state))
			return;

	eina_iface_debug("Updating item %d(cur %d): %s => %s", item, current_item, current_state, new_state);
	gtk_list_store_set(GTK_LIST_STORE(self->model), iter,
		PLAYLIST_COLUMN_STATE, new_state,
		-1);
}

void
__eina_playlist_update_item_title(EinaPlaylist *self, GtkTreeIter *iter, gint item, gchar *current_title, gint current_item, gchar *current_raw_title)
{
	gchar *markup = NULL;

	// Calculate new value
	if (item == current_item)
		markup = g_strdup_printf("<b>%s</b>", current_raw_title);
	else
		markup = g_strdup(current_raw_title);
	
	// Update only if needed
	if (!g_str_equal(current_title, markup))
	{
		eina_iface_debug("Updating item %d(cur %d): %s => %s", item, current_item, current_title, markup);
		gtk_list_store_set(GTK_LIST_STORE(self->model), iter,
			PLAYLIST_COLUMN_TITLE, markup,
			-1);
	}
	g_free(markup);
}

void
eina_playlist_update_item2(EinaPlaylist *self, GtkTreeIter *iter, gint item, ...)
{
	GtkTreePath *treepath;
	GtkTreeIter  _iter;
	gint*        treepath_indices; // Don't free, it's owned by treepath
	gint   current = -1;

	gchar *state     = NULL;
	gchar *title     = NULL;
	gchar *raw_title = NULL;

	va_list columns;
	gint column;

	if ((item == -1) && (iter == NULL))
	{
		eina_iface_error("You must supply an iter _or_ an item");
		return;
	}

	// Get item from iter
	if (item == -1)
	{
		treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(self->model), iter);
		if (treepath == NULL)
		{
			eina_iface_warn("Cannot get a GtkTreePath from index %d", item);
			return;
		}
		treepath_indices = gtk_tree_path_get_indices(treepath);
		item = treepath_indices[0];
		gtk_tree_path_free(treepath);
	}

	// Get iter from item
	if (iter == NULL)
	{
		treepath = gtk_tree_path_new_from_indices(item, -1);
		gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &_iter, treepath);
		gtk_tree_path_free(treepath);
		iter = &_iter;
	}

	current = lomo_player_get_current(LOMO(self));

	// Get data from GtkListStore supported columns
	gtk_tree_model_get(GTK_TREE_MODEL(self->model), iter,
		PLAYLIST_COLUMN_STATE, &state,
		PLAYLIST_COLUMN_TITLE, &title,
		PLAYLIST_COLUMN_RAW_TITLE, &raw_title,
		-1);

	// Start updating columns
	va_start(columns, item);
	do {
		column = va_arg(columns,gint);
		if (column == -1)
			break;

		eina_iface_debug("Updating column %d", column);
		switch (column)
		{
			case PLAYLIST_COLUMN_STATE:
				__eina_playlist_update_item_state(self, iter, item, state, current);
				break;

			case PLAYLIST_COLUMN_TITLE:
				__eina_playlist_update_item_title(self, iter, item, title, current, raw_title);
				break;

			default:
				eina_iface_warn("Unknow column %d", column);
				break;
		}
	} while(column != -1);
	g_free(state);
	g_free(title);
	g_free(raw_title);
}

gboolean
__eina_playlist_search_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, EinaPlaylist *self)
{
	gchar *title = NULL, *title_lc, *key_lc;
	gboolean ret = FALSE;

	if (column != PLAYLIST_COLUMN_TITLE)
	{
		gel_warn("Invalid search column %d", column);
		return TRUE;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(self->model), iter,
		PLAYLIST_COLUMN_TITLE, &title,
		-1);
	title_lc = g_utf8_strdown(title, -1);
	key_lc = g_utf8_strdown(key, -1);
	g_free(title);

	if (strstr(title_lc, key_lc) != NULL)
		ret = FALSE;
	else
		ret = TRUE;
	g_free(title_lc);
	g_free(key_lc);

	return ret;
}

EinaFsFilterAction
eina_playlist_fs_filter(GFileInfo *info)
{
	static const gchar *suffixes[] = {".mp3", ".ogg", ".wma", ".aac", ".flac", NULL };
	gchar *lc_name;
	gint i;
	EinaFsFilterAction ret = EINA_FS_FILTER_REJECT;

	if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY) {
		return  EINA_FS_FILTER_ACCEPT;
	}

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
 * UI Callbacks
 */
void
on_pl_row_activated(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaPlaylist *self)
{
	gint *indexes;
	GError *err = NULL;

	indexes = gtk_tree_path_get_indices(path);
	lomo_player_go_nth(LOMO(self), indexes[0], NULL);
	if (lomo_player_get_state(LOMO(self)) != LOMO_STATE_PLAY )
	{
		lomo_player_play(LOMO(self), &err);
		if (err)
		{
			gel_error("Cannot set state to LOMO_STATE_PLAY: %s", err->message);
			g_error_free(err);
		}
	}
}

void on_pl_add_button_clicked
(GtkWidget *w, EinaPlaylist *self)
{
	EinaFileChooserDialog *picker;
	GSList *uris = NULL;
	gboolean run = TRUE;

	picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	gtk_widget_show_all(GTK_WIDGET(picker));
	while (run)
	{
		switch (gtk_dialog_run(GTK_DIALOG(picker)))
		{
			case EINA_FILE_CHOOSER_RESPONSE_PLAY:
				uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
				if (uris && uris->data)
				{
					lomo_player_clear(LOMO(self));
					eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, eina_playlist_fs_filter, NULL, NULL);
				}
				g_slist_free(uris);
				lomo_player_play(LOMO(self), NULL);
				run = FALSE; // Stop
				break; 

			case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
				uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(picker));
				if (uris && uris->data)
					eina_fs_lomo_feed_uri_multi(LOMO(self), (GList*) uris, eina_playlist_fs_filter, NULL, NULL);
				g_slist_free(uris);
				break;

			default:
				run = FALSE;
		}
	}
	gtk_widget_destroy(GTK_WIDGET(picker));
}

gint
__eina_playlist_sort_int_desc(gconstpointer a, gconstpointer b)
{
	gint int_a = GPOINTER_TO_INT(a);
	gint int_b = GPOINTER_TO_INT(b);
	if (int_a > int_b) return -1;
	if (int_a < int_b) return  1;
	return 0;
}

static gchar*
playlist_format_stream_cb(gchar key, LomoStream *stream)
{
	return  lomo_stream_get_tag_by_id(stream, key);
	/*
	gchar *tag = lomo_stream_get_tag_by_id(stream, key);
	gchar *ret = g_markup_escape_text(tag, -1);
	g_free(tag);
	return ret;
	*/
}

void
on_pl_remove_button_clicked(GtkWidget *w, EinaPlaylist *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model = NULL;
	gchar *path_str;
	GList *rows, *l, *sorted, *to_sort = NULL;

	// Get selected
	selection = gtk_tree_view_get_selection(self->tv);
	l = rows = gtk_tree_selection_get_selected_rows(selection, &model);

	// Create an integer list
	while (l)
	{
		path_str = gtk_tree_path_to_string((GtkTreePath*  )(l->data));
		gint index = (gint) (atol(path_str) / 1);

		to_sort = g_list_prepend(to_sort, GINT_TO_POINTER(index));

		g_free(path_str);
		l = l->next;
	}

	// Free results
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	// Sort list and delete
	l = sorted = g_list_sort(to_sort, __eina_playlist_sort_int_desc);
	while (l) {
		lomo_player_del(LOMO(self), GPOINTER_TO_INT(l->data));
		l = l->next;
	}
	g_list_free(sorted);
}

void
on_pl_clear_button_clicked (GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_clear(LOMO(self));
}

void
on_pl_random_button_toggled(GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_set_random(
		LOMO(self),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	eina_conf_set_bool(
		self->conf,
		"/core/random",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

void
on_pl_repeat_button_toggled(GtkWidget *w, EinaPlaylist *self)
{
	lomo_player_set_repeat(
		LOMO(self),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
	eina_conf_set_bool(
		self->conf,
		"/core/repeat",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

/*
 * Lomo Callbacks
 */
void
on_pl_lomo_add (LomoPlayer *lomo, LomoStream *stream, gint pos, EinaPlaylist *self)
{
	GtkTreeIter iter;
	gchar *title = NULL, *tmp;

	lomo_stream_format(stream,
		"%t", 0,
		LOMO_STREAM_URL_DECODE | LOMO_STREAM_BASENAME | LOMO_STREAM_UTF8,
		&title);

	tmp = title;
	title = g_markup_escape_text(title, -1);
	g_free(tmp);

	gtk_list_store_insert_with_values(self->model,
		&iter, pos,
		PLAYLIST_COLUMN_STATE,     NULL,
		PLAYLIST_COLUMN_URI,       lomo_stream_get_tag(stream, LOMO_TAG_URI),
		PLAYLIST_COLUMN_DURATION,  NULL,
		PLAYLIST_COLUMN_RAW_TITLE, title,
		PLAYLIST_COLUMN_TITLE,     title,
		-1);
	g_free(title);
}

void
on_pl_lomo_del(LomoPlayer *lomo, gint pos, EinaPlaylist *self)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *path_str;
	gchar        *raw_title;
	path_str = g_strdup_printf("%d", pos);

	model = gtk_tree_view_get_model(self->tv);
	if (!gtk_tree_model_get_iter_from_string(model, &iter, (const gchar *) path_str))
	{
		gel_error("Cannot find iter corresponding to index %d", pos);
		g_free(path_str);
		return;
	}

	gtk_tree_model_get(model, &iter,
		PLAYLIST_COLUMN_RAW_TITLE, &raw_title,
		-1);
	
	if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
	{
		gel_error("Cannot remove iter");
		return;
	}

	g_free(path_str);
}

void
on_pl_lomo_state_change(LomoPlayer *lomo, EinaPlaylist *self)
{
	eina_playlist_update_current_item(self, PLAYLIST_COLUMN_STATE, -1);
}

void
on_pl_lomo_random(LomoPlayer *lomo, gboolean val, EinaPlaylist *self)
{
	if (val)
		lomo_player_randomize(lomo);
}

void
on_pl_lomo_eos(LomoPlayer *lomo, EinaPlaylist *self)
{
	if (lomo_player_get_next(lomo) != -1) {
		lomo_player_go_next(lomo, NULL);
		lomo_player_play(lomo, NULL); //XXX: Handle GError
	}
}

void
on_pl_lomo_all_tags (
	LomoPlayer *lomo,
	LomoStream *stream,
	EinaPlaylist *self)
{

	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *uri;
	gchar        *title = NULL;
	gchar        *escape = NULL;
	gchar        *title_markup = NULL;
	guint64      *duration;
	gchar        *duration_str;

	model = gtk_tree_view_get_model(self->tv);
	if (!gtk_tree_model_get_iter_first(model, &iter)) {
		return;
	}

	/* Search the stream on the treeview */
	while(gtk_list_store_iter_is_valid(GTK_LIST_STORE(model), &iter)) {
		gtk_tree_model_get(model, &iter, PLAYLIST_COLUMN_URI, &uri, -1);
		if (g_str_equal(lomo_stream_get_tag(stream, LOMO_TAG_URI), uri)) {
	  		/* Title */
			title = gel_str_parser((gchar *) self->title_fmtstr, 
				(GelStrParserFunc) playlist_format_stream_cb, stream);

			/* Duration */
			if ((duration = lomo_stream_get_tag(stream, LOMO_TAG_DURATION)) == NULL)
				duration_str = NULL;
			else {
				duration_str = g_strdup_printf("%02d:%02d",
					(gint) lomo_nanosecs_to_secs(*duration) / 60,
					(gint) lomo_nanosecs_to_secs(*duration) % 60);
			}
			
			/* Escape the title  */
			escape = g_markup_escape_text(title, -1);
			g_free(title);

			if (stream == lomo_player_get_current_stream(LOMO(self))) {
				title_markup = g_strconcat("<b>", escape, "</b>", NULL);
				gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					PLAYLIST_COLUMN_RAW_TITLE, escape,
					PLAYLIST_COLUMN_TITLE, title_markup,
					PLAYLIST_COLUMN_DURATION, duration_str,
					-1);
				g_free(title_markup);
			} else {
				gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					PLAYLIST_COLUMN_RAW_TITLE, escape,
					PLAYLIST_COLUMN_TITLE, escape,
					PLAYLIST_COLUMN_DURATION, duration_str,
					-1);
			}
			g_free(escape);
		}

		if(!gtk_tree_model_iter_next(model, &iter))
			break;
	}
}

void
on_pl_lomo_change(LomoPlayer *lomo, gint old, gint new, EinaPlaylist *self) {
	GtkTreePath *treepath;

	/* If old and new are invalid return */
	if ((old < 0) && (new < 0)) {
		gel_warn("old AND new positions are invalid (%d -> %d)", old, new);
		return;
	}

	eina_iface_debug("Handling change signal %d -> %d", old, new);
	/* Restore normal state on old stream */
	if (old >= 0) {
		eina_iface_debug("updating all columns from OLD %d", old);
		eina_playlist_update_item2(self, NULL, old, PLAYLIST_COLUMN_STATE, PLAYLIST_COLUMN_TITLE, -1);
	}

	/* Create markup for the new stream */
	if (new >= 0) {
		eina_iface_debug("updating all columns from NEW %d", new);
		eina_playlist_update_item2(self, NULL, new, PLAYLIST_COLUMN_STATE, PLAYLIST_COLUMN_TITLE, -1);

		// Scroll to new stream
		treepath = gtk_tree_path_new_from_indices(new, -1);
		gtk_tree_view_scroll_to_cell(self->tv, treepath, NULL,
			TRUE, 0.5, 0.0);
		gtk_tree_path_free(treepath);
	}
}

void
on_pl_lomo_clear(LomoPlayer *lomo, EinaPlaylist *self)
{
	gtk_list_store_clear(self->model);
}

void on_pl_drag_data_received(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist       *self) {
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
	uri_list = g_slist_reverse(uri_list);
	g_strfreev(uris);

	/*
	 * Scan files over uri_list, then free uri_list
	 * since eina_fs_scan duplicates data
	 */
	uri_scan = eina_fs_scan(uri_list);
	g_ext_slist_free(uri_list, g_free);

	/*
	 * Apply filters on the uri_scan GSList.
	 * Warn: We dont use eina_fs_filter_filter's duplicate flag
	 * so, we dont replicate data
	 */
	uri_filter = eina_fs_filter_filter(self->filter_suffixes, // Filter
		uri_scan,
		TRUE, FALSE, FALSE,
		FALSE, FALSE);
	
	/* Add valid uris to LomoPlayer */
	lomo_player_add_uri_multi(LOMO(self), (GList *) uri_filter);

	/* Free our dirty data */
	g_ext_slist_free(uri_scan, g_free);
	g_slist_free(uri_filter);
#endif
}

void on_pl_drag_data_get(GtkWidget *w,
	GdkDragContext   *drag_context,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaPlaylist     *self) {
#if 0
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreePath      *treepath;
	GtkTreeIter       iter;
	GList            *rows, *l;
	gint              pos;
	gchar           **charv;
	gint              i;
	LomoStream       *stream;
	gchar *tmp = NULL;

	/* Process:
	 * 1. Get the list of rows selected
	 * 2. Get GtkTreeIter for each GtkTreePath
	 * 3. Extract ID
	 * 4. Query LomoPlayer to get full URI and save it
	 */
	selection = gtk_tree_view_get_selection(self->tv);
	model = (GtkTreeModel *) (self->model);
	l = rows = gtk_tree_selection_get_selected_rows(selection, &model);

	charv = (gchar **) g_new0(gchar*, g_list_length(l));
	
	i = 0;
	while (l) {
		treepath = (GtkTreePath *) l->data;
		if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(self->model), &iter, treepath)) {
			continue;
		}

		gtk_tree_model_get(GTK_TREE_MODEL(self->model), &iter, PLAYLIST_COLUMN_NUMBER, &pos, -1);
		stream = (LomoStream *) lomo_player_get_nth(LOMO(self), pos);
		charv[i] = lomo_stream_get_tag(stream, LOMO_TAG_URI); // Dont copy, its rigth.

		i++;
		l = l->next;
	}
	
	/* End Process */
	tmp = g_strjoinv("\r\n", charv);
	gtk_selection_data_set (data, data->target, 8, (guchar*) tmp, strlen(tmp));
	g_free(charv); // No g_strfreev, its rigth

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows); // g_list_free dont revised

	gtk_drag_finish(drag_context, TRUE, FALSE, time);
#endif
}

void on_pl_settings_change(EinaConf *conf, const gchar *key, gpointer data) {
	EinaPlaylist *self = (EinaPlaylist *) data;
	if (g_str_equal(key, "/core/repeat")) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(W(self, "playlist-repeat-button")),
			eina_conf_get_bool(conf, "/core/repeat", TRUE));
	}

	else if (g_str_equal(key, "/core/random")) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(W(self, "playlist-random-button")),
			eina_conf_get_bool(conf, "/core/random", TRUE));
	}
}


/*
 * Connector
 */
G_MODULE_EXPORT GelHubSlave playlist_connector = {
	"playlist",
	&playlist_init,
	&playlist_exit
};

