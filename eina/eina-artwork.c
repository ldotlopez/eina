#define GEL_DOMAIN "Eina::Artwork"
#include <gel/gel.h>
#include <eina/eina-artwork.h>

G_DEFINE_TYPE (EinaArtwork, eina_artwork, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ARTWORK, EinaArtworkPrivate))

typedef struct _EinaArtworkPrivate  EinaArtworkPrivate;
struct _EinaArtworkPrivate {
	LomoStream *stream;
	GList      *providers, *running;
};

enum {
	EINA_ARTWORK_PROPERTY_DEFAULT_PATH = 1,
	EINA_ARTWORK_PROPERTY_LOADING_PATH,
	EINA_ARTWORK_PROPERTY_STREAM
};

static void
run_queue(EinaArtwork *self);

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_artwork_dispose (GObject *object)
{
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

void
eina_artwork_set_stream(EinaArtwork *self, LomoStream *stream)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);

	// Stop any running providers
	if (priv->running)
	{
		gel_warn("Provider running. Cancel it");
		eina_artwork_provider_cancel((EinaArtworkProvider *) priv->running->data, self);
		priv->running = NULL;
	}

	priv->stream = stream;

	// Start providers
	gel_warn("Start new queue of providers");
	priv->running = priv->providers;
	run_queue(self);
}

LomoStream *
eina_artwork_get_stream(EinaArtwork *self)
{
	return GET_PRIVATE(self)->stream;
}

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

	return TRUE;
}

gboolean
eina_artwork_remove_provider(EinaArtwork *self, const gchar *name)
{
	EinaArtworkPrivate *priv = GET_PRIVATE(self);
	gboolean call_run_queue = FALSE;

	if (priv->running && g_str_equal(eina_artwork_provider_get_name(EINA_ARTWORK_PROVIDER(priv->running->data)), name))
	{
		gel_warn("Removing currently running provider, go to next provider");
		eina_artwork_provider_cancel((EinaArtworkProvider*) priv->running->data, self);
		priv->running = priv->running->next;
		call_run_queue = TRUE;
	}

	GList *iter = eina_artwork_find_provider(self, name);
	if (iter)
	{
		priv->providers = g_list_remove(priv->providers, iter);
		eina_artwork_provider_free(iter->data);
	}

	if (call_run_queue)
		run_queue(self);

	return TRUE;
}

void
eina_artwork_provider_success(EinaArtwork *self, GType type, gpointer data)
{
	if (type == G_TYPE_STRING)
	{
		gel_warn("Got string: %s", (gchar *) data);
		gtk_image_set_from_file(GTK_IMAGE(self), (gchar *) data);
	}
	else if (type == GDK_TYPE_PIXBUF)
	{
		gel_warn("Got pixbuf: %p", data);
		gtk_image_set_from_pixbuf(GTK_IMAGE(self), GDK_PIXBUF(data));
	}
	else
	{
		gel_warn("Unknow result type");
		return;
	}

	// Set queue to idle
	GET_PRIVATE(self)->running = NULL;
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
	if (priv->stream == NULL)
	{
		gel_warn("No stream. Stop");
		priv->running = NULL;
		return;
	}
	if (priv->running == NULL)
	{
		gel_warn("No provider. Stop");
		return;
	}

	eina_artwork_provider_search((EinaArtworkProvider*) priv->running->data, self);
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

