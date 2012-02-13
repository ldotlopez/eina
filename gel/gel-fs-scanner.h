/*
 * gel/gel-fs-scanner.h
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

#ifndef __GEL_FS_SCANNER_H__
#define __GEL_FS_SCANNER_H__

/**
 * GEL_FS_SCANNER_INFO_KEY:
 *
 * gel_fs_scanner_scan() and friends inserts a #GFileInfo object into every #GFile object they create.
 * You obtain that #GFileInfo using g_object_get_data(gfile, GEL_FS_SCANNER_INFO_KEY)
 */
#define GEL_FS_SCANNER_INFO_KEY "x-gel-gfile-info"

/**
 * GelFSScannerReadyFunc
 *
 * Prototype for the ready_func used in gel_fs_scanner_scan() family
 */
typedef void (*GelFSScannerReadyFunc) (GList *result, gpointer user_data);

void gel_fs_scanner_scan(GList *file_objects, GCancellable *cancellable,
	GelFSScannerReadyFunc ready_func,
	GCompareFunc          compare_func,
	GSourceFunc           filter_func,
	gpointer       user_data,
	GDestroyNotify notify);

void gel_fs_scanner_scan_uri_list(GList *uri_list, GCancellable *cancellable,
	GelFSScannerReadyFunc ready_func,
	GCompareFunc          compare_func,
	GSourceFunc           filter_func,
	gpointer       user_data,
	GDestroyNotify notify);

gint gel_fs_scaner_compare_gfile_by_type_name(GFile *a, GFile *b);
gint gel_fs_scaner_compare_gfile_by_type     (GFile *a, GFile *b);
gint gel_fs_scaner_compare_gfile_by_name     (GFile *a, GFile *b);

#endif

