#include <glib/gi18n.h>
#include "eina-muine.h"
#include "eina-muine-ui.h"

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
};

enum {
	PROP_ADB = 1,
	PROP_LOMO,
	PROP_ART,
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
muine_update_icon(EinaMuine *self, ArtSearch *search);
static GList *
muine_get_uris_from_tree_iter(EinaMuine *self, GtkTreeIter *iter);

static void
row_activated_cb(GtkWidget *w, GtkTreePath *path, GtkTreeViewColumn *column, EinaMuine *self);
static void
search_cb(Art *art, ArtSearch *search, EinaMuine *self);

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

	case PROP_ART:
		g_value_set_pointer(value, eina_muine_get_art((EinaMuine *) object));
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

	case PROP_ART:
		eina_muine_set_art((EinaMuine *) object, g_value_get_pointer(value));
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

	g_object_class_install_property(object_class, PROP_ART,
		g_param_spec_pointer("art", "art", "art", G_PARAM_WRITABLE | G_PARAM_READABLE)
		);
}

static void
eina_muine_init (EinaMuine *self)
{
}

EinaMuine*
eina_muine_new (void)
{
	EinaMuine *self = g_object_new (EINA_TYPE_MUINE, "xml-string", __eina_muine_ui_xml, NULL);

	GelUIGeneric *ui_generic = GEL_UI_GENERIC(self);
	EinaMuinePrivate *priv = GET_PRIVATE(self);

	priv->listview = gel_ui_generic_get_typed(ui_generic, GTK_TREE_VIEW, "list-view");
	priv->filter   = gel_ui_generic_get_typed(ui_generic, GTK_TREE_MODEL_FILTER, "model-filter");
	priv->model    = gel_ui_generic_get_typed(ui_generic, GTK_LIST_STORE, "model");

	gtk_tree_view_set_model(priv->listview, (GtkTreeModel *) priv->model);
	g_object_set(gel_ui_generic_get_object(ui_generic, "markup-renderer"),
		"yalign", 0.0f,
		NULL);

	g_signal_connect(priv->listview, "row-activated", (GCallback) row_activated_cb, self);
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
eina_muine_set_art(EinaMuine *self, EinaArt *art)
{
	g_return_if_fail(EINA_IS_MUINE(self));

	GET_PRIVATE(self)->art = art;
	// Dont force refresh, too expensive

	g_object_notify((GObject *) self, "art");
}

EinaArt*
eina_muine_get_art(EinaMuine *self)
{
	g_return_val_if_fail(EINA_IS_MUINE(self), NULL);
	return GET_PRIVATE(self)->art;
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
	return EINA_MUINE_MODE_ALBUM;
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
	Art *art = eina_muine_get_art(self);

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
		ArtSearch *search = NULL;
		
		if (art && (db_artist || db_album))
			fake_stream = lomo_stream_new("file:///dev/null");

		if (db_artist)
		{
			artist = g_markup_escape_text(db_artist, -1);
			if (art)
				lomo_stream_set_tag(fake_stream, LOMO_TAG_ARTIST, db_artist);
		}

		if (db_album)
		{
			album  = g_markup_escape_text(db_album,  -1);
			if (art)
				lomo_stream_set_tag(fake_stream, LOMO_TAG_ALBUM,  db_album);
		}

		if (art)
			search = art_search(art, fake_stream, (ArtFunc) search_cb, self);

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
		gtk_list_store_insert_with_values(model, NULL, 0,
			COMBO_COLUMN_MARKUP, markup,
			COMBO_COLUMN_ID,     (mode == EINA_MUINE_MODE_ALBUM) ? db_album : db_artist,
			COMBO_COLUMN_SEARCH, search,
			// COMBO_COLUMN_ICON,   self->default_icon,
			-1);
		g_free(markup);
		gel_free_and_invalidate(artist, NULL, g_free);
		gel_free_and_invalidate(album,  NULL, g_free);
	}
	eina_adb_result_free(r);
}

static void
muine_update_icon(EinaMuine *self, ArtSearch *search)
{
	GtkTreeModel *model = (GtkTreeModel *) muine_get_model(self);

	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(model, &iter))
	{
		g_warning(N_("Cannot get first iter"));
		return;
	}

	GdkPixbuf *icon = art_search_get_result(search);
	GdkPixbuf *scaled = (icon && GDK_IS_PIXBUF(icon)) ? gdk_pixbuf_scale_simple(icon, DEFAULT_SIZE, DEFAULT_SIZE, GDK_INTERP_NEAREST) : NULL;

	ArtSearch *test;
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
	g_warning("Search NOT found");
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
search_cb(Art *art, ArtSearch *search, EinaMuine *self)
{
	muine_update_icon(self, search);
}

