#define GEL_DOMAIN "Eina::Artwork"
#include <gel/gel.h>
#include <eina/eina-artwork.h>

G_DEFINE_TYPE (EinaArtwork, eina_artwork, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ARTWORK, EinaArtworkPrivate))

enum {
	EINA_ARTWORK_PROPERTY_STREAM = 1,
	EINA_ARTWORK_PROPERTY_DEFAULT_PIXBUF,
	EINA_ARTWORK_PROPERTY_LOADING_PIXBUF
};

enum {
	DEFAULT,
	LOADING,
	N_PIXBUFS
};

enum {
	CHANGE,
	LAST_SIGNAL
};
static guint eina_artwork_signals[LAST_SIGNAL] = { 0 };

typedef struct _EinaArtworkPrivate  EinaArtworkPrivate;
struct _EinaArtworkPrivate {
	LomoStream *stream;
	GList      *providers, *running;
	gboolean    artwork_found;
	GdkPixbuf  *pixbufs[N_PIXBUFS];
};


static void
run_queue(EinaArtwork *self);
static void
set_internal_pixbuf(EinaArtwork *self, gint n, GdkPixbuf *pixbuf);
static GdkPixbuf *
get_internal_pixbuf(EinaArtwork *self, gint n);
static void
set_pixbuf(EinaArtwork *self, GdkPixbuf *pb);

static void
eina_artwork_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case EINA_ARTWORK_PROPERTY_STREAM:
		g_value_set_object(value, (gpointer) eina_artwork_get_stream((EinaArtwork *) object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_artwork_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case EINA_ARTWORK_PROPERTY_STREAM:
		eina_artwork_set_stream((EinaArtwork *) object,  g_value_get_object(value));
		break;
	case EINA_ARTWORK_PROPERTY_DEFAULT_PIXBUF:
		eina_artwork_set_default_pixbuf((EinaArtwork *) object, g_value_get_object(value));
		break;
	case EINA_ARTWORK_PROPERTY_LOADING_PIXBUF:
		eina_artwork_set_loading_pixbuf((EinaArtwork *) object, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_artwork_dispose (GObject *object)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(object);
	if (priv->providers)
	{
		gel_list_deep_free(priv->providers, eina_artwork_provider_free);
		priv->providers = priv->running = NULL;
	}
	gint i;
	for (i = 0 ; i < N_PIXBUFS; i++)
		gel_free_and_invalidate(GET_PRIVATE(object)->pixbufs[i], NULL, g_object_unref);
	G_OBJECT_CLASS (eina_artwork_parent_class)->dispose (object);
}

static void
eina_artwork_finalize (GObject *object)
{
	G_OBJECT_CLASS (eina_artwork_parent_class)->finalize (object);
}

static void
eina_artwork_class_init (EinaArtworkClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaArtworkPrivate));

	object_class->get_property = eina_artwork_get_property;
	object_class->set_property = eina_artwork_set_property;
	object_class->dispose = eina_artwork_dispose;
	object_class->finalize = eina_artwork_finalize;

	// Add properties
    g_object_class_install_property(object_class, EINA_ARTWORK_PROPERTY_STREAM,
		g_param_spec_object("lomo-stream", "Lomo stream", "Lomo stream",
		LOMO_TYPE_STREAM,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
    g_object_class_install_property(object_class, EINA_ARTWORK_PROPERTY_DEFAULT_PIXBUF,
		g_param_spec_object("default-pixbuf", "Default pixbuf", "Default pixbuf",
		GDK_TYPE_PIXBUF,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
    g_object_class_install_property(object_class, EINA_ARTWORK_PROPERTY_LOADING_PIXBUF,
		g_param_spec_object("loading-pixbuf", "Loading pixbuf", "Loading pixbuf",
		GDK_TYPE_PIXBUF,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	// Signals
	eina_artwork_signals[CHANGE] = g_signal_new ("change",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaArtworkClass, change),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,
		0);
}

static void
eina_artwork_init (EinaArtwork *self)
{
}

EinaArtwork*
eina_artwork_new (void)
{
	return g_object_new (EINA_TYPE_ARTWORK, NULL);
}

// --
// Properties
// --
void
eina_artwork_set_stream(EinaArtwork *self, LomoStream *stream)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);

	// Stop any running providers
	if (priv->running)
	{
		// gel_warn("Provider running. Cancel it");
		eina_artwork_provider_cancel((EinaArtworkProvider *) priv->running->data, self);
		priv->running = NULL;
	}

	// Stop if stream == NULL
	if ((priv->stream = stream) == NULL)
		return;

	// Run queue if there is any provider
	priv->artwork_found = FALSE;
	if ((priv->running = priv->providers) != NULL)
	{
		// gel_warn("Restart providers queue");
		run_queue(self);
	}
	else if (priv->pixbufs[DEFAULT])
	{
		set_pixbuf(self, priv->pixbufs[DEFAULT]);
	}
}

LomoStream *
eina_artwork_get_stream(EinaArtwork *self)
{
	return GET_PRIVATE(self)->stream;
}

void
eina_artwork_set_default_pixbuf(EinaArtwork *self, GdkPixbuf *pixbuf)
{
	set_internal_pixbuf(self, DEFAULT, pixbuf);

	// If there is no running provider use this pixbuf
	if (!GET_PRIVATE(self)->running && pixbuf)
	{
		gel_warn("Set default pb to %p", pixbuf);
		set_pixbuf(self, pixbuf);
	}
}

GdkPixbuf *
eina_artwork_get_default_pixbuf(EinaArtwork *self)
{
	return get_internal_pixbuf(self, DEFAULT);
}

void
eina_artwork_set_loading_pixbuf(EinaArtwork *self, GdkPixbuf *pixbuf)
{
	set_internal_pixbuf(self, LOADING, pixbuf);

	// If there is a running provider use this pixbuf
	if (GET_PRIVATE(self)->running && pixbuf)
		set_pixbuf(self, pixbuf);
}

GdkPixbuf *
eina_artwork_get_loading_pixbuf(EinaArtwork *self)
{
	return get_internal_pixbuf(self, LOADING);
}

// --
// API
// --

gboolean
eina_artwork_add_provider(EinaArtwork *self,
	const gchar *name,
	EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel,
	gpointer provider_data)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);

	// Search plugin
	if (!eina_artwork_have_provider(self, name))
		return FALSE;

	// Add provider al end of queue
	priv->providers = g_list_append(priv->providers, eina_artwork_provider_new(name, search, cancel, provider_data));

	// Run queue under some circumstances
	if (priv->stream && !priv->artwork_found && (priv->providers->next == NULL))
	{
		priv->running = priv->providers;
		run_queue(self);
	}
	return TRUE;
}

gboolean
eina_artwork_remove_provider(EinaArtwork *self, const gchar *name)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);
	gboolean call_run_queue = FALSE;

	if (priv->running && g_str_equal(eina_artwork_provider_get_name(EINA_ARTWORK_PROVIDER(priv->running->data)), name))
	{
		// gel_warn("Removing currently running provider, go to next provider");
		eina_artwork_provider_cancel((EinaArtworkProvider*) priv->running->data, self);
		priv->running = priv->running->next;
		call_run_queue = TRUE;
	}

	GList *iter = eina_artwork_find_provider(self, name);
	if (iter)
	{
		priv->providers = g_list_remove_link(priv->providers, iter);
		eina_artwork_provider_free(iter->data);
		g_list_free(iter);
	}

	if (call_run_queue)
		run_queue(self);

	return TRUE;
}

void
eina_artwork_provider_success(EinaArtwork *self, GType type, gpointer data)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);
	if (type == G_TYPE_STRING)
	{
		GdkPixbuf *pb = gdk_pixbuf_new_from_file((gchar *) data, NULL);
		set_pixbuf(self, pb);
		g_object_unref(pb);
	}
	else if (type == GDK_TYPE_PIXBUF)
	{
		set_pixbuf(self, GDK_PIXBUF(data));
	}
	else
	{
		gel_warn("Unknow result type: %s", g_type_name(type));
		return;
	}

	// Set queue to idle
	priv->running = NULL;
	priv->artwork_found = TRUE;
}

void
eina_artwork_provider_fail(EinaArtwork *self)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);

	// Move to next provider
	priv->running = priv->running->next;

	// Run queue
	run_queue(self);
}

GList *
eina_artwork_find_provider(EinaArtwork *self, const gchar *name)
{
	GList *iter = GET_PRIVATE(self)->providers;
	while (iter)
	{
		if (g_str_equal(eina_artwork_provider_get_name(EINA_ARTWORK_PROVIDER(iter->data)), name))
			return iter;
		iter = iter->next;
	}
	return NULL;
}

static void
run_queue(EinaArtwork *self)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);

	// Do some chechks
	if (priv->stream == NULL)
	{
		gel_error("run_queue was called with no stream information. This is a bug in eina's code.\n"
		  "Please report this at the address showed at Help -> Report bug.");
		priv->running = NULL;
		return;
	}

	// Got end of queue.
	if (priv->running == NULL)
	{
		if (priv->pixbufs[DEFAULT])
			set_pixbuf(self, priv->pixbufs[DEFAULT]);
		// gel_warn("End of providers queue.");
		return;
	}

	// gel_warn("Running provider %d of %d",  g_list_position(priv->providers, priv->running) + 1, g_list_length(priv->providers));
	eina_artwork_provider_search((EinaArtworkProvider*) priv->running->data, self);
}

static void
set_internal_pixbuf(EinaArtwork *self, gint n, GdkPixbuf *pixbuf)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);
	if (!priv || (n >= N_PIXBUFS))
		return;
	gel_free_and_invalidate(priv->pixbufs[n], NULL, g_object_unref);
	priv->pixbufs[n] = pixbuf;
}

static GdkPixbuf *
get_internal_pixbuf(EinaArtwork *self, gint n)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);
	if (!priv || (n >= N_PIXBUFS))
		return NULL;
	return priv->pixbufs[n];
}

static void
set_pixbuf(EinaArtwork *self, GdkPixbuf *pb)
{
	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pb, GTK_WIDGET(self)->allocation.width, GTK_WIDGET(self)->allocation.height, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf(GTK_IMAGE(self), scaled);
	g_signal_emit(self, eina_artwork_signals[CHANGE], 0);
}

// --
// EinaArtworkProvider implementation
// --
struct _EinaArtworkProvider {
	const gchar *name;
	EinaArtworkProviderSearchFunc search;
	EinaArtworkProviderCancelFunc cancel;
	gpointer data;
};

EinaArtworkProvider *
eina_artwork_provider_new(const gchar *name, EinaArtworkProviderSearchFunc search, EinaArtworkProviderCancelFunc cancel, gpointer provider_data)
{
	EinaArtworkProvider *self = g_new0(EinaArtworkProvider, 1);
	self->name   = name;
	self->search = search;
	self->cancel = cancel;
	self->data   = provider_data;
	return self;
}

void
eina_artwork_provider_free(EinaArtworkProvider *self)
{
	g_free(self);
}

const gchar *
eina_artwork_provider_get_name(EinaArtworkProvider *self)
{
	return self->name;
}

void
eina_artwork_provider_search(EinaArtworkProvider *self, EinaArtwork *artwork)
{
	if (self->search)
		self->search(artwork, GET_PRIVATE(artwork)->stream, self->data);
}

void
eina_artwork_provider_cancel(EinaArtworkProvider *self, EinaArtwork *artwork)
{
	if (self->cancel)
		self->cancel(artwork, self->data);
}

