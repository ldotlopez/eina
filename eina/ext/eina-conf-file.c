/*
 * eina/ext/eina-conf-file.c
 *
 * Copyright (C) 2004-2010 Eina
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

#define GEL_DOMAIN "Eina::Conf::File"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <gel/gel.h>
#include <eina/ext/eina-conf.h>

// --
// Internal GValue
// --
static void
free_gvalue(GValue *value)
{
	g_value_unset(value);
	g_free(value);
}

// --
// EinaConf stuff
// --
G_DEFINE_TYPE (EinaConf, eina_conf, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_CONF, EinaConfPrivate))

typedef struct _EinaConfPrivate EinaConfPrivate;

struct _EinaConfPrivate {
	GHashTable *values;
	guint       timeout_id;
	gchar      *filename;
	guint       timeout;
	gint        io_fd;
};

// Signals
enum {
	CHANGE,
	LAST_SIGNAL
};
static guint eina_conf_signals[LAST_SIGNAL] = { 0 };

// Properties
enum {
	PROPERTY_SOURCE = 1,
	PROPERTY_TIMEOUT
};

static gboolean
eina_conf_dump(EinaConf *self);

static void
eina_conf_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaConf *self = EINA_CONF(object);
	switch (property_id)
	{

	case PROPERTY_SOURCE:
		g_value_set_string(value, (gpointer) eina_conf_get_source(self));
		break;

	case PROPERTY_TIMEOUT:
		g_value_set_uint(value, (guint) eina_conf_get_timeout(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_conf_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaConf *self = EINA_CONF(object);
	switch (property_id)
	{

	case PROPERTY_SOURCE:
		eina_conf_set_source(self, (gchar *) g_value_get_string(value));
		break;

	case PROPERTY_TIMEOUT:
		eina_conf_set_timeout(self, (guint) g_value_get_uint(value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_conf_dispose (GObject *object)
{
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

	G_OBJECT_CLASS (eina_conf_parent_class)->dispose (object);
}

static void
eina_conf_class_init (EinaConfClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaConfPrivate));

	object_class->get_property = eina_conf_get_property;
	object_class->set_property = eina_conf_set_property;
	object_class->dispose = eina_conf_dispose;

	g_object_class_install_property(object_class, PROPERTY_SOURCE,
		g_param_spec_string("source", "Source", "Source for configuration",
		NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROPERTY_TIMEOUT,
		g_param_spec_uint("timeout", "Timeout", "Delay between flushes", 0, G_MAXUINT, 50000,
		G_PARAM_READABLE | G_PARAM_WRITABLE));


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

static void
eina_conf_init (EinaConf *self)
{
	EinaConfPrivate *priv = GET_PRIVATE(self);
	priv->values = g_hash_table_new_full(
		g_str_hash, g_str_equal,
		g_free, (GDestroyNotify) free_gvalue);
	priv->timeout = 5000;
	priv->filename = NULL;
}

EinaConf*
eina_conf_new (void)
{
	return g_object_new (EINA_TYPE_CONF, NULL);
}

EinaConf*
eina_conf_get_default(void)
{
	static EinaConf *self = NULL;
	if (!self)
		self = eina_conf_new();
	return self;
}

void
eina_conf_set_source(EinaConf *self, gchar *source)
{
	EinaConfPrivate *priv = GET_PRIVATE(self);

	priv->filename = g_strdup(source);
}

G_CONST_RETURN gchar*
eina_conf_get_source(EinaConf *self)
{
	return GET_PRIVATE(self)->filename;
}

guint
eina_conf_get_timeout(EinaConf *self)
{
	return GET_PRIVATE(self)->timeout;
}

void
eina_conf_set_timeout(EinaConf *self, guint timeout)
{
	GET_PRIVATE(self)->timeout = timeout;
}

static void
dump_forearch_cb(gpointer _key, gpointer _value, gpointer data)
{
	EinaConf *self        = (EinaConf *) data;
	EinaConfPrivate *priv = GET_PRIVATE(self);
	gchar *key            = (gchar *) _key;
	GValue *value         = (GValue *) _value;
	gchar *buff = NULL;

	gchar *type_prefix = NULL;
	switch (G_VALUE_TYPE(value))
	{
	case G_TYPE_BOOLEAN:
		type_prefix="boolean";
		break;

	case G_TYPE_INT:
		type_prefix="int";
		break;

	case G_TYPE_UINT:
		type_prefix="uint";
		break;

	case G_TYPE_FLOAT:
		type_prefix="float";
		break;

	case G_TYPE_STRING:
		type_prefix="string";
		break;

	default:
		return;
		gel_warn("unknow key type '%s'\n", key);
	}

	gchar *tmp = ((G_VALUE_TYPE(value) == G_TYPE_STRING) ? g_strdup(g_value_get_string(value)) : g_strdup_value_contents(value));
	buff = g_strdup_printf("%s:%s=%s\n", type_prefix, key, tmp);
	g_free(tmp);

	if (buff != NULL)
	{
		if (write(priv->io_fd, buff, strlen(buff)) == -1)
			; // Avoid warning
		g_free(buff);
	}
}

gboolean
eina_conf_dump(EinaConf *self)
{
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
		dump_forearch_cb,
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

		if (g_str_equal(parts[0], "boolean") ||
			g_str_equal(parts[0], "bool")) {
			eina_conf_set_bool(self, parts[1], g_str_equal(parts[2], "TRUE") ? TRUE : FALSE);
		}
		else if (g_str_equal(parts[0], "int")) {
			eina_conf_set_int(self, parts[1], (strtol(parts[2], NULL, 0) / 1));
		}
		else if (g_str_equal(parts[0], "uint")) {
			eina_conf_set_uint(self, parts[1], (strtol(parts[2], NULL, 0) / 1));
		}
		else if (g_str_equal(parts[0], "float")) {
			eina_conf_set_float(self, parts[1], strtol(parts[2], NULL, 0));
		}
		else if (g_str_equal(parts[0], "string") ||
		         g_str_equal(parts[0], "str")) {
			eina_conf_set_str(self, parts[1], parts[2]);
		}

		g_strfreev(parts);
	}

	g_strfreev(lines);
}

void eina_conf_set(EinaConf *self, gchar *key, GValue *val) {
	EinaConfPrivate *priv = GET_PRIVATE(self);
	g_hash_table_replace(priv->values, g_strdup(key), val);

	if (priv->timeout_id)
		g_source_remove(priv->timeout_id);
	priv->timeout_id = g_timeout_add(priv->timeout, (GSourceFunc) eina_conf_dump, self);

	g_signal_emit(G_OBJECT(self), eina_conf_signals[CHANGE], 0, key);
}

void eina_conf_set_boolean(EinaConf *self, gchar *key, gboolean val) {
	GValue *_val = g_new0(GValue, 1);
	g_value_init(_val, G_TYPE_BOOLEAN);
	g_value_set_boolean(_val, val);

	eina_conf_set(self, key, _val);
}

void eina_conf_set_int(EinaConf *self, gchar *key, gint val) {
	GValue *_val = g_new0(GValue, 1);
	g_value_init(_val, G_TYPE_INT);
	g_value_set_int(_val, val);

	eina_conf_set(self, key, _val);
}

void eina_conf_set_uint(EinaConf *self, gchar *key, guint val) {
	GValue *_val = g_new0(GValue, 1);
	g_value_init(_val, G_TYPE_UINT);
	g_value_set_uint(_val, val);

	eina_conf_set(self, key, _val);
}

void eina_conf_set_float(EinaConf *self, gchar *key, gfloat val) {
	GValue *_val = g_new0(GValue, 1);
	g_value_init(_val, G_TYPE_FLOAT);
	g_value_set_float(_val, val);

	eina_conf_set(self, key, _val);
}

void eina_conf_set_string(EinaConf *self, gchar *key, gchar *val) {
	GValue *_val = g_new0(GValue, 1);
	g_value_init(_val, G_TYPE_STRING);
	g_value_set_string(_val, val);

	eina_conf_set(self, key, _val);
}

GValue *eina_conf_get(EinaConf *self, gchar *key)
{
	EinaConfPrivate *priv = GET_PRIVATE(self);
	return g_hash_table_lookup(priv->values, key);
}

gboolean eina_conf_get_boolean(EinaConf *self, gchar *key, gboolean def) {
	GValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (G_VALUE_TYPE(val) != G_TYPE_BOOLEAN)
		return def;
	return g_value_get_boolean(val);
}

gint eina_conf_get_int(EinaConf *self, gchar *key, gint def) {
	GValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (G_VALUE_TYPE(val) != G_TYPE_INT)
		return def;
	return g_value_get_int(val);
}

guint eina_conf_get_uint(EinaConf *self, gchar *key, guint def) {
	GValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (G_VALUE_TYPE(val) != G_TYPE_UINT)
		return def;
	return g_value_get_uint(val);
}


gfloat eina_conf_get_float(EinaConf *self, gchar *key, gfloat def) {
	GValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (G_VALUE_TYPE(val) != G_TYPE_FLOAT)
		return def;
	return g_value_get_float(val);
}

const gchar *eina_conf_get_string(EinaConf *self, gchar *key, const gchar *def) {
	GValue *val;
   
	if ((val = eina_conf_get(self, key)) == NULL)
		return def;
	if (G_VALUE_TYPE(val) != G_TYPE_STRING)
		return def;
	return g_value_get_string(val);
}

GList*
eina_conf_get_keys(EinaConf *self)
{
	return g_hash_table_get_keys(GET_PRIVATE(self)->values);
}

GType eina_conf_get_key_type(EinaConf *self, gchar *key)
{
	GValue *val = eina_conf_get(self, key);
	if (!val)
		return G_TYPE_INVALID;
	else
		return G_VALUE_TYPE(val);
}

gboolean
eina_conf_delete_key(EinaConf *self, gchar *key)
{
	return  g_hash_table_remove(GET_PRIVATE(self)->values, key);
}
