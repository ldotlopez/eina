/*
 * eina/ext/eina-window.c
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

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gel/gel.h>
#include <eina/ext/eina-window.h>

G_DEFINE_TYPE (EinaWindow, eina_window, GTK_TYPE_WINDOW)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_WINDOW, EinaWindowPrivate))

typedef struct _EinaWindowPrivate EinaWindowPrivate;

typedef struct {
	EinaWindowKeyBind bind;
	gpointer          data;
} EinaWindowKeyBindPriv;

struct _EinaWindowPrivate {
	gboolean        persistant;
	GtkBox         *container;
	GtkUIManager   *ui_manager;
	GtkActionGroup *ag;
	GList          *keybinds;
	gboolean        keybindings_enabled;
};

gboolean
window_key_press_event_cb(GtkWidget *self, GdkEvent *ev, gpointer data);

static gchar *ui_mng_str =
"<ui>"
"  <menubar name='Main' >"
"    <menu name='File'    action='FileMenu'    />"
"    <menu name='Edit'    action='EditMenu'    />"
"    <menu name='Plugins' action='PluginsMenu' />"
"    <menu name='Help'    action='HelpMenu'    />"
"  </menubar>"
"</ui>";

static GtkActionEntry ui_mng_actions[] = {
	{ "FileMenu",    NULL, N_("_File"),    "<alt>f", NULL, NULL},
	{ "EditMenu",    NULL, N_("_Edit"),    "<alt>e", NULL, NULL},
	{ "PluginsMenu", NULL, N_("_Add-ons"), "<alt>a", NULL, NULL},
	{ "HelpMenu",    NULL, N_("_Help"),    "<alt>h", NULL, NULL},
};

static void
eina_window_dispose (GObject *object)
{
	EinaWindow *self = (EinaWindow *) object;
	EinaWindowPrivate *priv = GET_PRIVATE(self);

	if (priv->keybinds)
	{
		gel_list_deep_free(priv->keybinds, g_free);
		priv->keybinds = NULL;
	}

	if (priv->ag)
	{
		g_object_unref(priv->ag);
		priv->ag = NULL;
	}

	G_OBJECT_CLASS (eina_window_parent_class)->dispose (object);
}

static void
eina_window_class_init (EinaWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaWindowPrivate));

	object_class->dispose = eina_window_dispose;
}

static void
eina_window_init (EinaWindow *self)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);

	priv->persistant = FALSE;
	priv->container = (GtkBox *) gtk_vbox_new(FALSE, 0);
	priv->ui_manager = gtk_ui_manager_new();

	gtk_ui_manager_add_ui_from_string(priv->ui_manager, ui_mng_str, -1, NULL);

	priv->ag = gtk_action_group_new("_window");
	gtk_action_group_add_actions(priv->ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), NULL);
	gtk_ui_manager_insert_action_group(priv->ui_manager, priv->ag, 0);
	gtk_ui_manager_ensure_update(priv->ui_manager);

	gtk_box_pack_start(priv->container,
		gtk_ui_manager_get_widget(priv->ui_manager, "/Main"),
		FALSE, TRUE, 0
		);
	gtk_widget_show_all((GtkWidget *) priv->container);
	gtk_container_add((GtkContainer *) self, (GtkWidget *) priv->container);

	gtk_window_add_accel_group((GtkWindow *) self, gtk_ui_manager_get_accel_group(priv->ui_manager));
	g_signal_connect(self, "key-press-event", (GCallback) window_key_press_event_cb, NULL);

    gchar *icon_path = NULL;
	GdkPixbuf *icon_pb = NULL; 
	if ((icon_path = gel_resource_locate(GEL_RESOURCE_IMAGE, "eina.svg")) != NULL)
	{
		icon_pb = gdk_pixbuf_new_from_file_at_size(icon_path, 64, 64, NULL);
		gtk_window_set_default_icon(icon_pb);
		g_object_unref(icon_pb);
	}
	gel_free_and_invalidate(icon_path, NULL, g_free);			    
}

EinaWindow*
eina_window_new (void)
{
	EinaWindow *self = g_object_new (EINA_TYPE_WINDOW, NULL);
	return self;
}

void
eina_window_set_persistant(EinaWindow *self, gboolean persistant)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);
	if (priv->persistant == persistant)
		return;

	priv->persistant = persistant;
	if (persistant)
		g_signal_connect(self, "delete-event", (GCallback) gtk_widget_hide_on_delete, NULL);
	else
		g_signal_handlers_disconnect_by_func(self, gtk_widget_hide_on_delete, NULL);
}

gboolean
eina_window_get_persistant(EinaWindow *self)
{
	return GET_PRIVATE(self)->persistant;
}

gboolean
eina_window_key_bindings_get_enabled(EinaWindow *self)
{
	return GET_PRIVATE(self)->keybindings_enabled;
}

void
eina_window_key_bindings_set_enabled(EinaWindow *self, gboolean enabled)
{
	GET_PRIVATE(self)->keybindings_enabled = enabled;
}

void
eina_window_add_widget(EinaWindow *self, GtkWidget *child, gboolean expand, gboolean fill, gint spacing)
{
	gtk_box_pack_start(GET_PRIVATE(self)->container, child, expand, fill, spacing);
}

void
eina_window_remove_widget(EinaWindow *self, GtkWidget *child)
{
	gtk_container_remove((GtkContainer *) GET_PRIVATE(self)->container, child);
}

GtkUIManager *
eina_window_get_ui_manager(EinaWindow *self)
{
	return GET_PRIVATE(self)->ui_manager;
}

void
eina_window_register_key_bindings(EinaWindow *self, guint n, EinaWindowKeyBind bind[], gpointer data)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);

	guint i;
	GList *tmp = NULL;
	for (i = 0; i < n; i++)
	{
		EinaWindowKeyBindPriv *kbp = g_new0(EinaWindowKeyBindPriv, 1);
		kbp->bind = bind[i];
		kbp->data = data;
		tmp = g_list_prepend(tmp, (gpointer) kbp);
	}
	priv->keybinds = g_list_concat(priv->keybinds, g_list_reverse(tmp));
}

void
eina_window_unregister_key_bindings(EinaWindow *self, guint n, EinaWindowKeyBind bind[], gpointer data)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);

	gint i;
	for (i = 0; i < n; i++)
	{
		// Try to find a matching bind
		GList *iter = priv->keybinds;
		while (iter)
		{
			EinaWindowKeyBindPriv *kbp = (EinaWindowKeyBindPriv *) iter->data;

			if ((kbp->bind.keyval  == bind[i].keyval ) &&
				(kbp->bind.handler == bind[i].handler) &&
				(kbp->data         == data))
				break;
			iter = iter->next;
		}

		// If not found, warn and continue
		if (!iter)
		{
			g_warning("Bind %d,%p not found", bind[i].keyval, bind[i].handler);
			continue;
		}

		priv->keybinds = g_list_remove_link(priv->keybinds, iter);
		g_free(iter->data);
		g_list_free(iter);
	}
}

gboolean
window_key_press_event_cb(GtkWidget *self, GdkEvent *ev, gpointer data)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);
	if (!priv->keybindings_enabled)
		return FALSE;

	GList *iter = priv->keybinds;
	while (iter)
	{
		EinaWindowKeyBindPriv *kbp = (EinaWindowKeyBindPriv *) iter->data;
		if ((kbp->bind.keyval != GDK_VoidSymbol) && (kbp->bind.keyval != ev->key.keyval))
		{
			iter = iter->next;
			continue;
		}

		gboolean ret = kbp->bind.handler(ev, kbp->data);
		if (ret == TRUE)
			return ret;
		iter = iter->next;
	}
	return FALSE;
}
