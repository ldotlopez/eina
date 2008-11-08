#define GEL_DOMAIN "Adb"
#include "upgrade.h"
#include <sqlite3.h>
#include <gel/gel.h>

static gboolean
adb_db_upgrade_0(Adb *self)
{
	gint i;
	char *errmsg = NULL;
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
	for (i = 0; querys[i]; i++)
	{
		// gel_warn("Execute SQL('%s')", querys[i]);
		if (sqlite3_exec(self->db, querys[i], NULL, NULL, &errmsg) != SQLITE_OK)
		{
			// gel_error("Error: %s", errmsg);
			sqlite3_free(errmsg);
			return FALSE;
		}
	}
	return TRUE;
}

static gboolean
adb_db_upgrade_1(Adb *self)
{
	gint i;
	char *errmsg = NULL;
	const gchar *querys[] = {
		"DROP TABLE IF EXISTS tags",

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
		");"
	};
	for (i = 0; querys[i]; i++)
	{
		gel_warn("Execute SQL('%s')", querys[i]);
		if (sqlite3_exec(self->db, querys[i], NULL, NULL, &errmsg) != SQLITE_OK)
		{
			gel_error("Error: %s", errmsg);
			sqlite3_free(errmsg);
			return FALSE;
		}
	}
	return TRUE;
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

	for (i = from_version + 1; upgrade_funcs[i]; i++)
	{
		gboolean (*func)(Adb *self);
		func = upgrade_funcs[i];
		// gel_warn("Execute %d->%d %p", i-1, i, upgrade_funcs[i]);
		if (func(self) == FALSE)
			return FALSE;
	}
	return TRUE;
}
