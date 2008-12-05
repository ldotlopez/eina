#define GEL_DOMAIN "GelHub"
#include <gmodule.h>
#include <gel/gel.h>
#include <gel/gel-hub-marshal.h>

G_DEFINE_TYPE (GelHub, gel_hub, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_HUB, GelHubPrivate))

/*
 * Private fields
 */
typedef struct _GelHubPrivate GelHubPrivate;
struct _GelHubPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *slave_table;

	GelHubCallback dispose_func;
	gpointer dispose_data;
};

/*
 * Signals
 */
typedef enum {
	MODULE_INIT,
	MODULE_FINI,
	MODULE_LOAD,
	MODULE_UNLOAD,
	MODULE_REF,
	MODULE_UNREF,

	LAST_SIGNAL
} GelHubSignalType;
static guint gel_hub_signals[LAST_SIGNAL] = { 0 };

/*
 * "server-side" of GelHubSlave
 */
typedef struct GelHubHost {
	GelHubSlave    *slave;
	gpointer      data;
	GModule      *module;
	guint         refs;
} GelHubHost;

static void
gel_hub_dispose(GObject *object)
{
	GelHub *self = GEL_HUB(object);
	GelHubPrivate *priv = GET_PRIVATE(self);

	if (priv->dispose_func != NULL)
	{
		priv->dispose_func(self, priv->dispose_data);
		priv->dispose_func = NULL;
	}

	if (self->opt_cntxt)
	{
		g_option_context_free(self->opt_cntxt);
		self->opt_cntxt = NULL;
	}

	if (priv->slave_table)
	{
	    g_hash_table_unref(priv->slave_table);
		priv->slave_table = NULL;
	}

	if (G_OBJECT_CLASS (gel_hub_parent_class)->dispose)
		G_OBJECT_CLASS (gel_hub_parent_class)->dispose (object);
}

static void
gel_hub_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (gel_hub_parent_class)->finalize)
		G_OBJECT_CLASS (gel_hub_parent_class)->finalize (object);
}

static void
gel_hub_class_init (GelHubClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelHubPrivate));

	object_class->dispose = gel_hub_dispose;
	object_class->finalize = gel_hub_finalize;

	/* Create the class: insert signals, properties, etc ... */
    gel_hub_signals[MODULE_INIT] = g_signal_new ("module-init",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_init),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    gel_hub_signals[MODULE_FINI] = g_signal_new ("module-fini",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_fini),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* Load / unload */
    gel_hub_signals[MODULE_LOAD] = g_signal_new ("module-load",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_load),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    gel_hub_signals[MODULE_UNLOAD] = g_signal_new ("module-unload",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_unload),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* ref / unref */
    gel_hub_signals[MODULE_REF] = g_signal_new ("module-ref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_ref),
        NULL, NULL,
        gel_hub_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
    gel_hub_signals[MODULE_UNREF] = g_signal_new ("module-unref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GelHubClass, module_unref),
        NULL, NULL,
        gel_hub_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
}

static void
gel_hub_init (GelHub *self)
{
	GelHubPrivate *priv = GET_PRIVATE(self);

    self->opt_cntxt = g_option_context_new(NULL);
	priv->dispose_func = NULL;
	priv->dispose_data = NULL;

	priv->slave_table = g_hash_table_new(g_str_hash, g_str_equal);
}

GelHub*
gel_hub_new (gint *argc, gchar **argv[])
{
	GelHub *self = g_object_new (GEL_TYPE_HUB, NULL);
	GelHubPrivate *priv = GET_PRIVATE(self);

	priv->argc = argc;
	priv->argv = argv;

	return self;
}

void
gel_hub_set_dispose_callback(GelHub *self, GelHubCallback callback, gpointer user_data)
{
	GelHubPrivate *priv = GET_PRIVATE(self);
	priv->dispose_func = callback;
	priv->dispose_data = user_data;
}

GelHubHost *gel_hub_host_get(GelHub *self, gchar *name) {
	return g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
}

gboolean gel_hub_load  (GelHub *self, gchar *name) {
	GelHubPrivate *priv = GET_PRIVATE(self);
	GelHubHost  *host;
	GList     *plugin_list;
	gchar     *plugin_file;
	gchar     *symbol_name;

	gint i;
	gchar  *f;

	/* Check if the module is already loaded,
	 * If its loaded increment is refcount */
	host = gel_hub_host_get(self, name);
	if (host != NULL) {
		host->refs++;
		g_signal_emit(G_OBJECT(self), gel_hub_signals[MODULE_REF], 0, name, host->refs);
		return TRUE;
	}

	/* Build a list of posible files containing the code */
	plugin_file = g_strconcat("lib", name, ".so", NULL);
	plugin_list = gel_app_resource_get_list(GEL_APP_RESOURCE_LIB, plugin_file);
	g_free(plugin_file);

	/* Add a special case for open argv[0] */
	plugin_list = g_list_append(plugin_list, NULL); // Special case for open argv[0]

	/* Now lookup for the symbol */
	host        = g_new0(GelHubHost, 1);
	symbol_name = g_strconcat(name, "_connector", NULL);

	
	for ( i = 0; i < g_list_length(plugin_list); i++) {
		f = (gchar *) g_list_nth_data(plugin_list, i);

		host->module = g_module_open(f, G_MODULE_BIND_LAZY);
		if (host->module == NULL) {
			continue;
		}

		if (!g_module_symbol(host->module, symbol_name, (gpointer) &(host->slave))) {
			g_module_close(host->module);
			continue;
		}

		if (host->slave->init_func == NULL) {
			g_module_close(host->module);
			continue;
		}

		/* Insert the host in hub before exec init to enable component to use core's mem */
		g_hash_table_insert(priv->slave_table, name, host);
		if (((GelHubSlaveInitFunc) host->slave->init_func) (self, priv->argc, priv->argv)) {
			/* If init funcion returns TRUE, insert plugin into GelHub */
			g_signal_emit(G_OBJECT(self), gel_hub_signals[MODULE_LOAD], 0, name);
			host->refs++;

			//g_signal_emit(G_OBJECT(self), gel_hub_signals[MODULE_REF], 0, name, host->refs);
			gel_list_deep_free(plugin_list, g_free);
			return TRUE;
		} else {
			/* Init func failed: remove the mem and close module */
			g_hash_table_remove(priv->slave_table, name);
			g_module_close(host->module);
			continue;
		}
	}

	gel_list_deep_free(plugin_list, g_free);
	g_free(host);
	gel_warn("Cannot load %s", name);
	return FALSE;
}

gboolean gel_hub_unload(GelHub *self, gchar *name) {
	GelHubPrivate *priv = GET_PRIVATE(self);
	GelHubHost *host;

	host = gel_hub_host_get(self, name);
	if ( host == NULL )
		return FALSE;

	host->refs--;
	g_signal_emit(G_OBJECT(self), gel_hub_signals[MODULE_UNREF], 0, name, host->refs);
	if (host->refs > 0) {
		return TRUE;
	}
	
	if ( host->slave->exit_func != NULL ) {
		((GelHubSlaveExitFunc) host->slave->exit_func)
			(host->data);
	}

	g_module_close(host->module);
	g_hash_table_remove(priv->slave_table, name);
	g_free(host);

	g_signal_emit(G_OBJECT(self), gel_hub_signals[MODULE_UNLOAD], 0, name);

	return TRUE;
}

gboolean gel_hub_loaded(GelHub *self, gchar *name) {
	if (g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name) != NULL)
		return TRUE;
	else
		return FALSE;
}

gpointer gel_hub_shared_register(GelHub *self, gchar *name, guint size) {
	GelHubHost *host;

	host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
	if ( host != NULL ) {
		gel_warn("Attempt to re-register '%s' in shared memory", name);
		return NULL;
	}
	else {
		host->data = g_malloc0(size);
		return host->data;
	}
}

void gel_hub_shared_unregister(GelHub *self, gchar *name) {
	GelHubHost *host;

	host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
	if ( host == NULL ) {
		gel_warn("Attempt to unregister a non register memory: '%s'", name);
		return;
	}

	g_free(host->data);
}

gpointer gel_hub_shared_get(GelHub *self, gchar *name) {
	GelHubHost *host;

	host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
	return host ? host->data : NULL;
}

gboolean gel_hub_shared_set(GelHub *self, gchar *name, gpointer mem) {
	GelHubHost *host;

    host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);

	/* No host with this name */
	if ( host == NULL ) {
		gel_info("No host");
		return FALSE;
	}

	/* Host already has data. */
	if (host->data != NULL )
		return FALSE;

	/* Get the data */
	host->data = mem;
	return TRUE;
}

