#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-mpris-introspection.h"

#define BUS_NAME         "org.mpris.MediaPlayer2.eina"
#define OBJECT_PATH      "/org/mpris/MediaPlayer2"
#define MAIN_INTERFACE   "org.mpris.MediaPlayer2"
#define PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

typedef struct {
	LomoPlayer *lomo;
} MprisPlugin;

static void
server_bus_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);
static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);
static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new(MprisPlugin, 1);
	_plugin->lomo = eina_application_get_lomo(app);

	g_bus_own_name(G_BUS_TYPE_SESSION,
        BUS_NAME,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        (GBusAcquiredCallback)     server_bus_acquired_cb,
        (GBusNameAcquiredCallback) server_name_acquired_cb,
        (GBusNameLostCallback)     server_name_lost_cb,
        _plugin,
        NULL);

	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

/*
 * Glue code for org.mpris.MediaPlayer2
 */
static void
main_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data)
{
	g_warning("Call to %s", method_name);
	#if 0
	LomoPlayer *lomo = user_data;

	if (g_str_equal(method_name, "Play"))
	{
		/*
		gint change;
		g_variant_get (parameters, "(i)", &change);

		my_object_change_count (lomo, change);

		g_dbus_method_invocation_return_value (invocation, NULL);
		*/
		lomo_player_play(lomo);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	#endif
}

static GVariant *
main_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to get %s", property_name);
	return NULL;

	#if 0
	GVariant *ret;
	LomoPlayer *lomo = user_data;

	ret = NULL;
	if (g_strcmp0 (property_name, "Count") == 0)
		{
			ret = g_variant_new_int32 (lomo->count);
		}
	else if (g_strcmp0 (property_name, "Name") == 0)
		{
			ret = g_variant_new_string (lomo->name ? lomo->name : "");
		}

	return ret;
	#endif
}

static gboolean
main_interface_set_property(GDBusConnection  *connection,
const gchar  *sender,
const gchar  *object_path,
const gchar  *interface_name,
const gchar  *property_name,
GVariant     *value,
GError      **error,
gpointer      user_data)
{
	g_warning("Call to set %s", property_name);
	return FALSE;

	#if 0
	LomoPlayer *lomo = user_data;

	if (g_strcmp0 (property_name, "Count") == 0)
		{
			g_object_set (lomo, "count", g_variant_get_int32 (value), NULL);
		}
	else if (g_strcmp0 (property_name, "Name") == 0)
		{
			g_object_set (lomo, "name", g_variant_get_string (value, NULL), NULL);
		}

	return TRUE;
	#endif
}

/*
 * Glue code for org.mpris.MediaPlayer2.Player
 */
static void
player_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data)
{
	g_warning("Call to %s", method_name);
}

static GVariant *
player_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to get %s", property_name);
	return NULL;
}

static gboolean
player_interface_set_property(GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GVariant     *value,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to set %s", property_name);
	return FALSE;
}



static const GDBusInterfaceVTable interface_vtables[] =
{
	{
		main_interface_method_call,
		main_interface_get_property,
		main_interface_set_property
	},
	{
		player_interface_method_call,
		player_interface_get_property,
		player_interface_set_property
	}
};

static void
server_bus_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	GError *err = NULL;
	
    GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(_eina_mpris_introspection, &err);
	if (!info)
	{
		g_warning("%s", err->message);
		g_error_free(err);
		return;
	}

	const gchar const*interfaces[] = { MAIN_INTERFACE, PLAYER_INTERFACE };
	for (guint i = 0; i < G_N_ELEMENTS(interfaces); i++)
	{
		g_dbus_connection_register_object(connection,
	        OBJECT_PATH,
	        info->interfaces[i],
    	    &interface_vtables[i],
	        NULL,
			NULL,
    	    &err);
		if (err)
		{
			g_warning("Unable to register object: %s", err->message);
			g_error_free(err);
		}
		g_warning("%s", __FUNCTION__);
	}
}

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
}

static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
}

