#ifndef _GEL_IO_SCANNER_H
#define _GEL_IO_SCANNER_H

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GelIOScanner GelIOScanner;

typedef void (*GelIOScannerSuccessFunc) (GelIOScanner *scanner, GList *results, gpointer data);
typedef void (*GelIOScannerErrorFunc)   (GelIOScanner *scanner, GFile *file, GError *error, gpointer data);

GelIOScanner*
gel_io_scan(GList *files, const gchar *attributes, gboolean recurse,
	GelIOScannerSuccessFunc success_cb, GelIOScannerErrorFunc error_cb, gpointer data);

void
gel_io_scan_close(GelIOScanner *scanner);

GList*
gel_io_scan_flatten_result(GList *forest);

G_END_DECLS

#endif
