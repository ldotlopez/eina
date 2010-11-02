/*
 * eina/muine/eina-muine.c
 *
 * Copyright (C) 2004-2010 Eina
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

#include "eina-muine.h"
#include "eina-muine-ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaMuine, eina_muine, GEL_UI_TYPE_GENERIC)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_MUINE, EinaMuinePrivate))

#define DEFAULT_SIZE 64

typedef struct _EinaMuinePrivate EinaMuinePrivate;

struct _EinaMuinePrivate {
	// Props
	EinaAdb    *adb;
	EinaArt    *art;
	LomoPlayer *lomo;
	gint        mode;

	// Internal
	GtkTreeView        *listview;
	GtkTreeModelFilter *filter;
	GtkListStore       *model;
	GtkEntry           *search;
	gchar              *search_str;
};

enum {
	PROP_ADB = 1,
	PROP_LOMO,
	PROP_MODE
};

enum {
	COMBO_COLUMN_ICON,
	COMBO_COLUMN_MARKUP,
	COMBO_COLUMN_ID,
	COMBO_COLUMN_SEARCH,
	COMBO_N_COLUMNS
};

static GtkListStore*
muine_get_model(EinaMuine *self);
static GtkTreeModelFilter*
muine_get_filter(EinaMuine *self);
static void
muine_update(EinaMuine *self);
static void
muine_update_icon(EinaMuine *self, EinaArtSearch *search);
static GList *
muine_get_uris_from_tree_iter(EinaMuine *self, GtkTreeIter *iter);
static gboolean
muine_filter_func(GtkTreeModel *model, GtkTreeIter *iter, EinaMuine *self);

static void
row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaMuine *self);
static void
search_changed_cb(GtkWidget *w, EinaMuine *self);
static void
search_icon_press_cb(GtkWidget *w, GtkEntryIconPosition pos, GdkEvent *ev, EinaMuine *self);
static void
action_activate_cb(GtkAction *action, EinaMuine *self);
static void
search_cb(EinaArtSearch *search, EinaMuine *self);

static void
eina_muine_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_ADB:
		g_value_set_object(value, eina_muine_get_adb((EinaMuine *) object));
		return;

	case PROP_LOMO:
		g_value_set_object(value, eina_muine_get_lomo_player((EinaMuine *) object));
		return;

	case PROP_MODE:
		g_value_set_int(value, eina_muine_get_mode((EinaMuine *) object));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_muine_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_ADB:
		eina_muine_set_adb((EinaMuine *) object, g_value_get_object(value));
		return;

	case PROP_LOMO:
		eina_muine_set_lomo_player((EinaMuine *) object, g_value_get_object(value));
		return;

	case PROP_MODE:
		eina_muine_set_mode((EinaMuine *) object, g_value_get_int(value));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_muine_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_muine_parent_class)->dispose (object);
}

static void
eina_muine_class_init (EinaMuineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = eina_muine_get_property;
	object_class->set_property = eina_muine_set_property;
	object_class->dispose = eina_muine_dispose;

	g_type_class_add_private (klass, sizeof (EinaMuinePrivate));

	g_object_class_install_property(object_class, PROP_ADB,
		g_param_spec_object("adb", "adb", "adb", EINA_TYPE_ADB, G_PARAM_WRITABLE | G_PARAM_READABLE)
		);

	g_object_class_install_property(object_class, PROP_LOMO,
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player", LOMO_TYPE_PLAYER, G_PARAM_WRITABLE | G_PARAM_READABLE)
		);

	g_object_class_install_property(object_class, PROP_MODE,
		g_param_spec_int("mode", "mode", "mode", 0, 1, EINA_MUINE_MODE_ALBUM, G_PARAM_WRITABLE | G_PARAM_READABLE)
		);
}

static void
eina_muine_init (EinaMuine *self)
{
	GET_PRIVATE(self)->art = eina_art_new();
}

EinaMuine*
eina_muine_new (void)
{
	EinaMuine *self = g_object_new (EINA_TYPE_MUINE, "xml-string", __eina_muine_ui_xml, NULL);

	GelUIGeneric *ui_generic = GEL_UI_GENERIC(self);
	EinaMuinePrivate *priv = GET_PRIVATE(self);

	priv->listview = gel_ui_generic_get_typed(ui_generic, GTK_TREE_VIEW,         "list-view");
	priv->filter   = gel_ui_generic_get_typed(ui_generic, GTK_TREE_MODEL_FILTER, "model-filter");
	priv->model    = gel_ui_generic_get_typed(ui_generic, GTK_LIST_STORE,        "model");
	priv->search   = gel_ui_generic_get_typed(ui_generic, GTK_ENTRY,             "search-entry");

	g_object_set(gel_ui_generic_get_object(ui_generic, "markup-renderer"),
		"yalign", 0.0f,
		NULL);
	gtk_tree_model_filter_set_visible_func(priv->filter, (GtkTreeModelFilterVisibleFunc) muine_filter_func, self, NULL);
	g_object_bind_property(self, "mode", gel_ui_generic_get_object(ui_generic, "mode-view"), "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	gchar *actions[] = { "play-action", "queue-action" };
	for (guint i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GObject *a = gel_ui_generic_get_object(ui_generic, actions[i]);
		g_signal_connect(a, "activate", (GCallback) action_activate_cb, self);
	}
	g_signal_connect(priv->listview, "row-activated", (GCallback) row_activated_cb, self);
	g_signal_connect(priv->search,   "changed",       (GCallback) search_changed_cb, self);
	g_signal_connect(priv->search,   "icon-press",    (GCallback) search_icon_press_cb, self);
	return self;
}

void
eina_muine_set_adb(EinaMuine *self, EinaAdb *adb)
{
	g_return_if_fail(EINA_IS_MUINE(self));
	g_return_if_fail(EINA_IS_ADB(adb));

	EinaMuinePrivate *priv = GET_PRIVATE(self);
	gel_free_and_invalidate(priv->adb, NULL, g_object_unref);

	priv->adb = g_object_ref(adb);
	muine_update(self);

	g_object_notify((GObject *) self, "adb");
}

EinaAdb*
eina_muine_get_adb(EinaMuine *self)
{
	g_return_val_if_fail(EINA_IS_MUINE(self), NULL);
	return GET_PRIVATE(self)->adb;
}

void
eina_muine_set_lomo_player(EinaMuine *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_MUINE(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaMuinePrivate *priv = GET_PRIVATE(self);
	gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);
	
	priv->lomo = g_object_ref(lomo);

	g_object_notify((GObject *) self, "lomo-player");
}

LomoPlayer*
eina_muine_get_lomo_player(EinaMuine *self)
{
	g_return_val_if_fail(EINA_IS_MUINE(self), NULL);
	return GET_PRIVATE(self)->lomo;
}

void
eina_muine_set_mode(EinaMuine *self, EinaMuineMode mode)
{
	g_return_if_fail(EINA_IS_MUINE(self));

	GET_PRIVATE(self)->mode = mode;
	muine_update(self);

	g_object_notify((GObject *) self, "mode");
}

EinaMuineMode
eina_muine_get_mode(EinaMuine *self)
{
	g_return_val_if_fail(EINA_IS_MUINE(self), EINA_MUINE_MODE_INVALID);
	return GET_PRIVATE(self)->mode ;
}

// --
// Private
// --
static GtkListStore*
muine_get_model(EinaMuine *self)
{
	return GET_PRIVATE(self)->model;
}

static GtkTreeModelFilter*
muine_get_filter(EinaMuine *self)
{
	return GET_PRIVATE(self)->filter;
}

static void
muine_update(EinaMuine *self)
{
	g_return_if_fail(EINA_IS_MUINE(self));
	EinaMuinePrivate *priv = GET_PRIVATE(self);

	typedef struct {
		guint count;           // How many items have been folded
		gchar *artist, *album; // Metadata from DB
		LomoStream *stream;    // Fake stream
	} data_set_t;

	EinaMuineMode mode = eina_muine_get_mode(self);
	gchar *markup_fmt = NULL;

	// Build master query
	gchar *q = NULL;
	switch (mode)
	{
	case EINA_MUINE_MODE_ALBUM:
		// q = "select count(*) as count,artist,album from fast_meta group by(lower(album)) order by artist ASC";
		q = "select count(*) as count,artist,album from fast_meta group by(album) order by lower(artist) ASC";
		markup_fmt = "<big><b>%s</b></big>\n%s <span size=\"small\" weight=\"light\">(%d streams)</span>";
		break;
	case EINA_MUINE_MODE_ARTIST:
		// q = "select count(*) as count,artist,NULL from fast_meta group by(lower(artist)) order by artist ASC";
		q = "select count(*) as count,artist,NULL from fast_meta group by(artist) order by lower(artist) ASC";
		markup_fmt = "<big><b>%s</b></big>\n<span size=\"small\" weight=\"light\">(%d streams)</span>";
		break;
	default:
		g_warning(N_("Unknow mode: %d"), mode);
		return;
	}

	// Now fill the data_store;
	EinaAdbResult *r = eina_adb_query(eina_muine_get_adb(self), q, NULL);

	GList *db_data = NULL;
	data_set_t *ds = NULL;
	while (eina_adb_result_step(r))
	{
		if (ds == NULL)
			ds = g_new0(data_set_t, 1);

		if (!eina_adb_result_get(r,
		     0, G_TYPE_UINT,   &(ds->count),
		     1, G_TYPE_STRING, &(ds->artist),
		     2, G_TYPE_STRING, &(ds->album),
		     -1))
		{
			g_warning(N_("Failed to get result row"));
			continue;
		}

		db_data = g_list_prepend(db_data, ds);
		ds = NULL;
	}
	eina_adb_result_free(r);

	// Try to get a sample for each item
	// q = "select uri from streams where sid = (select sid from fast_meta where lower(%s)=lower('%q') limit 1 offset %d)";
	q = "select uri from streams where sid = (select sid from fast_meta where %s='%q' limit 1 offset %d)";
	EinaAdb *adb      = eina_muine_get_adb(self);
	gchar *sample_uri = NULL;
	gchar *field = (mode == EINA_MUINE_MODE_ALBUM) ? "album" : "artist";
	gchar *key   = NULL;

	GList *ds_p = db_data;
	while (ds_p)
	{
		data_set_t *ds = (data_set_t *) ds_p->data;

		char *q2 = sqlite3_mprintf(q, 
			field,
			key = ((mode == EINA_MUINE_MODE_ALBUM) ? ds->album : ds->artist),
			g_random_int_range(0, ds->count));
			
		EinaAdbResult *sr = eina_adb_query_raw(adb, q2);
		if (!sr || !eina_adb_result_step(sr) || !eina_adb_result_get(sr, 0, G_TYPE_STRING, &sample_uri, -1))
		{
			g_warning(N_("Unable to fetch sample URI for %s '%s', query was %s"), field, key, q2);
			sample_uri = g_strdup("file:///nonexistent");
		}
		gel_free_and_invalidate(sr, NULL, eina_adb_result_free);
		gel_free_and_invalidate(q2, NULL, sqlite3_free);

		ds->stream = lomo_stream_new(sample_uri);
		g_free(sample_uri);

		ds_p = ds_p->next;
	}

	// All data (and all I/O) from DB has been fetched, insert into interface
	gtk_list_store_clear(muine_get_model(self));
	ds_p = db_data;
	GtkListStore *model = muine_get_model(self);
	gchar *artist = NULL, *album = NULL; gchar *markup = NULL;
	while (ds_p)
	{
		data_set_t *ds = (data_set_t *) ds_p->data;

		if (ds->artist)
		{
			artist = g_markup_escape_text(ds->artist, -1);
			lomo_stream_set_tag(ds->stream, LOMO_TAG_ARTIST, ds->artist);
		}

		if (ds->album)
		{
			album  = g_markup_escape_text(ds->album,  -1);
			lomo_stream_set_tag(ds->stream, LOMO_TAG_ALBUM, ds->album);
		}

		switch (mode)
		{
		case EINA_MUINE_MODE_INVALID:
		case EINA_MUINE_MODE_ALBUM:
			markup = g_strdup_printf(markup_fmt, album, artist, ds->count);
			break;
		case EINA_MUINE_MODE_ARTIST:
			markup = g_strdup_printf(markup_fmt, artist, ds->count);
			break;
		}

		GtkTreeIter iter;
		gtk_list_store_insert_with_values(model, &iter, 0,
			COMBO_COLUMN_MARKUP, markup,
			COMBO_COLUMN_ID,     (mode == EINA_MUINE_MODE_ALBUM) ? ds->album : ds->artist,
			COMBO_COLUMN_SEARCH, eina_art_search(priv->art, ds->stream, (EinaArtSearchCallback) search_cb, self),
			// COMBO_COLUMN_ICON,   self->default_icon,
			-1);
		g_free(ds->artist);
		g_free(ds->album);
		g_free(ds);
		g_free(markup);
		gel_free_and_invalidate(artist, NULL, g_free);
		gel_free_and_invalidate(album,  NULL, g_free);

		ds_p = ds_p->next;
	}
	g_list_free(db_data);

	if (0) search_cb(NULL, NULL);
#if 0
	gtk_list_store_clear(muine_get_model(self));

	EinaMuineMode mode = eina_muine_get_mode(self);

	gchar *q = NULL;
	gchar *markup_fmt = NULL;
	switch (mode)
	{
	case EINA_MUINE_MODE_ALBUM:
		q = "select count(*) as count,artist,album from fast_meta group by(lower(album)) order by album DESC";
		markup_fmt = "<big><b>%s</b></big>\n%s <span size=\"small\" weight=\"light\">(%d streams)</span>";
		break;
	case EINA_MUINE_MODE_ARTIST:
		q = "select count(*) as count,artist,NULL from fast_meta group by(lower(artist)) order by artist DESC";
		markup_fmt = "<big><b>%s</b></big>\n<span size=\"small\" weight=\"light\">(%d streams)</span>";
		break;
	default:
		g_warning(N_("Unknow mode: %d"), mode);
		return;
	}

	EinaAdbResult *r = eina_adb_query(eina_muine_get_adb(self), q, NULL);
	if (!r)
	{
		g_warning("Query failed");
		return;
	}

	gchar *db_album = NULL, *db_artist = NULL;
	gchar *album = NULL, *artist = NULL;
	gint   count;

	GtkListStore *model = muine_get_model(self);

	while (eina_adb_result_step(r))
	{
		if (!eina_adb_result_get(r,
			0, G_TYPE_UINT, &count,
			1, G_TYPE_STRING, &db_artist,
			2, G_TYPE_STRING, &db_album,
			-1))
		{
			g_warning("Cannot get row results");
			continue;
		}

		LomoStream *fake_stream = NULL;
		
		if (priv->art && (db_artist || db_album))
		{
			gchar *stream_q = NULL;
			guint random_n = g_random_int_range(0, count);
			switch (mode)
			{
			case EINA_MUINE_MODE_ALBUM:
				stream_q = sqlite3_mprintf("select uri from streams where sid = (select sid from fast_meta where album = \"%q\" limit 1 offset %d)", db_album, random_n);
				break;
			case EINA_MUINE_MODE_ARTIST:
				stream_q = sqlite3_mprintf("select uri from streams where sid = (select sid from fast_meta where artist = \"%q\" limit 1 offset %d)", db_artist, random_n);
				break;
			default:
				break;
			}
			if (stream_q)
			{
				gchar *stream_uri = NULL;
				EinaAdbResult *stream_r = NULL;
				if (!(stream_r = eina_adb_query(eina_muine_get_adb(self), stream_q, NULL)) ||
				    !eina_adb_result_step(r) ||
					!eina_adb_result_get(stream_r, 0, G_TYPE_STRING, &stream_uri, -1))
				{
					g_warning("Unable to fetch a random stream");
				}
				else
				{
					if (stream_uri)
					{
					fake_stream = lomo_stream_new(stream_uri);
					g_free(stream_uri);
					} eina_adb_result_free(stream_r);
				}
			}
			
			if (!fake_stream)
				fake_stream = lomo_stream_new("file:///dev/null");
		}

		if (db_artist)
		{
			artist = g_markup_escape_text(db_artist, -1);
			lomo_stream_set_tag(fake_stream, LOMO_TAG_ARTIST, db_artist);
		}

		if (db_album)
		{
			album  = g_markup_escape_text(db_album,  -1);
			lomo_stream_set_tag(fake_stream, LOMO_TAG_ALBUM,  db_album);
		}

		gchar *markup = NULL;
		switch (mode)
		{
		case EINA_MUINE_MODE_INVALID:
		case EINA_MUINE_MODE_ALBUM:
			markup = g_strdup_printf(markup_fmt, album, artist, count);
			break;
		case EINA_MUINE_MODE_ARTIST:
			markup = g_strdup_printf(markup_fmt, artist, count);
			break;
		}
		GtkTreeIter iter;
		gtk_list_store_insert_with_values(model, &iter, 0,
			COMBO_COLUMN_MARKUP, markup,
			COMBO_COLUMN_ID,     (mode == EINA_MUINE_MODE_ALBUM) ? db_album : db_artist,
			COMBO_COLUMN_SEARCH, eina_art_search(priv->art, fake_stream, (EinaArtSearchCallback) search_cb, self),
			// COMBO_COLUMN_ICON,   self->default_icon,
			-1);
		g_free(markup);
		gel_free_and_invalidate(artist, NULL, g_free);
		gel_free_and_invalidate(album,  NULL, g_free);
	}
	eina_adb_result_free(r);
#endif
}

static void
muine_update_icon(EinaMuine *self, EinaArtSearch *search)
{
	g_return_if_fail(EINA_IS_MUINE(self));
	g_return_if_fail(EINA_IS_ART_SEARCH(search));

	GtkTreeModel *model = (GtkTreeModel *) muine_get_model(self);

	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(model, &iter))
	{
		g_warning(N_("Cannot get first iter"));
		return;
	}

	GdkPixbuf *icon = eina_art_search_get_result(search);
	GdkPixbuf *scaled = (icon && GDK_IS_PIXBUF(icon)) ? gdk_pixbuf_scale_simple(icon, DEFAULT_SIZE, DEFAULT_SIZE, GDK_INTERP_NEAREST) : NULL;

	EinaArtSearch *test;
	do {
		gtk_tree_model_get(model, &iter,
			COMBO_COLUMN_SEARCH, &test,
			-1);
		if (test == search)
		{
			if (scaled)
				gtk_list_store_set((GtkListStore *) model, &iter,
					COMBO_COLUMN_ICON, scaled,
					COMBO_COLUMN_SEARCH, NULL,
					-1);
			else
				gtk_list_store_set((GtkListStore *) model, &iter,
					COMBO_COLUMN_SEARCH, NULL,
					-1);
			return;
		}
	} while (gtk_tree_model_iter_next(model, &iter));
}

static GList *
muine_get_uris_from_tree_iter(EinaMuine *self, GtkTreeIter *iter)
{
	gchar *id = NULL;

	gtk_tree_model_get((GtkTreeModel *) muine_get_filter(self), iter,
		COMBO_COLUMN_ID, &id,
		-1);
	
	char *q = NULL;
	switch (eina_muine_get_mode(self))
	{
	case EINA_MUINE_MODE_INVALID:
	case EINA_MUINE_MODE_ALBUM:
		q = "select uri from streams where sid in (select sid from fast_meta where lower(album)=lower('%q'))";
		break;
	case EINA_MUINE_MODE_ARTIST:
		q = "select uri from streams where sid in (select sid from fast_meta where lower(artist)=lower('%q'))";
		break;
	default:
		g_warning(N_("Unknow mode"));
		return NULL;
	}

	EinaAdb *adb = eina_muine_get_adb(self);
	EinaAdbResult *r = eina_adb_query(adb, q, id);
	if (r == NULL)
	{
		g_warning(N_("Unable to build query '%s'"), q);
		return NULL;
	}

	GList *uris = NULL;
	gchar *uri;
	while (eina_adb_result_step(r))
		if (eina_adb_result_get(r, 0, G_TYPE_STRING, &uri, -1))
			uris = g_list_prepend(uris, uri);
	eina_adb_result_free(r);

	return g_list_reverse(uris);
}

static gboolean
muine_filter_func(GtkTreeModel *model, GtkTreeIter *iter, EinaMuine *self)
{
	EinaMuinePrivate *priv = GET_PRIVATE(self);
	if (priv->search_str == NULL)
		return TRUE;

	gchar *markup = NULL;
	gtk_tree_model_get(model, iter,
		COMBO_COLUMN_MARKUP, &markup,
		-1
		);
	g_return_val_if_fail(markup != NULL, FALSE);

	gchar *haystack = g_utf8_casefold(markup, -1);
	g_free(markup);

	gboolean ret = (strstr(haystack, priv->search_str) != NULL);
	g_free(haystack);
	return ret;
}

static void
row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaMuine *self)
{
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(muine_get_filter(self)), &iter, path))
	{
		g_warning(N_("Cannot get iter  for model"));
		return;
	}

	GList *uris = muine_get_uris_from_tree_iter(self, &iter);
	if (uris == NULL)
	{
		g_warning(N_("NULL result from ADB"));
		return;
	}

	LomoPlayer *lomo = eina_muine_get_lomo_player(self);
	lomo_player_clear(lomo);
	lomo_player_append_uri_multi(lomo, uris);

	gel_list_deep_free(uris, g_free);
}

static void
search_changed_cb(GtkWidget *w, EinaMuine *self)
{
	EinaMuinePrivate *priv = GET_PRIVATE(self);

	const gchar *search_str = gtk_entry_get_text(GTK_ENTRY(w));
	if (search_str && (search_str[0] == '\0'))
		search_str = NULL;

	// From no-search to search
	if ((priv->search_str == NULL) && (search_str != NULL))
	{
		priv->search_str = g_utf8_casefold(search_str, -1);
		gtk_tree_model_filter_refilter(muine_get_filter(self));
	}

	// From search to more complex search
	else if ((priv->search_str != NULL) && (search_str != NULL))
	{
		g_free(priv->search_str);
		priv->search_str = g_utf8_casefold(search_str, -1);
		gtk_tree_model_filter_refilter(muine_get_filter(self));
	}

	// From search to no search
	else if ((priv->search_str != NULL) && (search_str == NULL))
	{
		gel_free_and_invalidate(priv->search_str, NULL, g_free);
		gtk_tree_model_filter_refilter(muine_get_filter(self));
	}

	else
		g_warning(N_("Unhandled situation"));
}

static void
search_icon_press_cb(GtkWidget *w, GtkEntryIconPosition pos, GdkEvent *ev, EinaMuine *self)
{
	if (pos == GTK_ENTRY_ICON_SECONDARY)
		gtk_entry_set_text(GTK_ENTRY(w), "");
}

static void
action_activate_cb(GtkAction *action, EinaMuine *self)
{
	const gchar *name = gtk_action_get_name(action);
	gboolean do_clear = FALSE;

	if (g_str_equal(name, "play-action"))
		do_clear = TRUE;
	else if (g_str_equal(name, "queue-action"))
		do_clear = FALSE;
	else
	{
		g_warning(N_("Unknow action %s"), name);
		return;
	}

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GET_PRIVATE(self)->listview);
	if (!sel)
		return;

	GtkTreeIter iter;
	if (!gtk_tree_selection_get_selected(sel, NULL, &iter))
		return;

	GList *uris = muine_get_uris_from_tree_iter(self, &iter);
	if (!uris)
		return;

	LomoPlayer *lomo = eina_muine_get_lomo_player(self);
	if (do_clear)
		lomo_player_clear(lomo);
	lomo_player_append_uri_multi(lomo, uris);
	gel_list_deep_free(uris, g_free);
}

static void
search_cb(EinaArtSearch *search, EinaMuine *self)
{
	muine_update_icon(self, search);
}

