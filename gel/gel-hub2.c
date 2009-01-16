/*
 * gel/gel-hub2.c
 *
 * Copyright (C) 2004-2009 Eina
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

#include <gmodule.h>
#include <gel/gel-hub2.h>
#include <gel/gel-hub-marshal.h>

G_DEFINE_TYPE (GelHub, gel_hub, GEL_TYPE_HUB)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_HUB, GelHubPrivate))

// --
// Signals
// --
enum {
	MODULE_INIT,
	MODULE_FINI,
	MODULE_LOAD,
	MODULE_UNLOAD,
	MODULE_REF,
	MODULE_UNREF,

	LAST_SIGNAL
};
static guint gel_hub_signals[LAST_SIGNAL] = { 0 };

// --
// GelHub type
// --
typedef struct _GelHubPrivate GelHubPrivate;
struct _GelHubPrivate {
	GList      *paths;   // values: gchar*
	GList      *order;   // values: GelHubModule object
	GHashTable *modules; // key:pathname value:GelHubModule object
};

// --
// GelHubModule type
// --
enum {
	GEL_HUB_MODULE_CANNOT_OPEN_SHARED_OBJECT,
	GEL_HUB_MODULE_SYMBOL_NOT_FOUND
};

struct _GelHubModulePrivate {
	guint     refs;
	GModule  *mod;
	gchar    *pathname;
	gboolean  enabled;
};

static GQuark
gel_hub_module_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("Gel::HubModule");
	return ret;
}

GelHubModule*
gel_hub_module_new(gchar *pathname, gchar *symbol, GError **error)
{
	GModule *mod;
	gpointer symbol_p;

	if ((mod = g_module_open(pathname, G_MODULE_BIND_LAZY)) == NULL)
	{
		g_set_error(error, gel_hub_module_quark(), GEL_HUB_MODULE_CANNOT_OPEN_SHARED_OBJECT,
			"Cannot open shared object %s", pathname);
		return NULL;
	}

	if (!g_module_symbol(mod, symbol, &symbol_p))
	{
		g_set_error(error, gel_hub_module_quark(), GEL_HUB_MODULE_SYMBOL_NOT_FOUND,
			"Cannot find symbol %s in %s", symbol, pathname);
		g_module_close(mod);
		return NULL;
	}

	GelHubModule *self = (GelHubModule*) symbol_p;
	self->priv->refs     = 1;
	self->priv->mod      = mod;
	self->priv->enabled  = FALSE;
	self->priv->pathname = g_strdup(pathname);

	return self;
}

void
gel_hub_module_destroy(GelHubModule *self)
{
	GModule *mod = self->priv->mod;
	g_free(self->priv->pathname);
	g_free(self->priv);
	g_module_close(mod);
}

void
gel_hub_module_ref(GelHubModule *self)
{
	self->priv->refs++;
}

void
gel_hub_module_unref(GelHubModule *self)
{
	self->priv->refs--;
	if (self->priv->refs <= 0)
		gel_hub_module_destroy(self);
}

// --
// Implementation
// --
static GList*
rebuild_paths(GelHub *self)
{
	GelHubPrivate *priv = GET_PRIVATE(self);

	// Clear internal paths
	g_list_foreach(priv->paths, (GFunc) g_free, NULL);
	g_list_free(priv->paths);
	priv->paths = NULL;

#ifdef PACKAGE_LIB_DIR
	// Add system dir
	priv->paths = g_list_prepend(priv->paths, g_strdup(PACKAGE_LIB_DIR));
#endif

#ifdef PACKAGE_NAME
	// Add user dir
	gchar *home = (gchar *) g_getenv ("HOME");
	if (!home)
		home = (gchar *) g_get_home_dir();
	if (home)
	{
		gchar *dotdir = g_strconcat(".", PACKAGE_NAME, NULL);
		priv->paths = g_list_prepend(priv->paths, g_build_filename(home, dotdir, "plugins", NULL));
		g_free(dotdir);
	}
#endif

#ifdef PACKAGE_NAME
	// Add from ${PACKAGE_NAME.strup()}_PLUGINS_PATH
	gchar *uc_package_name = g_utf8_strup(PACKAGE_NAME, -1);
	gchar *envvar = g_strconcat(uc_package_name, "_PLUGINS_PATH", NULL);
	g_free(uc_package_name);

	gchar *plugins_path (gchar *) g_getenv(envvar);
	g_free(envvar);

	if (plugins_path)
	{
		gchar *tmp = g_strsplit(plugins_path, ":", 0);
		for (i = 0; tmp[i] != NULL; i++) {
			if (tmp[i][0] == '\0')
				continue;
			priv->paths = g_list_prepend(priv->paths, g_strdup(tmp[i]));
		}
		g_strfreev(tmp);
	}
#endif

	return priv->paths;
}

static void
gel_hub_dispose (GObject *object)
{
	G_OBJECT_CLASS (gel_hub_parent_class)->dispose (object);
}

static void
gel_hub_finalize (GObject *object)
{
	G_OBJECT_CLASS (gel_hub_parent_class)->finalize (object);
}

static void
gel_hub_class_init (GelHubClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelHubPrivate));

	object_class->dispose = gel_hub_dispose;
	object_class->finalize = gel_hub_finalize;

	// --
	// Signals
	// --

	// Init/fini
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

	// Load/unload
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

	// ref/unref
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
}

GelHub*
gel_hub_new (void)
{
	GelHub *self = g_object_new (GEL_TYPE_HUB, NULL);
	GelHubPrivate *priv = GET_PRIVATE(self);

	priv->modules = g_hash_table_new(g_str_hash, g_str_equal);
	
	rebuild_paths(self);
	return self;
}

gboolean
gel_hub_module_load(GelHub *self, const gchar *name)
{
	GelHubPrivate *priv = GET_PRIVATE(self);

	GelHubModule *module = NULL;
	GList *iter = priv->paths;
	while (iter)
	{
		gchar *dirname = g_build_filename(iter->data, name, NULL);
		gchar *module_pathname = g_module_build_path(dirname, name);
		g_free(dirname);

		// Check if is module is already loaded
		GelHubModule *module;
		if ((module = gel_hub_lookup_module_by_path(self, module_pathname)) != NULL)
		{
			// Add ref
			g_free(module_pathname);
			gel_hub_module_ref(module);
			break;
		}

		else
		{
			// Calculate symbol and try to loada
			gchar *symbol = g_strconcat(name, "_plugin", NULL);

			module = gel_hub_module_new(module_pathname, symbol, NULL);

			g_free(module_pathname);
			g_free(symbol);

			if (module)
				break;
		}
		iter = iter->next;
	}

	if (module)
	{
		// g_list_append(priv->modules, module)
		return TRUE;
	}
	else
		return FALSE;
}

GelHubModule *
gel_hub_lookup_module_by_path(GelHub *hub, const gchar *pathname)
{
	GList *iter = modules;
	while (iter)
	{
		if (g_str_equal(gel_hub_module_get_pathname(GEL_HUB_MODULE(iter->data)), pathname))
			break;
		iter = iter->next;
	}
	g_list_free(modules);

	if (iter && iter->data)
		return iter->data;
	else
		return NULL;
}
