/*
 * peasdemo-hello-world-plugin.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libpeas/peas.h>
#include <eina/ext/eina-activatable.h>
#include <eina/ext/eina-extension.h>
#include "plugin.h"

EINA_DEFINE_EXTENSION(EinaLomoPlugin, eina_lomo_plugin, EINA_TYPE_LOMO_PLUGIN)

static void
eina_lomo_plugin_activate (EinaActivatable *activatable, EinaApplication *application)
{
	g_warning (G_STRFUNC);
}

static void
eina_lomo_plugin_deactivate (EinaActivatable *activatable, EinaApplication *application)
{
	g_warning (G_STRFUNC);
}

