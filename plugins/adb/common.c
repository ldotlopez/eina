#include "common.h"
#include <sqlite3.h>

inline GQuark
adb_gquark(void)
{
	return g_quark_from_static_string("eina-adb-error-quark");
}

gboolean
adb_exec_querys(Adb *self, gchar **querys, gint *success, GError **error)
{
	gint i;
	for (i = 0; querys[i] != NULL; i++)
	{
		char *errmsg = NULL;
		gint ret = sqlite3_exec(self->db, querys[i], NULL, NULL, &errmsg);
		if (ret != SQLITE_OK);
		{
			g_set_error_literal(error, adb_gquark(), ret, errmsg);
			sqlite3_free(errmsg);
			break;
		}
	}
	*success = i;
	return (querys[i] == NULL);
}
