/*
 * gel/gel-io-resources.h
 *
 * Copyright (C) 2004-2011 Eina
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

#include <gio/gio.h>

#ifndef _GEL_IO_RESOURCES
#define _GEL_IO_RESOURCES

gboolean gel_io_resources_load_file_contents(const gchar *path, gchar **contents, gsize *length, GError **error);

#define gel_io_resources_load_file_contents_or_error(path,contents,length) \
	G_STMT_START { \
		GError *error = NULL; \
		if (!gel_io_resources_load_file_contents(path,contents,length,&error)) \
			g_error("Can't load file contents of '%s': %s", path, error->message); \
	} G_STMT_END

#endif

