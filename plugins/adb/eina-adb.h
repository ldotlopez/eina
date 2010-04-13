#ifndef _EINA_ADB
#define _EINA_ADB

#include <glib-object.h>
#include <lomo/lomo-player.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define EINA_TYPE_ADB eina_adb_get_type()

#define EINA_ADB(obj) \
	 (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ADB, EinaAdb))

#define EINA_ADB_CLASS(klass) \
	 (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ADB, EinaAdbClass))

#define EINA_IS_ADB(obj) \
	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ADB))

#define EINA_IS_ADB_CLASS(klass) \
	 (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ADB))

#define EINA_ADB_GET_CLASS(obj) \
	 (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ADB, EinaAdbClass))

typedef struct {
	GObject parent;
} EinaAdb;

typedef struct {
	GObjectClass parent_class;
} EinaAdbClass;

enum {
	EINA_ADB_NO_ERROR = 0,
	EINA_ADB_UPGRADE_ERROR_UNKNOW_SCHEMA_VERSION,
	EINA_ADB_UPGRADE_ERROR_TRANSACTION,
	EINA_ADB_UPGRADE_QUERY_FAILED
};

GType eina_adb_get_type (void);

EinaAdb* eina_adb_new (void);

LomoPlayer*
eina_adb_get_lomo_player(EinaAdb *self);
void
eina_adb_set_lomo_player(EinaAdb *self, LomoPlayer *lomo);

gchar *
eina_adb_get_db_file(EinaAdb *self);
gboolean
eina_adb_set_db_file(EinaAdb *self, const gchar *path);

// --
// Query queue
// --
void
eina_adb_queue_query(EinaAdb *self, char *query);
void
eina_adb_flush(EinaAdb *self);

// --
// Easy setup
// --

typedef sqlite3_stmt EinaAdbResult;

EinaAdbResult*
eina_adb_query(EinaAdb *self, gchar *query, ...);

gboolean
eina_adb_query_exec(EinaAdb *self, gchar *q, ...);

gboolean
eina_adb_result_step(EinaAdbResult *result);

gboolean
eina_adb_result_get(EinaAdbResult *result, ...);

void
eina_adb_result_free(EinaAdbResult *result);

//
gchar *
eina_adb_get_variable(EinaAdb *self, gchar *variable);
gboolean
eina_adb_set_variable(EinaAdb *self, gchar *variable, gchar *value);
gint
eina_adb_schema_get_version(EinaAdb *self, gchar *schema);
void
eina_adb_schema_set_version(EinaAdb *self, gchar *schema, gint version);

gboolean
eina_adb_upgrade_schema(EinaAdb *self, gchar *schema, gchar **queries[], GError **error);

G_END_DECLS

#endif /* _EINA_ADB */
