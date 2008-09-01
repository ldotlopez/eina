#define GEL_DOMAIN "Eina::Plugin::ADB"
#include <sqlite3.h>
#include <glib.h>
#include <gel/gel-ui.h>
#include "base.h"
#include "adb.h"

/* * * * * * * * * * */
/* Define ourselves  */
/* * * * * * * * * * */
struct _EinaAdb {
	EinaBase   parent;
	sqlite3   *db;
};

/* * * * * * * * * */
/* Lomo Callbacks */
/* * * * * * * * * */
/* Add here callback prototipes for LomoPlayer events: */
/* void on_adb_lomo_something(LomoPlayer *lomo, ..., EinaAdb *self); */

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
/* Add here callback prototipes por UI widgets */
/* void on_adb_name_type_event(GtkWidget *w, ..., EinaAdb *self); */

/* * * * * * * * * * * */
/* Signal definitions  */
/* * * * * * * * * * * */
GelUISignalDef _adb_signals[] = {
	/* { "nameType", "something", G_CALLBACK(on_adb_lomo_something) }, */
	GEL_UI_SIGNAL_DEF_NONE
};

void adb_create_db(EinaAdb *self) {
	static const gchar schema[] =
			"CREATE TABLE variables ("
			"key VARCHAR(30) PRIMARY KEY,"
			"value VARCHAR(30)"
			");"

			"CREATE TABLE history ("
			"uid INTEGER PRIMARY KEY AUTOINCREMENT,"
			"uri VARCHAR(256),"
			"timestamp TIMESTAMP"
			");";
	
	static const gchar *data   = "INSERT INTO variables (key,value) VALUES ('version','1');";
	sqlite3_exec(self->db, schema, NULL, 0, NULL);
	sqlite3_exec(self->db, data,   NULL, 0, NULL);
}

gint _adb_check_db_cb (void *self_, int argc, char **argv, char **azColName) {
	if ((argv[0] == NULL) || !g_str_equal(argv[0], "1")) {
		gel_error("Error checking DB Schema version");
		return 0;
	}

	return 0;
}

void adb_check_db(EinaAdb *self) {
	gint rc;
	gchar *err = NULL;
	static const gchar *query = "SELECT value FROM variables WHERE key='version'";

	rc = sqlite3_exec(self->db, query, _adb_check_db_cb, self, &err); 
	if (rc != SQLITE_OK) {
		gel_error("SQL error [%d]: %s\n", rc, err);
		sqlite3_free(err);
	}
}

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean adb_init(GelHub *hub, gint argc, gchar *argv[]) {
	EinaAdb *self;
	gchar   *db_path;
	
	/*
	 * Create mem in hub (if needed)
	 */
	self = g_new0(EinaAdb, 1);
	if(!eina_base_init((EinaBase *) self, hub, "adb", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	db_path = g_build_filename(g_get_home_dir(), ".eina", "adb.db", NULL);
	if (sqlite3_open(db_path, &(self->db)) != SQLITE_OK) {
		gel_error("Cannot open db: %s", sqlite3_errmsg(self->db));
		g_free(db_path);
		return FALSE;
	}
	g_free(db_path);

	/* Check if is a new/upgade/correct version db */
	adb_create_db(self);
	adb_check_db(self);
	
	/* At this point:
	 * - adb.glade was automagicaly loaded if exists
	 * - You have 4 macros to access resources:
	 *   LOMO(self):   access to LomoPlayer
	 *   HUB(self):    access the EinaHub
	 *   UI(self):     access GladeXML from adb.glade
	 *   W(self,name): returns the GtkWidget named 'name' from adb.glade
	 */

	/*
	 * Build some no-glade UI widgets here
	 */
	
	/*
	 * Connect signals for previously created widgets
	 */
	/* glade_ext_multiple_signal_connect_from_def(UI(self), _adb_signals, self); */
	/* g_signal_connect(self->some_custom_widget, "signal", G_CALLBACK(on_adb_widget_type_signal), self); */

	/*
	 * Connect signals for LomoPlayer
	 */
	/* g_signal_connect(self->lomo, "something", G_CALLBACK(on_adb_lomo_something), self); */
	/* g_signal_connect(self->lomo, "something", G_CALLBACK(on_adb_lomo_something), self); */

	/*
	 * Final setup and initial show
	 */
	/* gtk_widget_show(W(self->ui, "mainWindow")); */

	return TRUE;
}

G_MODULE_EXPORT gboolean adb_exit(EinaAdb *self) {
	sqlite3_close(self->db);
	eina_base_fini((EinaBase *) self);
	return TRUE;
}

/* * * * * * * * * * * * * */
/* Implement UI Callbacks  */
/* * * * * * * * * * * * * */

/* * * * * * * * * * ** * * */
/* Implement Lomo Callbacks */
/* * * * * * * * * * ** * * */

/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GelHubSlave adb_connector = {
	"adb",
	(GelHubSlaveInitFunc) &adb_init,
	(GelHubSlaveExitFunc) &adb_exit
};

