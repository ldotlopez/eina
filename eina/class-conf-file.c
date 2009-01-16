/*
 * eina/class-conf-file.c
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define GEL_DOMAIN "Class::Conf::File"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <gel/gel.h>
#include "class-conf-file.h"

#define CLASS_CONF_FILE_TIMEOUT 5000
#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_CONF, EinaConfPrivate))

static gboolean
eina_conf_dump(EinaConf *self);

/*
 * Value handling
 */
typedef struct EinaConfValue
{
	GType type;
	union {
		gboolean  _bool;
		gint      _int;
		gfloat    _float;
		gchar    *_str;
	} value;
} EinaConfValue;

void eina_conf_value_free(gpointer value) {
	EinaConfValue *val = (EinaConfValue *) value;

	if (val->type == G_TYPE_STRING)
		g_free(val->value._str);
	g_free(val);
}

/*
 * Implementation of the klass
 */
G_DEFINE_TYPE (EinaConf, eina_conf, G_TYPE_OBJECT);

typedef struct _EinaConfPrivate EinaConfPrivate;
struct _EinaConfPrivate {
	GHashTable *values;
	guint       timeout_id;
	gchar      *filename;
	gint        io_fd;
};

/* Signals */
enum {
	CHANGE,
	LAST_SIGNAL
};
static guint eina_conf_signals[LAST_SIGNAL] = { 0 };

/* Dispose method */
static void eina_conf_dispose(GObject *object) {
	EinaConf *self = EINA_CONF(object);
	EinaConfPrivate *priv = GET_PRIVATE(self);

	if (priv->timeout_id > 0)
	{
		g_source_remove(priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (priv->values)
	{
		eina_conf_dump(self);
		g_hash_table_destroy(priv->values);
		priv->values = NULL;
		g_free(priv->filename);
	}

	G_OBJECT_CLASS (eina_conf_parent_class)->dispose(object);
}

static void eina_conf_class_init (EinaConfClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eina_conf_dispose;

	g_type_class_add_private (klass, sizeof (EinaConfPrivate));

	eina_conf_signals[CHANGE] = g_signal_new ("change",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaConfClass, change),
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE,
		1,
		G_TYPE_POINTER);
}

static void eina_conf_init (EinaConf *self) {
	EinaConfPrivate *priv = GET_PRIVATE(self);
	priv->values = g_hash_table_new_full(
		g_str_hash, g_str_equal,
		g_free, eina_conf_value_free);
}

EinaConf *eina_conf_new (void) {
	return g_object_new (EINA_TYPE_CONF, NULL);
}

void eina_conf_set_filename(EinaConf *self, gchar *filename) {
	EinaConfPrivate *priv = GET_PRIVATE(self);

	priv->filename = g_strdup(filename);
}

const gchar *eina_conf_get_filename(EinaConf *self) {
	EinaConfPrivate *priv = GET_PRIVATE(self);

	return priv->filename;
}

void _eina_conf_dump_foreach_cb(gpointer _key, gpointer _value, gpointer data) {
	EinaConf *self       = (EinaConf *) data;
	EinaConfPrivate *priv = GET_PRIVATE(self);
	gchar *key           = (gchar *) _key;
	EinaConfValue *value = (EinaConfValue *) _value;
	gchar *buff = NULL;

	switch (value->type)
	{
		case G_TYPE_BOOLEAN:
			buff = g_strdup_printf("bool:%s=%d\n", key, value->value._bool);
			break;

		case G_TYPE_INT:
			buff = g_strdup_printf("int:%s=%d\n", key, value->value._int);
			break;

		case G_TYPE_FLOAT:
			buff = g_strdup_printf("float:%s=%f\n", key, value->value._float);
			break;

		case G_TYPE_STRING:
			buff = g_strdup_printf("str:%s=%s\n", key, value->value._str);
			break;
			
		default:
			gel_warn("unknow key type '%s'\n", key);
	}

	if (buff != NULL)
	{
		if (write(priv->io_fd, buff, strlen(buff)) == -1)
			; // Avoid warning
		g_free(buff);
	}
}

gboolean eina_conf_dump(EinaConf *self) {
	EinaConfPrivate *priv = GET_PRIVATE(self);
	priv->io_fd = g_open(
		priv->filename,
		O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
	
	if (priv->io_fd == -1) {
		gel_error("Error while opening '%s' for writing",
			priv->filename);
		return FALSE;
	}
	
	g_hash_table_foreach(priv->values,
		_eina_conf_dump_foreach_cb,
		self);
	close(priv->io_fd);

	return FALSE;
}

void eina_conf_load(EinaConf *self) {
	gchar *buff = NULL;
	gchar **lines;
	gchar **parts;
	gint i;
	GError *error = NULL;
	
	EinaConfPrivate *priv = GET_PRIVATE(self);
	if (!g_file_get_contents(priv->filename, &buff, NULL, &error)) {
		gel_error("Error loading '%s': %s\n", priv->filename, error->message);
	   	g_error_free(error);
		return;
	}

	lines = g_strsplit_set(buff, "\r\n", 0);
	g_free(buff);

	for (i = 0; lines[i] != NULL; i++) {
		parts = g_strsplit_set(lines[i], ":=", 3);
		if (
			(parts[0] == NULL) ||
			(parts[1] == NULL) ||
			(parts[2] == NULL)) {
			g_strfreev(parts);
			continue;
		}

		if (g_str_equal(parts[0], "bool")) {
			eina_conf_set_bool(self, parts[1], (strtol(parts[2], NULL, 0) /1));
		}
		else if (g_str_equal(parts[0], "int")) {
			eina_conf_set_int(self, parts[1], (strtol(parts[2], NULL, 0) / 1));
		}
		else if (g_str_equal(parts[0], "float")) {
			eina_conf_set_float(self, parts[1], strtol(parts[2], NULL, 0));
		}
		else if (g_str_equal(parts[0], "str")) {
			eina_conf_set_str(self, parts[1], parts[2]);
		}

		g_strfreev(parts);
	}

	g_strfreev(lines);
}

void eina_conf_set(EinaConf *self, gchar *key, EinaConfValue *val) {
	EinaConfPrivate *priv = GET_PRIVATE(self);
	g_hash_table_replace(priv->values, g_strdup(key), val);

	if (priv->timeout_id)
		g_source_remove(priv->timeout_id);
	priv->timeout_id = g_timeout_add(CLASS_CONF_FILE_TIMEOUT, (GSourceFunc) eina_conf_dump, self);

	g_signal_emit(G_OBJECT(self), eina_conf_signals[CHANGE], 0, key);
}

void eina_conf_set_bool(EinaConf *self, gchar *key, gboolean val) {
	EinaConfValue *_val = g_new0(EinaConfValue, 1);
	_val->type = G_TYPE_BOOLEAN;
	_val->value._bool = val;

	eina_conf_set(self, key, _val);
}

void eina_conf_set_int(EinaConf *self, gchar *key, gint val) {
	EinaConfValue *_val = g_new0(EinaConfValue, 1);
	_val->type = G_TYPE_INT;
	_val->value._int = val;

	eina_conf_set(self, key, _val);
}

void eina_conf_set_float(EinaConf *self, gchar *key, gfloat val) {
	EinaConfValue *_val = g_new0(EinaConfValue, 1);
	_val->type = G_TYPE_FLOAT;
	_val->value._float = val;

	eina_conf_set(self, key, _val);
}

void eina_conf_set_str(EinaConf *self, gchar *key, gchar *val) {
	EinaConfValue *_val = g_new0(EinaConfValue, 1);
	_val->type = G_TYPE_STRING;
	_val->value._str = g_strdup(val);

	eina_conf_set(self, key, _val);
}

EinaConfValue *eina_conf_get(EinaConf *self, gchar *key) {
	EinaConfPrivate *priv = GET_PRIVATE(self);
	return g_hash_table_lookup(priv->values, key);
}

gboolean eina_conf_get_bool(EinaConf *self, gchar *key, gboolean def) {
	EinaConfValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (val->type != G_TYPE_BOOLEAN)
		return def;
	return val->value._bool;
}

gint eina_conf_get_int(EinaConf *self, gchar *key, gint def) {
	EinaConfValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (val->type != G_TYPE_INT)
		return def;
	return val->value._int;
}

gfloat eina_conf_get_float(EinaConf *self, gchar *key, gfloat def) {
	EinaConfValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (val->type != G_TYPE_FLOAT)
		return def;
	return val->value._float;
}

const gchar *eina_conf_get_str(EinaConf *self, gchar *key, const gchar *def) {
	EinaConfValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (val->type != G_TYPE_STRING)
		return def;
	return val->value._str;
}

gboolean
eina_conf_delete_key(EinaConf *self, gchar *key)
{
	return  g_hash_table_remove(GET_PRIVATE(self)->values, key);
}
