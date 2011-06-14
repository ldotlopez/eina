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
#include <eina/lomo/eina-lomo-plugin.h>
#include <eina/art/eina-art-plugin.h>

#define HAVE_VOGON 0

#define SETTINGS_PATH "/ntfy"
#define ACTION_NAME   "Notifications"
#define ACTION_PATH   "/Main/PluginsPlaceholder/Notifications"

typedef struct {
	EinaApplication *app;
	GSettings       *settings;
	gboolean         enabled;

	#if HAVE_VOGON
	gboolean        vogon_enabled;
	GtkActionGroup *vogon_ag;
	guint           vogon_merge_id;
	#endif

	NotifyNotification *ntfy;
	GtkStatusIcon      *status_icon;
	LomoStream         *stream;
} EinaNtfyData;

// Mini API
static gboolean
ntfy_enable(EinaNtfyData *self, GError **error);
static void
ntfy_disable(EinaNtfyData *self);
#if HAVE_VOGON
static gboolean
vogon_enable(EinaNtfyData *self);
static gboolean
vogon_disable(EinaNtfyData *self);
#endif

static void
ntfy_sync(EinaNtfyData *self);

// Callback
#if HAVE_VOGON
static void
engine_plugin_init_cb(GelPluginEngine *engine, GelPlugin *plugin, EinaNtfyData *self);
static void
engine_plugin_fini_cb(GelPluginEngine *engine, GelPlugin *plugin, EinaNtfyData *self);
static void
action_activate_cb(GtkAction *action, EinaNtfyData *self);
#endif

static void
stream_weak_ref_cb(EinaNtfyData *self, GObject *_stream);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaNtfyData *self);
static void
lomo_state_notify_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaNtfyData *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfyData *self);
static void
stream_em_updated_cb(LomoStream *stream, const gchar *key, EinaNtfyData *self);
static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaNtfyData *self);

// ------------------
// Init / fini plugin
// ------------------
EINA_DEFINE_EXTENSION(EinaNtfyPlugin, eina_ntfy_plugin, EINA_TYPE_NTFY_PLUGIN)

static gboolean
eina_ntfy_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaNtfyData *self;

	self = g_new0(EinaNtfyData, 1);
	self->app = app;

	// Enable if needed (by default on)
	self->settings = eina_application_get_settings(app, EINA_NTFY_PREFERENCES_DOMAIN);
	if (g_settings_get_boolean(self->settings, EINA_NTFY_ENABLED_KEY))
	{
		if (!ntfy_enable(self, error))
		{
			eina_ntfy_plugin_deactivate(activatable, app, NULL);
			return FALSE;
		}
	}
	g_signal_connect(self->settings, "changed", (GCallback) settings_changed_cb, self);

	// Try to enable vogon integration
	#if HAVE_VOGON
	GelPlugin *vogon = gel_app_get_plugin_by_name(app, "vogon");
	if (vogon && gel_plugin_is_enabled(vogon))
		if (!vogon_enable(self))
			gel_warn("Cannot enable vogon integration");

	g_signal_connect(app, "plugin-init", (GCallback) engine_plugin_init_cb, self);
	g_signal_connect(app, "plugin-fini", (GCallback) engine_plugin_init_cb, self);
	#endif

	eina_activatable_set_data(activatable, self);
	return TRUE;
}

static gboolean
eina_ntfy_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaNtfyData *self = (EinaNtfyData *) eina_activatable_steal_data(activatable);

	#if HAVE_VOGON
	g_signal_handlers_disconnect_by_func(engine, engine_plugin_init_cb, self);
	g_signal_handlers_disconnect_by_func(engine, engine_plugin_fini_cb, self);

	// Try to disable vogon integration
	GelPlugin *vogon = gel_app_get_plugin_by_name(app, "vogon");
	if (vogon && gel_plugin_is_enabled(vogon))
		if (!vogon_disable(self))
			gel_warn("Cannot disable vogon integration");
	#endif

	ntfy_disable(self);
	g_free(self);

	return TRUE;
}

static gboolean
ntfy_enable(EinaNtfyData *self, GError **error)
{
	if (self->enabled)
		return TRUE;
	
	if (!notify_is_initted())
		notify_init(PACKAGE_NAME);

	self->enabled = TRUE;
	#if defined(NOTIFY_VERSION_0_7)
	self->ntfy = notify_notification_new(N_("Now playing"), NULL, NULL);
	#elif defined(NOTIFY_VERSION_0_5)
	self->ntfy = notify_notification_new(N_("Now playing"), NULL, NULL, NULL);
	#else
	#error "Unknow notify version"
	#endif

	LomoPlayer *lomo = eina_application_get_lomo(self->app);
	g_signal_connect(lomo, "notify::state", (GCallback) lomo_state_notify_cb, self);
	g_signal_connect(lomo, "change",   (GCallback) lomo_change_cb, self);
	g_signal_connect(lomo, "all-tags", (GCallback) lomo_all_tags_cb, self);

	return TRUE;
}

static void
ntfy_disable(EinaNtfyData *self)
{
	if (!self->enabled)
		return;
	self->enabled = FALSE;

	LomoPlayer *lomo = eina_application_get_lomo(self->app);
	g_signal_handlers_disconnect_by_func(lomo, (GCallback) lomo_state_notify_cb, self);
	g_signal_handlers_disconnect_by_func(lomo, (GCallback) lomo_all_tags_cb, self);

	if (self->stream)
	{
		g_signal_handlers_disconnect_by_func(self->stream, stream_em_updated_cb, self);
		self->stream  = NULL;
	}

	gel_free_and_invalidate(self->ntfy,  NULL, g_object_unref);

	if (notify_is_initted())
		notify_uninit();
}

#if HAVE_VOGON
static gboolean
vogon_enable(EinaNtfyData *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	g_return_val_if_fail(vogon != NULL, FALSE);

	if (self->vogon_enabled)
		return TRUE;
	self->vogon_enabled = TRUE;

	// Get status icon
	self->status_icon = eina_vogon_get_status_icon(vogon);

	// Gel UI Manager
	GtkUIManager *ui_mng =  eina_vogon_get_ui_manager(vogon);
	if (!ui_mng)
	{
		vogon_disable(self);
		return FALSE;
	}

	// Add actions
	GtkToggleActionEntry ui_actions[] =
	{
		{ "Notifications", NULL, N_("Show notifications"), NULL, N_("Show notifications"), G_CALLBACK(action_activate_cb) }
	};
	self->vogon_ag = gtk_action_group_new("notify");
	gtk_action_group_add_toggle_actions(self->vogon_ag, ui_actions, G_N_ELEMENTS(ui_actions), self);

	// Merge UI
	const gchar *ui_xml = 
	"<ui>"
	"	<popup name='Main'>"
	"		<placeholder name='PluginsPlaceholder' >"
	"			<separator />"
	"			<menuitem action='Notifications' />"
	"			<separator />"
	"		</placeholder>"
	"	</popup>"
	"</ui>";

	GError *err = NULL;
	if ((self->vogon_merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_xml, -1, &err)) == 0)
	{
		gel_error("Cannot merge menu: %s", err->message);
		g_error_free(err);
		vogon_disable(self);
		return FALSE;
	}

	// Add action group
	gtk_ui_manager_insert_action_group(ui_mng, self->vogon_ag, 0);
	gtk_ui_manager_ensure_update(ui_mng);

	// Set state
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_ui_manager_get_action(ui_mng, ACTION_PATH)), self->enabled);
	notify_notification_attach_to_status_icon(self->ntfy, self->status_icon);

	return TRUE;
}

static gboolean
vogon_disable(EinaNtfyData *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	g_return_val_if_fail(vogon != NULL, FALSE);

	notify_notification_attach_to_status_icon(self->ntfy, NULL);

	GtkUIManager *ui_mng = eina_vogon_get_ui_manager(vogon);
	g_return_val_if_fail(ui_mng != NULL, FALSE);

	// Remove action group
	if (self->vogon_ag)
	{
		gtk_ui_manager_remove_action_group(ui_mng, self->vogon_ag);
		gel_free_and_invalidate(self->vogon_ag, NULL, g_object_unref);
		gtk_ui_manager_ensure_update(ui_mng); 
	}

	// Unmerge UI
	if (self->vogon_merge_id)
	{
		gtk_ui_manager_remove_ui(ui_mng, self->vogon_merge_id);
		gtk_ui_manager_ensure_update(ui_mng);
		self->vogon_merge_id = 0;
	}

	self->status_icon = NULL;

	return TRUE;
}
#endif

// -------------
static void
ntfy_sync(EinaNtfyData *self)
{
	LomoPlayer *lomo     = eina_application_get_lomo(self->app);
	LomoStream *stream   = lomo_player_get_current_stream(lomo);

	if (lomo_player_get_state(lomo) != LOMO_STATE_PLAY)
		return;

	if (self->stream != stream)
	{
		if (self->stream)
		{
			g_signal_handlers_disconnect_by_func(self->stream, stream_em_updated_cb, self);
			g_object_weak_unref(G_OBJECT(self->stream), (GWeakNotify) stream_weak_ref_cb, self);
		}
		self->stream = stream;
		g_object_weak_ref(G_OBJECT(self->stream), (GWeakNotify) stream_weak_ref_cb, self);
		g_signal_connect(self->stream, "extended-metadata-updated", G_CALLBACK (stream_em_updated_cb), self);
	}

	GdkPixbuf *scaled = NULL;

	// Build body
	gchar *tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	gchar *bname = g_uri_unescape_string(tmp, NULL);
	g_free(tmp);

	const gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	const gchar *title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	gchar *body = g_strdup_printf("<b>%s</b>\n%s", title ? title : bname, artist ? artist : "");

	gel_free_and_invalidate(bname, NULL, g_free);

	notify_notification_update(self->ntfy, N_("Playing now"), body, NULL);

	const gchar *uri = (const gchar *) lomo_stream_get_extended_metadata(stream, "art-uri");
	if (!uri)
	{
		g_warn_if_fail(uri);
		goto ntfy_sync_show;
	}

	GFile *f = g_file_new_for_uri(uri);
	GInputStream *s = G_INPUT_STREAM(g_file_read(f, NULL, NULL));
	g_object_unref(f);
	if (!G_IS_INPUT_STREAM(s))
	{
		g_warn_if_fail(G_IS_INPUT_STREAM(s));
		goto ntfy_sync_show;
	}

	GdkPixbuf *pb = gdk_pixbuf_new_from_stream(s, NULL, NULL);
	g_object_unref(s);
	if (!GDK_IS_PIXBUF(pb))
	{
		g_warn_if_fail(GDK_IS_PIXBUF(pb));
		goto ntfy_sync_show;
	}

	gdouble sx = (gdouble) 64 / gdk_pixbuf_get_width(pb);
	gdouble sy = (gdouble) 64 / gdk_pixbuf_get_height(pb);

	gint dx = MIN(sx,sy) * gdk_pixbuf_get_width(pb);
	gint dy = MIN(sx,sy) * gdk_pixbuf_get_height(pb);

	scaled = gdk_pixbuf_scale_simple(pb, dx, dy, GDK_INTERP_NEAREST);
	g_object_unref(pb);
	notify_notification_set_icon_from_pixbuf(self->ntfy, scaled);

ntfy_sync_show:
	notify_notification_show(self->ntfy, NULL);
	gel_free_and_invalidate(body,   NULL, g_free);
	gel_free_and_invalidate(scaled, NULL, g_object_unref);
}
// -------------

#if HAVE_VOGON
static void
engine_plugin_init_cb(GelPluginEngine *engine, GelPlugin *plugin, EinaNtfyData *self)
{
	if (!g_str_equal(gel_plugin_get_info(plugin)->name, "vogon"))
		return;
	vogon_enable(self);
}

static void
engine_plugin_fini_cb(GelPluginEngine *engine, GelPlugin *plugin, EinaNtfyData *self)
{
	if (!g_str_equal(gel_plugin_get_info(plugin)->name, "vogon"))
		return;
	vogon_disable(self);
}
#endif

static void
stream_weak_ref_cb(EinaNtfyData *self, GObject *_stream)
{
	self->stream = NULL;
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaNtfyData *self)
{
	if (self->stream)
	{
		g_signal_handlers_disconnect_by_func(self->stream, stream_em_updated_cb, self);
		self->stream = NULL;
	}
	ntfy_sync(self);
}

static void
lomo_state_notify_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaNtfyData *self)
{
	if (!g_str_equal(pspec->name, "state"))
		return;
	// g_warning("State: %s", lomo_state_to_str(lomo_player_get_state(lomo)));
	ntfy_sync(self);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfyData *self)
{
	if (lomo_player_get_current_stream(lomo) != stream)
		return;
	ntfy_sync(self);
}

static void
stream_em_updated_cb(LomoStream *stream, const gchar *key, EinaNtfyData *self)
{
	g_return_if_fail(LOMO_IS_STREAM(stream));
	g_return_if_fail(key);

	if (!g_str_equal(key, "art-uri"))
		return;
	ntfy_sync(self);
}

#if HAVE_VOGON
static void
action_activate_cb(GtkAction *action, EinaNtfyData *self)
{
	const gchar *name = gtk_action_get_name(action);
	if (g_str_equal(ACTION_NAME, name))
	{
		// FIXME: Check if callback is called
		g_settings_set_boolean(self->settings, EINA_NTFY_ENABLED_KEY, gtk_toggle_action_get_active((GtkToggleAction*) action));
		/*
		gboolean active = gtk_toggle_action_get_active((GtkToggleAction*) action);
		if (active)
			ntfy_enable(self, NULL);
		else
			ntfy_disable(self);

		eina_conf_set_bool(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH "/Notifications", self->enabled);
		gtk_toggle_action_set_active((GtkToggleAction*) action, self->enabled);
		*/
	}
}
#endif

static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaNtfyData *self)
{
	if (g_str_equal(key, EINA_NTFY_ENABLED_KEY))
	{
		if (g_settings_get_boolean(settings, key))
			ntfy_enable(self, NULL);
		else
			ntfy_disable(self);
		return;
	}

	g_warning(N_("Unhanded key '%s'"), key);
}

