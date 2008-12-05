#define GEL_DOMAIN "Adb"
#include "upgrade.h"
#include "common.h"
#include <sqlite3.h>
#include <gel/gel.h>

static gboolean
adb_db_upgrade_0(Adb *self, GError **error)
{
	const gchar *querys[] = {
		"DROP TABLE IF EXISTS variables;",

		"CREATE TABLE variables ("
		"key VARCHAR(30) PRIMARY KEY,"
		"value VARCHAR(30)"
		");",

		"DROP TABLE IF EXISTS streams;",

		"CREATE TABLE streams ("
		"sid INTEGER PRIMARY KEY AUTOINCREMENT,"
		"uri VARCHAR(1024) UNIQUE,"
		"timestamp TIMESTAMP);",
		
		"INSERT INTO variables VALUES('schema-version', 0);",

		NULL };

	return adb_exec_querys(self, querys, NULL, error);
}

static gboolean
adb_db_upgrade_1(Adb *self, GError **error)
{
	const gchar *querys[] = {
		"DROP TABLE IF EXISTS tags;",

		"CREATE TABLE tags ("
		"tid INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name VARCHAR(63) UNIQUE"
		");",

		"CREATE TABLE metadata ("
		"tid INTEGER,"
		"sid INTEGER,"
		"value VARCHAR(63),"
		"PRIMARY KEY (tid,sid),"
		"FOREIGN KEY (tid) REFERENCES tags(tid),"
		"FOREIGN KEY (sid) REFERENCES streams(sid)"
		");",

		NULL
	};
	return adb_exec_querys(self, querys, NULL, error);
}

gboolean
adb_db_upgrade(Adb *self, gint from_version)
{
	gint i;
	gpointer upgrade_funcs[] = {
		adb_db_upgrade_0,
		adb_db_upgrade_1,
		NULL
	};

	GError *error = NULL;
	for (i = from_version + 1; upgrade_funcs[i]; i++)
	{
		gboolean (*func)(Adb *self, GError **error);
		func = upgrade_funcs[i];
		if (func(self, &error) == FALSE)
		{
			gel_error("Failed to upgrade ADB from version %d to %d: %s", i, i+1, error->message);
			g_error_free(error);
			return FALSE;
		}

		if (!adb_set_variable(self, "schema-version", G_TYPE_UINT, (gpointer) i))
		{
			gel_error("Cannot upgrade to version %d", i);
			return FALSE;
		}

		gel_error("Upgraded to version %d", i);
	}
	return TRUE;
}
