#include "eina-adb.h"
#include <glib/gi18n.h>
#include <gel/gel.h>

gboolean
eina_adb_upgrade_schema(EinaAdb *self, gchar *schema, EinaAdbFunc callbacks[], GError **error)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);

	gint schema_version = eina_adb_schema_get_version(self, schema);

	g_warning("Current '%s' schema: %d", schema, schema_version);
	gint i;
	for (i = schema_version + 1; callbacks[i] != NULL; i++)
	{
		g_warning("Use callbacks from callbacks[%d]", i);
		if (!callbacks[i](self, error))
			break;
		eina_adb_schema_set_version(self, schema, i);
	}

	return (callbacks[i] == NULL);
}


