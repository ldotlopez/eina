/*
 * peasdemo-hello-world-plugin.h
 * This file is part of libpeas
 *
 * Copyright (C) 2009-2010 Steve Frécinaux
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EINA_LOMO_PLUGIN_H__
#define __EINA_LOMO_PLUGIN_H__

#include <libpeas/peas.h>

G_BEGIN_DECLS

#define EINA_TYPE_LOMO_PLUGIN         (eina_lomo_plugin_get_type ())
#define EINA_LOMO_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_LOMO_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_IS_LOMO_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_LOMO_PLUGIN))
#define EINA_IS_LOMO_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EINA_TYPE_LOMO_PLUGIN))
#define EINA_LOMO_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EINA_TYPE_LOMO_PLUGIN, EinaLomoPluginClass))

typedef struct _EinaLomoPlugin       EinaLomoPlugin;
typedef struct _EinaLomoPluginClass  EinaLomoPluginClass;

struct _EinaLomoPlugin {
  PeasExtensionBase parent_instance;
};

struct _EinaLomoPluginClass {
  PeasExtensionBaseClass parent_class;
};

GType                 eina_lomo_plugin_get_type        (void) G_GNUC_CONST;
G_MODULE_EXPORT void  peas_register_types                         (PeasObjectModule *module);

G_END_DECLS

#endif /* __EINA_LOMO_PLUGIN_H__ */
