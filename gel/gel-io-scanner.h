/*
 * gel/gel-io-scanner.h
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


#ifndef _GEL_IO_SCANNER
#define _GEL_IO_SCANNER

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GEL_IO_TYPE_SCANNER gel_io_scanner_get_type()

#define GEL_IO_SCANNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_IO_TYPE_SCANNER, GelIOScanner))
#define GEL_IO_SCANNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEL_IO_TYPE_SCANNER, GelIOScannerClass))
#define GEL_IO_IS_SCANNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_IO_TYPE_SCANNER))
#define GEL_IO_IS_SCANNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEL_IO_TYPE_SCANNER))
#define GEL_IO_SCANNER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEL_IO_TYPE_SCANNER, GelIOScannerClass))

typedef struct _GelIOScannerPrivate GelIOScannerPrivate;
typedef struct {
	GObject parent;
	GelIOScannerPrivate *priv;
} GelIOScanner;

typedef struct {
	GObjectClass parent_class;
	void (*finish) (GelIOScanner *self, GList *forest);
	void (*error)  (GelIOScanner *self, GFile *source, GError *error);
	void (*cancel) (GelIOScanner *self);
} GelIOScannerClass;

GType gel_io_scanner_get_type (void);

GelIOScanner* gel_io_scanner_new (void);
GelIOScanner* gel_io_scanner_new_full (GList *uris, const gchar *attributes, gboolean recurse);
void          gel_io_scanner_scan(GelIOScanner *self, GList *uris, const gchar *attributes, gboolean recurse);

GList*        gel_io_scanner_flatten_result(GList *forest);

G_END_DECLS

#endif /* _GEL_IO_SCANNER */
