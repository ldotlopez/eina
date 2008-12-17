#define GEL_DOMAIN "Eina::ADB:Common"

#include "common.h"
#include <sqlite3.h>
#include <gel/gel.h>

GQuark
adb_gquark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-adb-error-quark");
	return ret;
}

gboolean
adb_exec_querys(Adb *self, const gchar **querys, gint *success, GError **error)
{
	gint i;
	for (i = 0; querys[i] != NULL; i++)
	{
		char *errmsg = NULL;
		gint ret = sqlite3_exec(self->db, querys[i], NULL, NULL, &errmsg);
		if (ret != SQLITE_OK)
		{
			gel_error("ADB got error %d: %s. Query: %s", ret, errmsg, querys[i]);
			g_set_error_literal(error, adb_gquark(), ret, errmsg);
			sqlite3_free(errmsg);
			break;
		}
	}
	if (success)
		*success = i;
	return (querys[i] == NULL);
}

gboolean
adb_set_variable(Adb *self, const gchar *variable, GType type, gpointer value)
{
	gchar *fmt = NULL;
	gboolean deference = TRUE;
	switch (type)
	{
	case G_TYPE_BOOLEAN:
	case G_TYPE_UINT:
	case G_TYPE_INT:
		fmt = "%d";
		break;
	case G_TYPE_STRING:
		deference = FALSE;
		fmt = "%s";
		break;
	case G_TYPE_CHAR:
		fmt = "%c";
		break;
	case G_TYPE_FLOAT:
		fmt = "%f";
		break;
	default:
		return FALSE;
	}
	gchar *query = g_strdup_printf("UPDATE variables set value='%s' WHERE key='%%s'", fmt);
	gchar *q;
	if (deference)
		q = g_strdup_printf(query, value, *variable);
	else
		q = g_strdup_printf(query, value, variable);
	g_free(query);

	char *error = NULL;
	gboolean ret = (sqlite3_exec(self->db, q, NULL, NULL, &error) == SQLITE_OK);
	if (!ret)
	{
		gel_error("Cannot update variable %s: %s. Query: %s", variable, error, q);
		sqlite3_free(error);
	}
	g_free(q);
	return ret;
}
