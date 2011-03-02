#include "mpris.h"

void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	MprisPlugin *plugin)
{
	g_warning("%s.%s", interface_name, method_name);

	if (g_str_equal("Quit", method_name) ||
		g_str_equal("Raise", method_name))
	{
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}
	else
		goto root_method_call_cb_error;

root_method_call_cb_error:
	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	MprisPlugin *plugin)
{
	g_warning("%s.%s", interface_name, property_name);

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
		return g_variant_new_string("eina.desktop");
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

gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	MprisPlugin *plugin)
{
	g_warning("%s.%s", interface_name, property_name);
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

void
player_method_call_cb(GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   MprisPlugin *plugin)

{
	g_warning("%s.%s", interface_name, method_name);

	const gchar *nulls[] = { "Next", "Previous", "Pause", "Play", "PlayPause", "Stop",
		"Seek", "SetPosition", "OpenUri" };
	for (guint i = 0; i < G_N_ELEMENTS(nulls); i++)
	{
		if (g_str_equal(nulls[i], method_name))
		{
			g_dbus_method_invocation_return_value(invocation, NULL);
			return;
		}
	}

	goto player_method_call_cb_error;

player_method_call_cb_error:
	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

GVariant *
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	MprisPlugin *plugin)
{
	g_warning("%s.%s", interface_name, property_name);

	const gchar *bools[] = { "Shuffle", "CanGoNext", "CanGoPrevious", "CanPlay", "CanPause", "CanSeek", "CanControl" };
	for (guint i = 0; i < G_N_ELEMENTS(bools); i++)
		if (g_str_equal(bools[i], property_name))
			return g_variant_new_boolean(TRUE);

	if (g_str_equal("PlaybackStatus", property_name))
		return g_variant_new_string("Stopped");

	if (g_str_equal("LoopStatus", property_name))
		return g_variant_new_string("None");

	if (g_str_equal("Rate", property_name))
		return g_variant_new_double(1.0);

	if (g_str_equal("Metadata", property_name))
	{
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
		g_variant_builder_end (builder);
	}

	if (g_str_equal("Volume", property_name))
		return g_variant_new_double(1.0);

	if (g_str_equal("Position", property_name))
		return g_variant_new_int64(0);

	if (g_str_equal("MinimumRate", property_name) ||
		g_str_equal("MaximumRate", property_name))
		return g_variant_new_double(1.0);

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

gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	MprisPlugin *plugin)
{
	g_warning("%s.%s", interface_name, property_name);
	
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

