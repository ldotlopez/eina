/*
 * eina/ntfy/ntfy.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-ntfy-plugin.h"
#include <libnotify/notify.h>
#include <gel/gel-ui.h>
#include <eina/lomo/eina-lomo-plugin.h>

/*
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_NTFY_PLUGIN         (eina_ntfy_plugin_get_type ())
#define EINA_NTFY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_NTFY_PLUGIN, EinaNtfyPlugin))
#define EINA_NTFY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_NTFY_PLUGIN, EinaNtfyPlugin))
#define EINA_IS_NTFY_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_NTFY_PLUGIN))
#define EINA_IS_NTFY_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_NTFY_PLUGIN))
#define EINA_NTFY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_NTFY_PLUGIN, EinaNtfyPluginClass))

typedef struct {
	GSettings       *settings;
	gboolean         enabled;

	NotifyNotification *ntfy;
	LomoStream         *stream;
} EinaNtfyPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_NTFY_PLUGIN, EinaNtfyPlugin, eina_ntfy_plugin)

static gboolean ntfy_enable (EinaNtfyPlugin *plugin, GError **error);
static void     ntfy_disable(EinaNtfyPlugin *plugin);
static void     ntfy_sync   (EinaNtfyPlugin *plugin);

static void stream_weak_ref_cb  (EinaNtfyPlugin *plugin, GObject *_stream);
static void stream_em_updated_cb(LomoStream *stream, const gchar *key, EinaNtfyPlugin *plugin);

static void lomo_change_cb      (LomoPlayer *lomo,   gint from,         gint to, EinaNtfyPlugin *plugin);
static void lomo_state_notify_cb(LomoPlayer *lomo,   GParamSpec *pspec,          EinaNtfyPlugin *plugin);
static void lomo_all_tags_cb    (LomoPlayer *lomo,   LomoStream *stream,         EinaNtfyPlugin *plugin);

static void settings_changed_cb (GSettings *settings, const gchar *key, EinaNtfyPlugin *plugin);

static gboolean
eina_ntfy_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaNtfyPlugin      *plugin = EINA_NTFY_PLUGIN(activatable);
	EinaNtfyPluginPrivate *priv = plugin->priv;

	priv->settings = eina_application_get_settings(app, EINA_NTFY_PREFERENCES_DOMAIN);

	gboolean ret = TRUE;
	if (g_settings_get_boolean(priv->settings, EINA_NTFY_ENABLED_KEY))
		if (!ntfy_enable(plugin, error))
			ret = FALSE;

	g_signal_connect(priv->settings, "changed", (GCallback) settings_changed_cb, plugin);

	return ret;
}

static gboolean
eina_ntfy_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	g_warn_if_fail(EINA_IS_NTFY_PLUGIN(activatable));
	if (EINA_IS_NTFY_PLUGIN(activatable))
		ntfy_disable((EinaNtfyPlugin *) activatable);

	return TRUE;
}

static gboolean
ntfy_enable(EinaNtfyPlugin *plugin, GError **error)
{
	g_return_val_if_fail(EINA_IS_NTFY_PLUGIN(plugin), FALSE);
	EinaNtfyPluginPrivate *priv = plugin->priv;

	if (priv->enabled)
		return TRUE;

	if (!notify_is_initted())
		notify_init(PACKAGE_NAME);
	priv->enabled = TRUE;

	#if defined(NOTIFY_VERSION_0_7)
	priv->ntfy = notify_notification_new(N_("Now playing"), NULL, NULL);
	#elif defined(NOTIFY_VERSION_0_5)
	priv->ntfy = notify_notification_new(N_("Now playing"), NULL, NULL, NULL);
	#else
	#error "Unknow notify version"
	#endif

	EinaApplication *app = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	LomoPlayer     *lomo = eina_application_get_lomo(app);

	g_signal_connect(lomo, "notify::state", (GCallback) lomo_state_notify_cb, plugin);
	g_signal_connect(lomo, "change",        (GCallback) lomo_change_cb,       plugin);
	g_signal_connect(lomo, "all-tags",      (GCallback) lomo_all_tags_cb,     plugin);

	return TRUE;
}

static void
ntfy_disable(EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	EinaNtfyPluginPrivate *priv = plugin->priv;

	if (!priv->enabled)
		return;

	priv->enabled = FALSE;

	EinaApplication *app = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	LomoPlayer     *lomo = eina_application_get_lomo(app);

	g_signal_handlers_disconnect_by_func(lomo, (GCallback) lomo_state_notify_cb, plugin);
	g_signal_handlers_disconnect_by_func(lomo, (GCallback) lomo_change_cb,       plugin);
	g_signal_handlers_disconnect_by_func(lomo, (GCallback) lomo_all_tags_cb,     plugin);

	if (priv->stream)
	{
		g_signal_handlers_disconnect_by_func(priv->stream, stream_em_updated_cb, plugin);
		priv->stream  = NULL;
	}

	gel_free_and_invalidate(priv->ntfy,  NULL, g_object_unref);

	if (notify_is_initted())
		notify_uninit();
}

static void
ntfy_sync(EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	EinaNtfyPluginPrivate *priv = plugin->priv;

	EinaApplication *app = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	LomoPlayer     *lomo = eina_application_get_lomo(app);
	LomoStream   *stream = lomo_player_get_current_stream(lomo);

	if (lomo_player_get_state(lomo) != LOMO_STATE_PLAY)
		return;

	if (priv->stream != stream)
	{
		if (priv->stream)
		{
			g_signal_handlers_disconnect_by_func(priv->stream, stream_em_updated_cb, plugin);
			g_object_weak_unref(G_OBJECT(priv->stream), (GWeakNotify) stream_weak_ref_cb, plugin);
		}
		priv->stream = stream;
		g_object_weak_ref(G_OBJECT(priv->stream), (GWeakNotify) stream_weak_ref_cb, plugin);
		g_signal_connect(priv->stream, "extended-metadata-updated", G_CALLBACK (stream_em_updated_cb), plugin);
	}

	GdkPixbuf *scaled = NULL;

	// Build body
	gchar *tmp = g_path_get_basename(lomo_stream_get_uri(stream));
	gchar *bname = g_uri_unescape_string(tmp, NULL);
	g_free(tmp);

	gchar *artist = lomo_stream_strdup_tag_value(stream, LOMO_TAG_ARTIST);
	gchar *title  = lomo_stream_strdup_tag_value(stream, LOMO_TAG_TITLE);
	gchar *body = g_strdup_printf("<b>%s</b>\n%s", title ? title : bname, artist ? artist : "");
	gel_str_free_and_invalidate(artist);
	gel_str_free_and_invalidate(title);
	gel_str_free_and_invalidate(bname);

	notify_notification_update(priv->ntfy, N_("Playing now"), body, NULL);

	GdkPixbuf *pb = gel_ui_pixbuf_from_value(lomo_stream_get_extended_metadata(stream, LOMO_STREAM_EM_ART_DATA));
	if (!pb || !GDK_IS_PIXBUF(pb))
	{
		g_warn_if_fail(pb && GDK_IS_PIXBUF(pb));
		goto ntfy_sync_show;
	}

	gdouble sx = (gdouble) 64 / gdk_pixbuf_get_width(pb);
	gdouble sy = (gdouble) 64 / gdk_pixbuf_get_height(pb);

	gint dx = MIN(sx,sy) * gdk_pixbuf_get_width(pb);
	gint dy = MIN(sx,sy) * gdk_pixbuf_get_height(pb);

	scaled = gdk_pixbuf_scale_simple(pb, dx, dy, GDK_INTERP_NEAREST);
	g_object_unref(pb);

	notify_notification_set_icon_from_pixbuf(priv->ntfy, scaled);

ntfy_sync_show:
	notify_notification_show(priv->ntfy, NULL);
	gel_free_and_invalidate(body,   NULL, g_free);
	gel_free_and_invalidate(scaled, NULL, g_object_unref);
}

static void
stream_weak_ref_cb(EinaNtfyPlugin *plugin, GObject *_stream)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	plugin->priv->stream = NULL;
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	EinaNtfyPluginPrivate *priv = EINA_NTFY_PLUGIN(plugin)->priv;

	if (priv->stream)
	{
		g_signal_handlers_disconnect_by_func(priv->stream, stream_em_updated_cb, plugin);
		priv->stream = NULL;
	}
	ntfy_sync(plugin);
}

static void
lomo_state_notify_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));

	if (!g_str_equal(pspec->name, "state"))
		return;
	ntfy_sync(plugin);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	if (lomo_player_get_current_stream(lomo) != stream)
		return;
	ntfy_sync(plugin);
}

static void
stream_em_updated_cb(LomoStream *stream, const gchar *key, EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	g_return_if_fail(LOMO_IS_STREAM(stream));
	g_return_if_fail(key);

	if (!g_str_equal(key, LOMO_STREAM_EM_ART_DATA))
		return;
	ntfy_sync(plugin);
}

static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaNtfyPlugin *plugin)
{
	g_return_if_fail(EINA_IS_NTFY_PLUGIN(plugin));
	g_return_if_fail(key != NULL);
	g_return_if_fail(G_IS_SETTINGS(settings));

	if (g_str_equal(key, EINA_NTFY_ENABLED_KEY))
	{
		if (g_settings_get_boolean(settings, key))
			ntfy_enable(plugin, NULL);
		else
			ntfy_disable(plugin);
		return;
	}

	g_warning(N_("Unhanded key '%s'"), key);
}

