#include "eina-adb.h"
#include <glib/gi18n.h>
#include <gel/gel.h>

GEL_DEFINE_QUARK_FUNC(adb_upgrade);

gboolean
eina_adb_upgrade_schema(EinaAdb *self, gchar *schema, gchar **queries[], GError **error)
{
	g_return_val_if_fail(EINA_IS_ADB(self), FALSE);
	g_return_val_if_fail(schema  != NULL,   FALSE);
	g_return_val_if_fail(queries != NULL,   FALSE);

	gint schema_version = eina_adb_schema_get_version(self, schema);

	g_warning("Current '%s' schema: %d", schema, schema_version);
	gint i;
	for (i = schema_version + 1; queries[i] != NULL; i++)
	{
		g_warning("Use queries from schemas[%d]", i);
		if (!eina_adb_query_exec(self, "BEGIN TRANSACTION;"))
		{
			g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_ERROR_TRANSACTION,
				N_("Cannot begin transaction"));
			break;
		}

		gint j;
		for (j = 0; queries[i][j] != NULL; j++)
		{
			if (!eina_adb_query_exec(self, queries[i][j]))
			{
				g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_QUERY_FAILED,
					N_("Query %d for core schema version %d failed"), j, i);
				eina_adb_query_exec(self, "ROLLBACK;");
				break;
			}
		}

		if (!eina_adb_query_exec(self, "END TRANSACTION;"))
		{
			g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_ERROR_TRANSACTION,
				N_("Cannot end transaction"));
			break;
		}

		eina_adb_schema_set_version(self, schema, i);
	}

	return (queries[i] == NULL);
}

#if 0
gboolean
adb_upgrade_database(EinaAdb *self, GError **error)
{
	gint core_version = eina_adb_schema_get_version(self, "core");
	if (core_version < 0)
		g_set_error(error, 0, 0, "Unable to upgrade");
	
	g_warning("Current core schema: %d, current: %ld", core_version, G_N_ELEMENTS(schemas));
	for (; core_version < G_N_ELEMENTS(schemas); core_version++)
	{
		g_warning("Use queries from schemas[%d]", core_version);
		if (!eina_adb_query_exec(self, "BEGIN TRANSACTION;"))
		{
			g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_ERROR_TRANSACTION,
				N_("Cannot begin transaction"));
			break;
		}

		gint i;
		for (i = 0; schemas[core_version][i] != NULL; i++)
		{
			if (!eina_adb_query_exec(self, schemas[core_version][i]))
			{
				g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_QUERY_FAILED,
					N_("Query %d for core schema version %d failed"), i, core_version);
				eina_adb_query_exec(self, "ROLLBACK;");
				break;
			}
		}

		if (!eina_adb_query_exec(self, "END TRANSACTION;"))
		{
			g_set_error(error, adb_upgrade_quark(), EINA_ADB_UPGRADE_ERROR_TRANSACTION,
				N_("Cannot end transaction"));
			break;
		}

		eina_adb_schema_set_version(self, "core", core_version + 1);
	}

	return (core_version == G_N_ELEMENTS(schemas));
}
#endif
