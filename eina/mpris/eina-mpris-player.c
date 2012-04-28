/*
 * eina/mpris/eina-mpris-player.c
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "eina-mpris-player.h"
#include "eina-mpris-player-spec.h"
#include <gel/gel.h>
#include <glib/gi18n.h>
#include <eina/ext/eina-window.h>
#include <eina/lomo/eina-lomo-plugin.h>

G_DEFINE_TYPE (EinaMprisPlayer, eina_mpris_player, G_TYPE_OBJECT)

struct _EinaMprisPlayerPrivate {
	EinaApplication *app;
	gchar           *bus_name_suffix;
	GDBusConnection *conn;
	GDBusNodeInfo   *nodeinfo;
	guint bus_id, root_id, player_id, playlist_id;
	GHashTable    *prop_changes;
	guint prop_change_id;
};

enum
{
	PROP_APP = 1,
	PROP_BUS_NAME_SUFFIX
#if 0
	PROP_CAN_QUIT,
	PROP_CAN_RAISE,
	PROP_HAS_TRACK_LIST,
	PROP_IDENTITY,
	PROP_DESKTOP_ENTRY,
	PROP_SUPPORTED_URI_SCHEMES,
	PROP_SUPPORTED_MIME_TYPES
#endif
};

static void
set_application(EinaMprisPlayer *self, EinaApplication *app);
static void
set_bus_name_suffix(EinaMprisPlayer *self, const gchar *bus_name_suffix);

static GVariant*
build_metadata_variant(LomoStream *stream);

static void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaMprisPlayer *self);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaMprisPlayer *self);

static void
server_name_lost_cb (GDBusConnection *connection, const gchar *name, gpointer user_data);
static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data);

/*
 * Root node call/get/set
 */
static void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self);
static GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self);
static gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self);

GDBusInterfaceVTable root_vtable =
{
	(GDBusInterfaceMethodCallFunc)  root_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) root_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) root_set_property_cb
};

/*
 * Player node call/get/set
 */
static void
player_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self);
static GVariant*
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self);
static gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self);

GDBusInterfaceVTable player_vtable =
{
	(GDBusInterfaceMethodCallFunc)  player_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) player_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) player_set_property_cb
};

/*
 * Player node call/get/set
 */
static void
playlist_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self);
static GVariant*
playlist_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self);
static gboolean
playlist_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self);

GDBusInterfaceVTable playlist_vtable =
{
	(GDBusInterfaceMethodCallFunc)  playlist_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) playlist_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) playlist_set_property_cb
};

static void
eina_mpris_player_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_APP:
		g_value_set_object(value, eina_mpris_player_get_application(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_BUS_NAME_SUFFIX:
		g_value_set_string(value, eina_mpris_player_get_bus_name_suffix(EINA_MPRIS_PLAYER(object)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_mpris_player_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_APP:
		set_application(EINA_MPRIS_PLAYER(object), g_value_get_object(value));
		break;
	case PROP_BUS_NAME_SUFFIX:
		set_bus_name_suffix(EINA_MPRIS_PLAYER(object), g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_mpris_player_dispose (GObject *object)
{
	EinaMprisPlayer *self = EINA_MPRIS_PLAYER(object);

	// FIXME: Disconnect signals
	// FIXME: Merge this code with complete_setup_error goto

	gel_free_and_invalidate(self->priv->bus_id, 0, g_bus_unown_name);

	if (self->priv->player_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->player_id);
		self->priv->player_id = 0;
	}

	if (self->priv->playlist_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->playlist_id);
		self->priv->playlist_id = 0;
	}

	if (self->priv->root_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->root_id);
		self->priv->root_id = 0;
	}

	gel_free_and_invalidate(self->priv->nodeinfo, NULL, g_dbus_node_info_unref);
	gel_free_and_invalidate(self->priv->conn,     NULL, g_object_unref);

	gel_free_and_invalidate(self->priv->app, NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->bus_name_suffix, NULL, g_free);

	G_OBJECT_CLASS (eina_mpris_player_parent_class)->dispose (object);
}

static void
eina_mpris_player_class_init (EinaMprisPlayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaMprisPlayerPrivate));

	object_class->get_property = eina_mpris_player_get_property;
	object_class->set_property = eina_mpris_player_set_property;
	object_class->dispose = eina_mpris_player_dispose;

	g_object_class_install_property(object_class, PROP_APP,
		g_param_spec_object("application", "application", "application",
			EINA_TYPE_APPLICATION, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class, PROP_BUS_NAME_SUFFIX,
		g_param_spec_string("bus-name-suffix", "bus-name-suffix", "bus-name-suffix",
			NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

static void
eina_mpris_player_init (EinaMprisPlayer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerPrivate);
	self->priv->prop_changes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
}

EinaMprisPlayer*
eina_mpris_player_new (EinaApplication *app, const gchar *bus_name_suffix)
{
	return g_object_new (EINA_TYPE_MPRIS_PLAYER,
		"application", app,
		"bus-name-suffix", bus_name_suffix,
		NULL);
}

static void
complete_setup(EinaMprisPlayer *self)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));

	GError *error = NULL;

	self->priv->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!self->priv->conn)
	{
		g_warning(_("Unable to get session bus: '%s'"), error->message);
		goto complete_setup_error;
	}

	self->priv->nodeinfo = g_dbus_node_info_new_for_xml(mpris_spec_xml, &error);
	if (!self->priv->nodeinfo)
	{
		g_warning(_("Unable to parse MPRIS spec XML: '%s'"), error->message);
		goto complete_setup_error;
	}

	self->priv->root_id = g_dbus_connection_register_object(self->priv->conn,
		MPRIS_SPEC_OBJECT_PATH,
        g_dbus_node_info_lookup_interface(self->priv->nodeinfo, MPRIS_SPEC_ROOT_INTERFACE),
        &root_vtable,
        self,
        NULL,
        &error);
	if (!self->priv->root_id)
    {
        g_warning(_("Unable to register interface %s: '%s'"), MPRIS_SPEC_ROOT_INTERFACE, error->message);
		goto complete_setup_error;
    }

	self->priv->player_id = g_dbus_connection_register_object(self->priv->conn,
		MPRIS_SPEC_OBJECT_PATH,
		g_dbus_node_info_lookup_interface(self->priv->nodeinfo, MPRIS_SPEC_PLAYER_INTERFACE),
		&player_vtable,
		self,
		NULL,
		&error);
	if (!self->priv->player_id)
	{
		g_warning(_("Unable to register interface %s: '%s'"), MPRIS_SPEC_PLAYER_INTERFACE, error->message);
		goto complete_setup_error;
	}

	self->priv->playlist_id = g_dbus_connection_register_object(self->priv->conn,
		MPRIS_SPEC_OBJECT_PATH,
		g_dbus_node_info_lookup_interface(self->priv->nodeinfo, MPRIS_SPEC_PLAYLIST_INTERFACE),
		&playlist_vtable,
		self,
		NULL,
		&error);
	if (!self->priv->playlist_id)
	{
		g_warning(_("Unable to register interface %s: '%s'"), MPRIS_SPEC_PLAYER_INTERFACE, error->message);
		goto complete_setup_error;
	}

	gchar *bus_name = g_strconcat(MPRIS_SPEC_BUS_NAME_PREFIX, ".", self->priv->bus_name_suffix, NULL);
	self->priv->bus_id = g_bus_own_name(G_BUS_TYPE_SESSION,
		bus_name,
		G_BUS_NAME_OWNER_FLAGS_NONE,
		NULL,
		(GBusNameAcquiredCallback) server_name_acquired_cb,
		(GBusNameLostCallback)     server_name_lost_cb,
		self,
		NULL);
	if (!self->priv->bus_id)
	{
		g_warning(_("Unable to own bus name '%s'"), bus_name);
		g_free(bus_name);
		goto complete_setup_error;
	}
	g_free(bus_name);

	LomoPlayer *lomo = eina_application_get_lomo(self->priv->app);
	g_signal_connect(lomo, "notify::state", (GCallback) lomo_notify_state_cb, self);
	g_signal_connect(lomo, "change",        (GCallback) lomo_change_cb, self);

	return;

complete_setup_error:

	gel_free_and_invalidate(error, NULL, g_error_free);
	gel_free_and_invalidate(self->priv->bus_id, 0, g_bus_unown_name);

	if (self->priv->player_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->player_id);
		self->priv->player_id = 0;
	}
	if (self->priv->playlist_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->playlist_id);
		self->priv->playlist_id = 0;
	}
	if (self->priv->root_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->root_id);
		self->priv->root_id = 0;
	}

	gel_free_and_invalidate(self->priv->nodeinfo, NULL, g_dbus_node_info_unref);
	gel_free_and_invalidate(self->priv->conn,     NULL, g_object_unref);
}

EinaApplication *
eina_mpris_player_get_application (EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), NULL);
	return EINA_APPLICATION(self->priv->app);
}

static void
set_application(EinaMprisPlayer *self, EinaApplication *app)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));
	g_return_if_fail(EINA_IS_APPLICATION(app));

	g_return_if_fail(self->priv->app == NULL);

	self->priv->app = g_object_ref(app);

	if (self->priv->app && self->priv->bus_name_suffix)
		complete_setup(self);

	g_object_notify((GObject *) self, "application");
}

const gchar *
eina_mpris_player_get_object_path(EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), NULL);
	return self->priv->bus_name_suffix;
}

static void
set_bus_name_suffix(EinaMprisPlayer *self, const gchar *bus_name_suffix)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));
	g_return_if_fail(bus_name_suffix != NULL);

	g_return_if_fail(self->priv->bus_name_suffix == NULL);

	self->priv->bus_name_suffix = g_strdup(bus_name_suffix);

	if (self->priv->app && self->priv->bus_name_suffix)
		complete_setup(self);

	g_object_notify((GObject *) self, "bus-name-suffix");
}

const gchar*
eina_mpris_player_get_bus_name_suffix(EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), NULL);
	return self->priv->bus_name_suffix;
}

static gboolean
emit_properties_change_idle_cb(EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), FALSE);

	self->priv->prop_change_id = 0;
	g_return_val_if_fail(g_hash_table_size(self->priv->prop_changes) > 0, FALSE);

	GVariantBuilder *properties = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));

	GHashTableIter iter;
	gpointer propname, propvalue;
	GList *invalid_props = NULL;
	g_hash_table_iter_init (&iter, self->priv->prop_changes);
	while (g_hash_table_iter_next (&iter, &propname, &propvalue))
	{
		g_variant_builder_add (properties, "{sv}", propname, propvalue);
		invalid_props = g_list_prepend(invalid_props, propname);
	}
	gchar **invalid_props_strv = gel_list_to_strv(invalid_props, FALSE); // Save memory
	g_list_free(invalid_props);

	GVariant *parameters = g_variant_new ("(sa{sv}^as)",
				    MPRIS_SPEC_PLAYER_INTERFACE,
				    properties,
				    invalid_props_strv);
	GError *error = NULL;
	g_dbus_connection_emit_signal (self->priv->conn,
				       NULL,
					   MPRIS_SPEC_OBJECT_PATH,
				       "org.freedesktop.DBus.Properties",
				       "PropertiesChanged",
				       parameters,
				       &error);
	if (error != NULL)
	{
		g_warning ("Unable to send MPRIS property changes: %s", error->message);
		g_clear_error (&error);
	}

	// Final cleanup, 100% safe
	g_variant_unref(parameters);
	g_variant_builder_unref (properties);
	g_free(invalid_props_strv);
	g_hash_table_remove_all(self->priv->prop_changes);

	return FALSE;
}

static void
emit_properties_change(EinaMprisPlayer *self)
{
	if (self->priv->prop_change_id)
		return;
	else
		self->priv->prop_change_id = g_idle_add((GSourceFunc) emit_properties_change_idle_cb, self);
}

static void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaMprisPlayer *self)
{
	const gchar *value = NULL;
	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		value = "Playing";
		break;
	case LOMO_STATE_PAUSE:
		value = "Paused";
		break;
	case LOMO_STATE_STOP:
		value = "Stopped";
		break;
	default:
		g_warning(_("Unknow status %d"), state);
		return;
	}

	g_hash_table_insert(self->priv->prop_changes, g_strdup("PlaybackStatus"), g_variant_new_string(value));
	emit_properties_change(self);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaMprisPlayer *self)
{
	if (to == -1)
		return;

	LomoStream *stream = lomo_player_get_nth_stream(lomo, to);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	if (lomo_stream_get_all_tags_flag(stream))
	{
		g_hash_table_insert(self->priv->prop_changes, g_strdup("Metadata"), build_metadata_variant(stream));
		emit_properties_change(self);
	}
}

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	// g_warning("%s", __FUNCTION__);
}

static void
server_name_lost_cb (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_warning("%s", __FUNCTION__);
}

/*
 * Section: DBus access functions
 */
static void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self)
{
	if (g_str_equal("Raise", method_name))
	{
		GtkWindow *window = GTK_WINDOW(eina_application_get_window(self->priv->app));
		if (!window || !GTK_IS_WINDOW(window))
		{
			g_warn_if_fail(GTK_IS_WINDOW(window));
			goto root_method_call_cb_error;
		}

		gtk_window_present(window);
	}

	else if (g_str_equal("Quit", method_name))
		g_application_release(G_APPLICATION(self->priv->app));

	else
		goto root_method_call_cb_error;

	g_dbus_method_invocation_return_value(invocation, NULL);

root_method_call_cb_error:
	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

static GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self)
{
	// g_warning("%s.%s", interface_name, property_name);

	if (g_str_equal("CanQuit", property_name) ||
		g_str_equal("CanRaise", property_name))
	{
		return g_variant_new_boolean(TRUE);
	}

	else if (g_str_equal("HasTrackList", property_name))
	{
		return g_variant_new_boolean(FALSE);
	}

	else if (g_str_equal("Identity", property_name))
	{
		return g_variant_new_string("Eina Music Player");
	}

	else if (g_str_equal("DesktopEntry", property_name))
	{
		return g_variant_new_string(PACKAGE);
	}

	else if (g_str_equal("SupportedUriSchemes", property_name))
	{
		const char *fake_supported_schemes[] = { "file", "http", "cdda", "smb", "sftp", NULL };
		return g_variant_new_strv (fake_supported_schemes, -1);
	}

	else if (g_str_equal(property_name, "SupportedMimeTypes"))
	{
		const char *fake_supported_mimetypes[] = { "application/ogg", "audio/x-vorbis+ogg", "audio/x-flac", "audio/mpeg", NULL };
		return g_variant_new_strv (fake_supported_mimetypes, -1);
	}

	else
		goto root_get_property_cb_error;

root_get_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self)
{
	// g_warning("%s.%s", interface_name, property_name);
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

static void
player_method_call_cb(GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   EinaMprisPlayer *self)

{
	// g_warning("%s.%s", interface_name, method_name);
	LomoPlayer *lomo = eina_application_get_lomo(self->priv->app);

	if (g_str_equal("PlayPause", method_name))
	{
		lomo_player_set_state(lomo,
			(lomo_player_get_state(lomo) == LOMO_STATE_PLAY) ? LOMO_STATE_PAUSE : LOMO_STATE_PLAY, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}

	else if (g_str_equal("Next", method_name))
	{
		lomo_player_go_next(lomo, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}

	else if (g_str_equal("Previous", method_name))
	{
		lomo_player_go_previous(lomo, NULL);
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}

	const gchar *nulls[] = { "Pause", "Play", "Stop", "Seek", "SetPosition", "OpenUri" };
	for (guint i = 0; i < G_N_ELEMENTS(nulls); i++)
	{
		if (g_str_equal(nulls[i], method_name))
		{
			g_dbus_method_invocation_return_value(invocation, NULL);
			return;
		}
	}

	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

static GVariant*
build_metadata_variant(LomoStream *stream)
{
	GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
	g_return_val_if_fail(LOMO_IS_STREAM(stream), g_variant_builder_end(builder));

	GList *taglist = lomo_stream_get_tags(stream);
	GList *l = taglist;
	while (l)
	{
		const gchar *tag = (const gchar *) l->data;
		if (lomo_tag_get_gtype(tag) != G_TYPE_STRING)
			goto build_metadata_variant_loop_next;

		// Translate LomoTag to xesam tag
		const gchar *xesam_tag = NULL;
		GVariant    *tag_variant = NULL;
		if (g_str_equal(LOMO_TAG_ALBUM, tag))
			xesam_tag = "album";

		else if (g_str_equal(LOMO_TAG_TITLE, tag))
			xesam_tag = "title";

		else if (g_str_equal(LOMO_TAG_ARTIST, tag))
		{
			xesam_tag = "artist";
			gchar *artists[] = { lomo_stream_strdup_tag_value(stream, tag), NULL };
			tag_variant = g_variant_new_strv((const gchar * const *) artists, -1);
			g_free(artists[0]);
		}

		else
			goto build_metadata_variant_loop_next;

		if (tag_variant == NULL)
		{
			gchar *tag_str = lomo_stream_strdup_tag_value(stream, tag);
			if (tag_str)
				tag_variant = g_variant_new_string(tag_str);
			g_free(tag_str);
		}

		if (!xesam_tag || !tag_variant)
			goto build_metadata_variant_loop_next;

		gchar *tmp = g_strconcat("xesam:", xesam_tag, NULL);
		g_variant_builder_add(builder, "{sv}", tmp, tag_variant);
		g_free(tmp);

build_metadata_variant_loop_next:
		l = l->next;
	}
	gel_list_deep_free(taglist, g_free);

	const GValue *art_value = lomo_stream_get_extended_metadata(stream, LOMO_STREAM_EM_ART_DATA);
	if (art_value && G_VALUE_HOLDS_STRING(art_value))
		g_variant_builder_add(builder, "{sv}",
			"mpris:artUrl",
			g_variant_new_string(g_value_get_string(art_value)));

	GVariant *ret = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);

	return ret;
}

static GVariant *
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self)
{
	// g_warning("%s.%s", interface_name, property_name);

	// FIXME: Return error if there are invalid parameters

	g_warn_if_fail(EINA_IS_APPLICATION(self->priv->app));

	LomoPlayer *lomo = eina_application_get_lomo(self->priv->app);
	if (!lomo || !LOMO_IS_PLAYER(lomo))
	{
		g_warn_if_fail(LOMO_IS_PLAYER(lomo));
		goto player_get_property_cb_error;
	}

	// CanPlay, CanPause, CanSeek, CanControl
	const gchar *bools[] = { "CanPlay", "CanPause", "CanSeek", "CanControl" };
	for (guint i = 0; i < G_N_ELEMENTS(bools); i++)
	{
		if (g_str_equal(bools[i], property_name))
			return g_variant_new_boolean(TRUE);
	}

	// CanGoPrevious
	if (g_str_equal("CanGoPrevious", property_name))
	{
		return g_variant_new_boolean(lomo_player_get_can_go_next(lomo));
	}


	// CanGoNext
	if (g_str_equal("CanGoNext", property_name))
	{
		return g_variant_new_boolean(lomo_player_get_can_go_next(lomo));
	}

	// LoopStatus
	if (g_str_equal("LoopStatus", property_name))
	{
		return g_variant_new_string(lomo_player_get_repeat(lomo) ?
			"Playlist" : "None");
	}

	// Metadata
	if (g_str_equal("Metadata", property_name))
	{
		LomoStream *stream = lomo_player_get_current_stream(lomo);
		if (!LOMO_IS_STREAM(stream))
		{
			g_warn_if_fail(LOMO_IS_STREAM(stream));
			goto player_get_property_cb_error;
		}

		return build_metadata_variant(stream);
	}

	// MinimumRate, MaximumRate
	if (g_str_equal("MinimumRate", property_name) ||
		g_str_equal("MaximumRate", property_name))
		return g_variant_new_double(1.0);

	// PlaybackStatus
	if (g_str_equal("PlaybackStatus", property_name))
	{
		const gchar *ret = "Stopped";
		switch (lomo_player_get_state(lomo))
		{
		case LOMO_STATE_PLAY:
			ret = "Playing";
			break;
		case LOMO_STATE_PAUSE:
			ret = "Paused";
			break;
		default:
			break;
		}
		return g_variant_new_string(ret);
	}

	// Position
	if (g_str_equal("Position", property_name))
	{
		return g_variant_new_int64(lomo_player_get_length(lomo));
	}

	// Rate
	if (g_str_equal("Rate", property_name))
	{
		return g_variant_new_double(1.0);
	}

	// Shuffle
	if (g_str_equal("Shuffle", property_name))
	{
		return g_variant_new_boolean(lomo_player_get_repeat(lomo));
	}

	// Volume
	if (g_str_equal("Volume", property_name))
	{
		return g_variant_new_double(lomo_player_get_volume(lomo) / (double) 100.0);
	}

	goto player_get_property_cb_error;

player_get_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self)
{
	// g_warning("%s.%s", interface_name, property_name);

	const gchar *props[] = { "LoopStatus", "Rate", "Shuffle", "Volume" };
	for (guint i = 0; i < G_N_ELEMENTS(props); i++)
		if (g_str_equal(props[i], property_name))
			return TRUE;

	goto player_set_property_cb_error;

player_set_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

static void
playlist_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self)
{
	if (g_strcmp0 (object_path, MPRIS_SPEC_OBJECT_PATH) != 0 ||
	    g_strcmp0 (interface_name, MPRIS_SPEC_PLAYLIST_INTERFACE) != 0)
	{
		g_dbus_method_invocation_return_error (invocation,
			G_DBUS_ERROR,
			G_DBUS_ERROR_NOT_SUPPORTED,
			"Method %s.%s not supported",
			interface_name,
			method_name);
		return;
	}

	if (g_strcmp0 (method_name, "ActivatePlaylist") == 0) {
		/*
		ActivateSourceData data;

		data.plugin = plugin;
		g_variant_get (parameters, "(&o)", &data.playlist_id);
		gtk_tree_model_foreach (GTK_TREE_MODEL (plugin->page_model),
			(GtkTreeModelForeachFunc) activate_source_by_id,
			&data);
		*/
		g_dbus_method_invocation_return_value (invocation, NULL);
		return;
    }

	else if (g_strcmp0 (method_name, "GetPlaylists") == 0) {
		guint index;
		guint max_count;
		const gchar *order;
		gboolean reverse;

		g_variant_get (parameters, "(uu&sb)", &index, &max_count, &order, &reverse);
		GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("a(oss)"));

		if (max_count > 0)
		{
			gchar *path = g_strdup_printf("%s/Playlists/default", EINA_APP_PATH_DOMAIN);
			g_variant_builder_add (builder, "(oss)", path, "default", "");
			g_free(path);
		}
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a(oss))", builder));
		g_variant_builder_unref (builder);

		#if 0
		gtk_tree_model_foreach (GTK_TREE_MODEL (plugin->page_model),
			(GtkTreeModelForeachFunc) get_playlist_list,
			&playlists);

		/* list is already in reverse order, reverse it again if we want normal order */
		if (reverse == FALSE) {
			playlists = g_list_reverse (playlists);
		}

		builder = g_variant_builder_new (G_VARIANT_TYPE ("a(oss)"));
		for (l = playlists; l != NULL; l = l->next) {
			RBSource *source;
			const char *id;
			char *name;

			if (index > 0) {
				index--;
				continue;
			}

			source = l->data;
			id = g_object_get_data (G_OBJECT (source), MPRIS_PLAYLIST_ID_ITEM);
			g_object_get (source, "name", &name, NULL);
			g_variant_builder_add (builder, "(oss)", id, name, "");
			g_free (name);

			if (max_count > 0) {
				max_count--;
				if (max_count == 0) {
					break;
				}
			}
		}

		g_list_free (playlists);
		g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a(oss))", builder));
		g_variant_builder_unref (builder);
		#endif
	}

	else
	{
		g_dbus_method_invocation_return_error (invocation,
			G_DBUS_ERROR,
			G_DBUS_ERROR_NOT_SUPPORTED,
			"Method %s.%s not supported",
			interface_name,
			method_name);
	}
}

static GVariant*
playlist_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self)
{
	if (g_strcmp0(property_name, "PlaylistCount") == 0)
	{
		return g_variant_new_uint32(1);
	}

	else if (g_strcmp0(property_name, "Orderings") == 0)
	{
		const char *orderings[] = {
			/* "Alphabetical", */ NULL
		};
		return g_variant_new_strv (orderings, -1);
	}

	else if (g_strcmp0 (property_name, "ActivePlaylist") == 0)
	{
		gchar *playlist_path = g_strdup_printf("%s/Playlists/default", EINA_APP_PATH_DOMAIN);
		GVariant *ret = g_variant_new ("(b(oss))", TRUE, playlist_path, "default", "");
		g_free(playlist_path);

		return ret;
	}

	g_set_error (error,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"get_property %s.%s not supported",
		interface_name,
		property_name);
	return NULL;
}

static gboolean
playlist_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self)
{
	g_set_error (error,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"set_property %s.%s not supported",
		interface_name,
		property_name);
	return FALSE;
}

