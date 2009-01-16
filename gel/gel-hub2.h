/*
 * gel/gel-hub2.h
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

#ifndef _GEL_HUB
#define _GEL_HUB

#include <glib-object.h>

G_BEGIN_DECLS

#define GEL_TYPE_HUB gel_hub_get_type()

#define GEL_HUB(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_TYPE_HUB, GelHub))

#define GEL_HUB_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GEL_TYPE_HUB, GelHubClass))

#define GEL_IS_HUB(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_TYPE_HUB))

#define GEL_IS_HUB_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_TYPE_HUB))

#define GEL_HUB_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_TYPE_HUB, GelHubClass))

typedef struct {
	GObject parent;
} GelHub;

typedef struct {
	GObjectClass parent_class;

	// Signals
	void (*module_load)   (GelHub *self, const gchar *name);
	void (*module_unload) (GelHub *self, const gchar *name);
	void (*module_init)   (GelHub *self, const gchar *name);
	void (*module_fini)   (GelHub *self, const gchar *name);
	void (*module_ref)    (GelHub *self, const gchar *name, guint refs);
	void (*module_unref)  (GelHub *self, const gchar *name, guint refs);
} GelHubClass;

typedef gboolean (*GelHubModuleInitFunc) (GelHub *hub);
typedef gboolean (*GelHubModuleFiniFunc) (GelHub *hub);

typedef struct _GelHubModulePrivate GelHubModulePrivate;
typedef struct {
	const gchar *name;
	GelHubModuleInitFunc init;
	GelHubModuleFiniFunc fini;
	GelHubModulePrivate *priv;
} GelHubModule;

GType gel_hub_get_type (void);

GelHub* gel_hub_new (void);

#define gel_hub_is_loaded(hub,pathname) (gel_hub_lookup_module_by_path(hub,pathname) != NULL)

GelHubModule *
gel_hub_lookup_module_by_path(GelHub *hub, const gchar *pathname);

G_END_DECLS

#endif /* _GEL_HUB */
