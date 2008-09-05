#define GEL_DOMAIN "Eina::IFace"

#include <gmodule.h>
#include <gel/gel.h>
#include "base.h"
#include "iface.h"
#include "player.h"

struct _EinaIFace {
	EinaBase     parent;
	EinaPlayer  *player;
	GtkNotebook *dock;
	GHashTable  *dock_items;
	GList       *plugins;
};

struct EinaIFaceSignalPack {
	gchar *signal;
	gpointer callback;
};

typedef struct _EinaPluginPrivate
{
	gchar      *pathname;
	GModule    *mod;
	EinaIFace  *iface;
	LomoPlayer *lomo;

} _EinaPluginPrivate;

void
eina_iface_dock_init(EinaIFace *self);

/*
 * Callbacks
 */

// Watch for load of player in case it's not already loaded before us
void
on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data);

// LomoPlayer signals
void
on_eina_iface_lomo_play(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_pause(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_stop(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_seek(LomoPlayer *lomo, gint64 old_pos, gint64 new_pos, gpointer data);
void
on_eina_iface_lomo_mute(LomoPlayer *lomo, gboolean mute, gpointer data);
void
on_eina_iface_lomo_add(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);
void
on_eina_iface_lomo_del(LomoPlayer *lomo, gint pos, gpointer data);
void
on_eina_iface_lomo_change(LomoPlayer *lomo, gint from, gint to, gpointer data);
void
on_eina_iface_lomo_clear(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_random(LomoPlayer *lomo, gboolean val, gpointer data);
void
on_eina_iface_lomo_repeat(LomoPlayer *lomo, gboolean val, gpointer data);
void
on_eina_iface_lomo_eos(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_error(LomoPlayer *lomo, GError *err, gpointer data);
void
on_eina_iface_lomo_tag(LomoPlayer *lomo, LomoStream *stream, LomoTag tag, gpointer data);
void
on_eina_iface_lomo_all_tags(LomoPlayer *lomo, LomoStream *stream, gpointer data);

G_MODULE_EXPORT gboolean eina_iface_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaIFace *self;
	gint i;
	struct EinaIFaceSignalPack signals[] = {
		{ "play",     on_eina_iface_lomo_play     },
		{ "pause",    on_eina_iface_lomo_pause    },
		{ "stop",     on_eina_iface_lomo_stop     },
		{ "seek",     on_eina_iface_lomo_seek     },
		{ "mute",     on_eina_iface_lomo_mute     },
		{ "add",      on_eina_iface_lomo_add      },
		{ "del",      on_eina_iface_lomo_del      },
		{ "change",   on_eina_iface_lomo_change   },
		{ "clear",    on_eina_iface_lomo_clear    },
		{ "random",   on_eina_iface_lomo_random   },
		{ "repeat",   on_eina_iface_lomo_repeat   },
		{ "eos",      on_eina_iface_lomo_eos      },
		{ "error",    on_eina_iface_lomo_error    },
		{ "tag",      on_eina_iface_lomo_tag      },
		{ "all-tags", on_eina_iface_lomo_all_tags },
		{ NULL, NULL }
	};

	// Create mem in hub
	self = g_new0(EinaIFace, 1);
	if (!eina_base_init((EinaBase *) self, hub, "iface", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	// Connect all signals of LomoPlayer
	for (i = 0; signals[i].signal != NULL; i++)
	{
		g_signal_connect(LOMO(self), signals[i].signal,
			(GCallback) signals[i].callback, self);
	}

	if ((self->player = gel_hub_shared_get(HUB(self), "player")) == NULL)
	{
		gel_warn("Player is not loaded, schuld. dock setup");
		g_signal_connect(hub, "module-load", G_CALLBACK(on_eina_iface_hub_load), self);
	}
	else
		eina_iface_dock_init(self);


	return TRUE;
}

G_MODULE_EXPORT gboolean eina_iface_exit
(gpointer data)
{
	EinaIFace *self = (EinaIFace *) data;

	eina_base_fini((EinaBase *) self);
	return TRUE;
}

GelHub *eina_iface_get_hub(EinaIFace *self)
{
	return HUB(self);
}

GList *
eina_iface_get_plugin_paths(void)
{
	GList *ret = NULL;
	gchar *tmp;
	gchar **tmp2;
	gint i;

	// Add system dir
	ret = g_list_prepend(ret, g_strdup(PACKAGE_LIB_DIR));

	// Add user dir
	tmp = (gchar *) g_getenv ("HOME");
	if (!tmp)
		tmp = (gchar *) g_get_home_dir();
	if (tmp)
		ret = g_list_prepend(ret, g_build_filename(tmp, ".eina", "plugins", NULL));

	// Add from $EINA_PLUGINS_PATH
	tmp = (gchar *) g_getenv("EINA_PLUGINS_PATH");
	if (tmp != NULL)
	{
		tmp2 = g_strsplit(tmp, ":", 0);
		for (i = 0; tmp2[i] != NULL; i++) {
			if (tmp2[i][0] == '\0')
				continue;
			ret = g_list_prepend(ret, g_strdup(tmp2[i]));
		}
		g_strfreev(tmp2);
	}

	return ret;
}

gboolean eina_iface_load_plugin(EinaIFace *self, gchar *plugin_name)
{
	GList *plugin_paths = NULL, *l = NULL;
	gchar *plugin_filename = NULL;
	GModule *mod = NULL;
	gchar *plugin_symbol = NULL; // Name of the symbol to lookup
	gpointer plugin_init = NULL; // The symbol it self
	EinaPlugin *plugin = NULL;   // EinaPlugin struct returned by plugin

	plugin_paths = eina_iface_get_plugin_paths();

	plugin_symbol = g_strdup_printf("%s_init", plugin_name);
	l = plugin_paths;
	while (l) {

		gchar *plugin_dirname = g_build_filename((gchar *) l->data, plugin_name, NULL);
		plugin_filename = g_module_build_path(plugin_dirname, plugin_name);
		g_free(plugin_dirname);
		eina_iface_debug("Testing %s", plugin_filename);

		if ((mod = g_module_open(plugin_filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)) == NULL)
		{
			g_free(plugin_filename);
			l = l->next;
			continue;
		}

		if (!g_module_symbol(mod, plugin_symbol, &plugin_init))
		{
			// Not found
			gel_warn("Found matching plugin for '%s' at '%s' but has no symbol '%s'",
				plugin_name,
				plugin_filename,
				plugin_symbol);
			g_module_close(mod);
		}
		else
		{
			// Found
			break;
		}

		g_free(plugin_filename);
		break;

		l = l->next;
	}
	g_free(plugin_symbol);

	if (plugin_init != NULL) {
		plugin = ((EinaPluginInitFunc)plugin_init)(HUB(self), self);
		if (plugin != NULL)
		{
			// Fill EinaPluginPrivate
			gel_info("Loading plugin %s for %s", plugin_filename, plugin_name);
			plugin->priv->pathname = plugin_filename;
			plugin->priv->mod      = mod;
			plugin->priv->iface    = self;
			plugin->priv->lomo     = gel_hub_shared_get(GEL_HUB(HUB(self)), "lomo");

			self->plugins = g_list_append(self->plugins, plugin);

			if (plugin->dock_widget != NULL)
				eina_iface_dock_add(self, (gchar *) plugin->name, plugin->dock_widget, plugin->dock_label_widget);
		}
	}

	g_list_foreach(plugin_paths, (GFunc) g_free, NULL);
	g_list_free(plugin_paths);

	return FALSE;
}

gchar *
eina_iface_build_plugin_filename(gchar *plugin_name, gchar *filename)
{
	return  g_build_filename(g_get_home_dir(), ".eina", "plugins", plugin_name, filename, NULL);
}

gchar *
eina_iface_get_plugin_dir(gchar *plugin_name)
{
	return  g_build_filename(g_get_home_dir(), ".eina", "plugins", plugin_name, NULL);
}

gchar *
eina_iface_plugin_resource_get_pathname(EinaPlugin *plugin, gchar *resource)
{
	gchar *ret;
	GList *plugin_paths, *iter;

	gboolean found = FALSE;
	gchar *plugin_dirname = NULL;

	if (plugin == NULL) 
	{
		gel_error("Invalid plugin");
		return NULL;
	}

	if ((plugin->priv == NULL) || (plugin->priv->pathname == NULL))
	{
		gel_warn("Oops, plugin is not initialize yet. Using heuristic mode to guess resource pathname");
		iter = plugin_paths = eina_iface_get_plugin_paths();
		while (iter && !found)
		{
			ret = g_build_filename(iter->data, plugin->name, resource, NULL);
			if (g_file_test(ret, G_FILE_TEST_IS_REGULAR))
			{
				found = TRUE;
				break;
			}
			g_free(ret);

			iter = iter->next;
		}
		gel_glist_free(plugin_paths, (GFunc) g_free, NULL);

		if (found)
		{
			gel_info("Found resource via heuristic method at '%s'", ret);
			return ret;
		}
		else
		{
			gel_error("Heuristic mode: epic fail");
			return NULL;
		}
	}

	else
	{
		plugin_dirname = g_path_get_dirname(plugin->priv->pathname);
		if (plugin_dirname == NULL)
			return NULL;

		ret = g_build_filename(plugin_dirname, resource, NULL);
		gel_info("RET: '%s'", ret);
		g_free(plugin_dirname);
		return ret;
	}
}

void eina_iface_dock_init(EinaIFace *self)
{
	gint i;

	if (!self->player || self->dock)
		return;

	if ((self->dock = GTK_NOTEBOOK(W(self->player, "dock"))) == NULL)
	{
		gel_warn("Cannot get dock, disabled");
	}
	gtk_widget_realize(GTK_WIDGET(self->dock));

	for (i = 0; i < gtk_notebook_get_n_pages(self->dock); i++)
		gtk_notebook_remove_page(self->dock, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	gtk_notebook_set_show_tabs(self->dock, FALSE);

	gel_info("Dock initialized");
}

gboolean eina_iface_dock_add(EinaIFace *self, gchar *id, GtkWidget *dock_widget, GtkWidget *label)
{
	if (!self->dock || (g_hash_table_lookup(self->dock_items, id) != NULL))
	{
		return FALSE;
	}

	if (gtk_notebook_append_page(self->dock, dock_widget, label) == -1)
	{
		gel_error("Cannot add widget to dock");
		return FALSE;
	}
	gel_info("Added dock '%s'", id);
	g_object_ref(G_OBJECT(dock_widget));
	g_hash_table_insert(self->dock_items, g_strdup(id), (gpointer) dock_widget);

	if (gtk_notebook_get_n_pages(self->dock) > 1)
		gtk_notebook_set_show_tabs(self->dock, TRUE);

	return TRUE;
}

gboolean eina_iface_dock_remove(EinaIFace *self, gchar *id)
{
	GtkWidget *dock_item;

	if (!self->dock || ((dock_item = g_hash_table_lookup(self->dock_items, id)) == NULL))
	{
		return FALSE;
	}

	g_object_unref(G_OBJECT(dock_item));
	gtk_notebook_remove_page(GTK_NOTEBOOK(self->dock),
		gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item));

	if (gtk_notebook_get_n_pages(self->dock) <= 1)
		gtk_notebook_set_show_tabs(self->dock, FALSE);

	return g_hash_table_remove(self->dock_items, id);
}

gboolean
eina_iface_dock_switch(EinaIFace *self, gchar *id)
{
	gint page_num;
	GtkWidget *dock_item;

	dock_item = g_hash_table_lookup(self->dock_items, (gpointer) id);
	if (dock_item == NULL)
	{
		gel_error("Cannot find dock widget '%s'", id);
		return FALSE;
	}

	page_num = gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item);
	if (page_num == -1)
	{
		gel_error("Dock item %s is not in dock", id);
		return FALSE;
	}

	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->dock), page_num);
	return TRUE;
}


/*
 * EinaPlugin functions
 */
// XXX Check const
#if 0
EinaPluginIFace *
eina_plugin_iface_new(gchar *plugin_filename, GModule *mod, EinaPlugin *plugin)
{
	EinaPluginIFace *ret = g_new0(EinaPluginIFace, 1);
	ret->name = plugin->name;
	ret->path = plugin_filename;
	ret->mod  = mod;
	ret->data = plugin;

	return ret;
}
#endif

EinaPlugin*
eina_plugin_new(EinaIFace *iface,
	const gchar *name, const gchar *provides, gpointer data, EinaPluginExitFunc fini,
    GtkWidget *dock_widget, GtkWidget *dock_label, GtkWidget *conf_widget)
{
	EinaPlugin *self = g_new0(EinaPlugin, 1);
	self->priv = g_new0(EinaPluginPrivate, 1);

	self->name = name;
	self->provides = provides;
	self->data = data;
	self->fini = fini;
	self->dock_widget = dock_widget;
	self->dock_label_widget = dock_label;
	self->conf_widget = conf_widget;

	return self;
}

void
eina_plugin_free(EinaPlugin *self)
{
	g_free(self->priv->pathname);
	g_free(self->priv);
	g_free(self);
}

/*
 * Signals implementation
 */

LomoPlayer *
eina_plugin_get_lomo_player(EinaPlugin *plugin)
{
	return plugin->priv->lomo;
}

EinaIFace *
eina_plugin_get_iface(EinaPlugin *plugin)
{
	return plugin->priv->iface;
}

void on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data)
{
	EinaIFace  *self = (EinaIFace *) data;

	if (!g_str_equal(modname, "player"))
		return;

	if ((self->player = gel_hub_shared_get(hub, "player")) == NULL)
		return;

	eina_iface_dock_init(self);
}

#define magic_arg (((EinaPlugin *) iter->data))
#define magic(field, ...) \
	do { \
		GList *iter = ((EinaIFace *) data)->plugins; \
		void (*callback)(LomoPlayer *lomo, ...);     \
		while (iter) { \
			callback = ((EinaPlugin *) iter->data)->field; \
			if (callback) \
				callback(lomo, __VA_ARGS__); \
			iter = iter->next; \
		} \
	} while(0)

void
on_eina_iface_lomo_play(LomoPlayer *lomo, gpointer data)
{
	magic(play, magic_arg);
}

void
on_eina_iface_lomo_pause(LomoPlayer *lomo, gpointer data)
{
	magic(pause, magic_arg);
}

void
on_eina_iface_lomo_stop(LomoPlayer *lomo, gpointer data)
{
	magic(stop, magic_arg);
}

void
on_eina_iface_lomo_seek(LomoPlayer *lomo, gint64 old_pos, gint64 new_pos, gpointer data)
{
	magic(seek, old_pos, new_pos, magic_arg);
}

void
on_eina_iface_lomo_mute(LomoPlayer *lomo, gboolean mute, gpointer data)
{
	magic(mute, mute, magic_arg);
}

void
on_eina_iface_lomo_add(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data)
{
	magic(add, stream, pos, magic_arg);
}

void
on_eina_iface_lomo_del(LomoPlayer *lomo, gint pos, gpointer data)
{
	magic(del, pos, magic_arg);
}

void
on_eina_iface_lomo_change(LomoPlayer *lomo, gint from, gint to, gpointer data)
{
	// magic(change, from, to, magic_arg);
	magic(change, from, to, magic_arg, magic_arg);
}

void
on_eina_iface_lomo_clear(LomoPlayer *lomo, gpointer data)
{
	magic(clear, magic_arg);
}

void
on_eina_iface_lomo_random(LomoPlayer *lomo, gboolean val, gpointer data)
{
	magic(random, val, magic_arg);
}

void
on_eina_iface_lomo_repeat(LomoPlayer *lomo, gboolean val, gpointer data)
{
	magic(repeat, val, magic_arg);
}

void
on_eina_iface_lomo_eos(LomoPlayer *lomo, gpointer data)
{
	magic(eos, magic_arg);
}

void
on_eina_iface_lomo_error(LomoPlayer *lomo, GError *err, gpointer data)
{
	magic(error, err, magic_arg);
}

void
on_eina_iface_lomo_tag(LomoPlayer *lomo, LomoStream *stream, LomoTag tag, gpointer data)
{
	magic(tag, stream, tag, magic_arg);
}

void
on_eina_iface_lomo_all_tags(LomoPlayer *lomo, LomoStream *stream, gpointer data)
{
	magic(all_tags, stream, magic_arg);
}

G_MODULE_EXPORT GelHubSlave iface_connector = {
	"iface",
	&eina_iface_init,
	&eina_iface_exit
};

