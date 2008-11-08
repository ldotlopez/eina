#include "adb.h"
#define GEL_DOMAIN "Adb"
#include <sqlite3.h>
#include <gel/gel.h>
#include "upgrade.h"

struct _Adb {
	sqlite3   *db;
};

static gboolean
adb_db_setup(Adb *self);

static void
lomo_add_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);

Adb*
adb_new(LomoPlayer *lomo, GError **error)
{
	Adb *self;
	gchar   *db_path;
	sqlite3 *db = NULL;

	db_path = g_build_filename(g_get_home_dir(), ".eina", "adb.db", NULL);
	if (sqlite3_open(db_path, &db) != SQLITE_OK)
	{
		gel_error("Cannot open db: %s", sqlite3_errmsg(self->db));
		g_free(db_path);
		return FALSE;
	}
	g_free(db_path);

	self = g_new0(Adb, 1);
	self->db = db;
	adb_db_setup(self);
	g_signal_connect(lomo, "add", G_CALLBACK(lomo_add_cb), self);
	return self;
}

void
adb_free(Adb *self)
{
	sqlite3_close(self->db);
	g_free(self);
}

static gboolean
adb_db_setup(Adb *self)
{
	gchar *msg = NULL;
	sqlite3_stmt *stmt;
	const unsigned char *version_str = NULL;
	gint version = -1;

	if (sqlite3_prepare_v2(self->db, "SELECT value FROM variables WHERE key = 'schema-version'", -1, &stmt, NULL) != SQLITE_OK)
	{
		gel_error("Cannot check db schema version: %s", msg);
		g_free(msg);
	}
	if (stmt && (SQLITE_ROW == sqlite3_step(stmt)))
	{
		version_str = sqlite3_column_text(stmt, 0);
		if (version_str)
			version = atoi((gchar *) version_str);
	}
	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		gel_error("Cannot check db schema version: %s", msg);
		g_free(msg);
	}
	gel_warn("DB version: %d", version);
	adb_db_upgrade(self, version);
	return TRUE;
}

static void
lomo_add_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data)
{
	Adb *self = (Adb *) data;
	char *errmsg = NULL;

	gchar *uri = (gchar*) lomo_stream_get_tag(stream, LOMO_TAG_URI);
	gchar *q = sqlite3_mprintf(
		"INSERT OR REPLACE INTO streams VALUES("
			"(SELECT sid FROM streams WHERE uri='%q'),"
			"'%q',"
			"DATETIME('NOW'));",
			uri,
			uri);

	if (sqlite3_exec(self->db, q, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		gel_warn("Cannot insert %s into database: %s", lomo_stream_get_tag(stream, LOMO_TAG_URI), errmsg);
		sqlite3_free(errmsg);
	}
	sqlite3_free(q);
}

