#ifndef _MPRIS_BRIDGE_H
#define _MPRIS_BRIDGE_H
#endif

#include <eina/mpris/mpris.h>

void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	MprisPlugin *plugin);

GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	MprisPlugin *plugin);

gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	MprisPlugin *plugin);

void
player_method_call_cb(GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   MprisPlugin *plugin);

GVariant *
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	MprisPlugin *plugin);

gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	MprisPlugin *plugin);

const GDBusInterfaceVTable root_vtable =
{
	(GDBusInterfaceMethodCallFunc)  root_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) root_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) root_set_property_cb
};

const GDBusInterfaceVTable player_vtable =
{
	(GDBusInterfaceMethodCallFunc)  player_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) player_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) player_set_property_cb
};

