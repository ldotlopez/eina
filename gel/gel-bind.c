/*
 * gel/gel-bind.c
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

#include "gel/gel-bind.h"
#include <glib/gi18n.h>
#include <glib/gprintf.h>

struct _GelObjectBind {
	GObject *src, *dst;
	GParamSpec *s_pspec, *d_pspec;
	GelObjectBindMappingFunc mapping;
};


static gboolean
gel_value_equal(GValue *a, GValue *b);


static void
object_bind_notify_cb(GObject *src, GParamSpec *s_pspec, GelObjectBind *data);
void
object_bind_weak_src_cb(GelObjectBind *bind, GObject *old_object);
void
object_bind_weak_dst_cb(GelObjectBind *bind, GObject *old_object);

static gboolean
gel_value_equal(GValue *a, GValue *b)
{
	g_return_val_if_fail(G_VALUE_TYPE(a) == G_VALUE_TYPE(b), FALSE);

	GType type = G_VALUE_TYPE(a);
	switch (type)
	{
	case G_TYPE_BOOLEAN:
		return g_value_get_boolean(a) == g_value_get_boolean(b);

	case G_TYPE_INT:
		return g_value_get_int(a) == g_value_get_int(b);

	case G_TYPE_FLOAT:
		return g_value_get_float(a) == g_value_get_float(b);

	case G_TYPE_DOUBLE:
		return g_value_get_double(a) == g_value_get_double(b);

	default:
		g_warning(N_("Unhandled type %s"), g_type_name(type));
		return FALSE;
	}
}

GelObjectBind*
gel_object_bind_with_mapping(GObject *src, gchar *s_prop, GObject *dst, gchar *d_prop, GelObjectBindMappingFunc mapping)
{
	g_return_val_if_fail(G_IS_OBJECT(src) && G_IS_OBJECT(dst), NULL);
	g_return_val_if_fail((s_prop != NULL) &&  (d_prop != NULL), NULL);

	GObjectClass *src_k = G_OBJECT_GET_CLASS(src);
	GObjectClass *dst_k = G_OBJECT_GET_CLASS(dst);

	GParamSpec *s_pspec = g_object_class_find_property(src_k, s_prop);
	GParamSpec *d_pspec = g_object_class_find_property(dst_k, d_prop);
	g_return_val_if_fail(s_pspec && d_pspec, NULL);

	if (mapping == NULL)
		g_return_val_if_fail(g_value_type_compatible(s_pspec->value_type, d_pspec->value_type), NULL);

	GelObjectBind *data = g_new0(GelObjectBind, 1);
	data->src = src;
	data->dst = dst;
	data->s_pspec = s_pspec;
	data->d_pspec = d_pspec;
	data->mapping = mapping;

	object_bind_notify_cb(src, s_pspec, data);

	gchar *detail = g_strconcat("notify::", s_prop, NULL);
	g_signal_connect((GObject *) src, detail, (GCallback) object_bind_notify_cb, data);
	g_free(detail);

	g_object_weak_ref(src, (GWeakNotify) object_bind_weak_src_cb, data);
	g_object_weak_ref(dst, (GWeakNotify) object_bind_weak_dst_cb, data);

	return data;
}

void
gel_object_unbind(GelObjectBind *bind)
{
}

static void
object_bind_notify_cb(GObject *src, GParamSpec *s_pspec, GelObjectBind *data)
{
	const gchar *s_prop, *d_prop;
	s_prop = g_param_spec_get_name(data->s_pspec);
	d_prop = g_param_spec_get_name(data->d_pspec);
	g_return_if_fail(g_str_equal(s_prop, g_param_spec_get_name(s_pspec)));

	g_printf("Transfer prop %p:'%s' -> %p:'%s'\n", data->src, s_prop, data->dst, d_prop);

	// Create the new GValue
	GValue src_v = {0};
	GValue dst_v = {0};
	GValue new_v = {0};
	g_value_init(&src_v, data->s_pspec->value_type);
	g_value_init(&dst_v, data->d_pspec->value_type);
	g_value_init(&new_v, data->d_pspec->value_type);

	// Get current values
	g_object_get_property(data->src, s_prop, &src_v);
	g_object_get_property(data->dst, d_prop, &dst_v);

	if (data->mapping)
	{
		g_return_if_fail(data->mapping(&src_v, &new_v));
	}
	else
	{	
		if (data->s_pspec->value_type == data->d_pspec->value_type)
		{
			g_printf("Copy\n");
			g_value_copy(&src_v, &new_v);
		}
		else
		{
			g_printf("Transform\n");
			g_value_transform(&src_v, &new_v);
		}
	}

	if (!gel_value_equal(&new_v, &dst_v))
	{
		g_printf("Update %p:%s -> %s\n", data->dst, d_prop, g_strdup_value_contents(&new_v));
		g_object_set_property(data->dst, d_prop, &new_v);
	}
	g_value_unset(&src_v);
	g_value_unset(&dst_v);
	g_value_unset(&new_v);
}

void
object_bind_weak_src_cb(GelObjectBind *bind, GObject *old_object)
{
	g_printf("Object:src %p is going to be destroy, unbind automatically\n", old_object);
	g_object_weak_unref(bind->src, (GWeakNotify) object_bind_weak_src_cb, bind);
	g_object_weak_unref(bind->dst, (GWeakNotify) object_bind_weak_dst_cb, bind);
	g_free(bind);
}

void
object_bind_weak_dst_cb(GelObjectBind *bind, GObject *old_object)
{
	g_printf("Object:dst %p is going to be destroy, unbind automatically\n", old_object);

	gchar *detail = g_strconcat("notify::", g_param_spec_get_name(bind->s_pspec), NULL);
	g_signal_handlers_disconnect_by_func(bind->src, object_bind_notify_cb, bind);
	g_free(detail);

	g_object_weak_unref(bind->src, (GWeakNotify) object_bind_weak_src_cb, bind);
	g_object_weak_unref(bind->dst, (GWeakNotify) object_bind_weak_dst_cb, bind);

	g_free(bind);
}


