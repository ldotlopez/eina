#define GEL_DOMAIN "Gel::GHub"
#include <gmodule.h>
#include <gel/gel.h>
#include "ghub.h"
#include "ghub-marshal.h"

G_DEFINE_TYPE (GHub, g_hub, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), G_TYPE_HUB, GHubPrivate))

/*
 * Private fields
 */
typedef struct _GHubPrivate GHubPrivate;
struct _GHubPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *slave_table;

	GHubCallback dispose_func;
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
} GHubSignalType;
static guint g_hub_signals[LAST_SIGNAL] = { 0 };

/*
 * "server-side" of GHubSlave
 */
typedef struct GHubHost {
	GHubSlave    *slave;
	gpointer      data;
	GModule      *module;
	guint         refs;
} GHubHost;

static void
g_hub_dispose(GObject *object)
{
	GHub *self = G_HUB(object);
	GHubPrivate *priv = GET_PRIVATE(self);

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

	if (G_OBJECT_CLASS (g_hub_parent_class)->dispose)
		G_OBJECT_CLASS (g_hub_parent_class)->dispose (object);
}

static void
g_hub_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (g_hub_parent_class)->finalize)
		G_OBJECT_CLASS (g_hub_parent_class)->finalize (object);
}

static void
g_hub_class_init (GHubClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GHubPrivate));

	object_class->dispose = g_hub_dispose;
	object_class->finalize = g_hub_finalize;

	/* Create the class: insert signals, properties, etc ... */
    g_hub_signals[MODULE_INIT] = g_signal_new ("module-init",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_init),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    g_hub_signals[MODULE_FINI] = g_signal_new ("module-fini",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_fini),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* Load / unload */
    g_hub_signals[MODULE_LOAD] = g_signal_new ("module-load",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_load),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
    g_hub_signals[MODULE_UNLOAD] = g_signal_new ("module-unload",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_unload),
        NULL, NULL,
        g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE,
        1,
		G_TYPE_POINTER);
	/* ref / unref */
    g_hub_signals[MODULE_REF] = g_signal_new ("module-ref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_ref),
        NULL, NULL,
        ghub_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
    g_hub_signals[MODULE_UNREF] = g_signal_new ("module-unref",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GHubClass, module_unref),
        NULL, NULL,
        ghub_marshal_VOID__POINTER_UINT,
        G_TYPE_NONE,
        2,
		G_TYPE_POINTER,
		G_TYPE_UINT);
}

static void
g_hub_init (GHub *self)
{
	GHubPrivate *priv = GET_PRIVATE(self);

    self->opt_cntxt = g_option_context_new(NULL);
	priv->dispose_func = NULL;
	priv->dispose_data = NULL;

	priv->slave_table = g_hash_table_new(g_str_hash, g_str_equal);
}

GHub*
g_hub_new (gint *argc, gchar **argv[])
{
	GHub *self = g_object_new (G_TYPE_HUB, NULL);
	GHubPrivate *priv = GET_PRIVATE(self);

	priv->argc = argc;
	priv->argv = argv;

	return self;
}

void
g_hub_set_dispose_callback(GHub *self, GHubCallback callback, gpointer user_data)
{
	GHubPrivate *priv = GET_PRIVATE(self);
	priv->dispose_func = callback;
	priv->dispose_data = user_data;
}

GHubHost *g_hub_host_get(GHub *self, gchar *name) {
	return g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
}

gboolean g_hub_load  (GHub *self, gchar *name) {
	GHubPrivate *priv = GET_PRIVATE(self);
	GHubHost  *host;
	GList     *plugin_list;
	gchar     *plugin_file;
	gchar     *symbol_name;

	gint i;
	gchar  *f;

	/* Check if the module is already loaded,
	 * If its loaded increment is refcount */
	host = g_hub_host_get(self, name);
	if (host != NULL) {
		host->refs++;
		g_signal_emit(G_OBJECT(self), g_hub_signals[MODULE_REF], 0, name, host->refs);
		return TRUE;
	}

	/* Build a list of posible files containing the code */
	plugin_file = g_strconcat("lib", name, ".so", NULL);
	plugin_list = gel_app_resource_get_list(GEL_APP_RESOURCE_LIB, plugin_file);
	g_free(plugin_file);

	/* Add a special case for open argv[0] */
	plugin_list = g_list_append(plugin_list, NULL); // Special case for open argv[0]

	/* Now lookup for the symbol */
	host        = g_new0(GHubHost, 1);
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
		if (((GHubSlaveInitFunc) host->slave->init_func) (self, priv->argc, priv->argv)) {
			/* If init funcion returns TRUE, insert plugin into GHub */
			g_signal_emit(G_OBJECT(self), g_hub_signals[MODULE_LOAD], 0, name);
			host->refs++;

			//g_signal_emit(G_OBJECT(self), g_hub_signals[MODULE_REF], 0, name, host->refs);
			gel_glist_free(plugin_list, (GFunc) g_free, NULL);
			return TRUE;
		} else {
			/* Init func failed: remove the mem and close module */
			g_hash_table_remove(priv->slave_table, name);
			g_module_close(host->module);
			continue;
		}
	}

	gel_glist_free(plugin_list, (GFunc) g_free, NULL);
	g_free(host);
	gel_warn("Cannot load %s", name);
	return FALSE;
}

gboolean g_hub_unload(GHub *self, gchar *name) {
	GHubPrivate *priv = GET_PRIVATE(self);
	GHubHost *host;

	host = g_hub_host_get(self, name);
	if ( host == NULL )
		return FALSE;

	host->refs--;
	g_signal_emit(G_OBJECT(self), g_hub_signals[MODULE_UNREF], 0, name, host->refs);
	if (host->refs > 0) {
		return TRUE;
	}
	
	if ( host->slave->exit_func != NULL ) {
		((GHubSlaveExitFunc) host->slave->exit_func)
			(host->data);
	}

	g_module_close(host->module);
	g_hash_table_remove(priv->slave_table, name);
	g_free(host);

	g_signal_emit(G_OBJECT(self), g_hub_signals[MODULE_UNLOAD], 0, name);

	return TRUE;
}

gboolean g_hub_loaded(GHub *self, gchar *name) {
	if (g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name) != NULL)
		return TRUE;
	else
		return FALSE;
}

gpointer g_hub_shared_register(GHub *self, gchar *name, guint size) {
	GHubHost *host;

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

void g_hub_shared_unregister(GHub *self, gchar *name) {
	GHubHost *host;

	host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
	if ( host == NULL ) {
		gel_warn("Attempt to unregister a non register memory: '%s'", name);
		return;
	}

	g_free(host->data);
}

gpointer g_hub_shared_get(GHub *self, gchar *name) {
	GHubHost *host;

	host = g_hash_table_lookup(GET_PRIVATE(self)->slave_table, name);
	return host ? host->data : NULL;
}

gboolean g_hub_shared_set(GHub *self, gchar *name, gpointer mem) {
	GHubHost *host;

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

