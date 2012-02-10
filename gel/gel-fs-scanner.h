#include <gio/gio.h>

#ifndef __GEL_FS_SCANNER_H__
#define __GEL_FS_SCANNER_H__

#define GEL_FS_SCANNER_INFO_KEY "x-gel-gfile-info"

typedef struct _GelFSScannerContext GelFSScannerContext;
typedef void (*GelFSScannerReadyFunc) (GList *result, gpointer user_data);

void gel_fs_scanner_scan(GList *file_objects, GCancellable *cancellable, GelFSScannerReadyFunc ready_callback,
	GCompareFunc compare_func,
	GSourceFunc  filter_func,
	gpointer user_data,
	GDestroyNotify notify);

gint gel_fs_scaner_compare_gfile_by_type_name(GFile *a, GFile *b);
gint gel_fs_scaner_compare_gfile_by_type     (GFile *a, GFile *b);
gint gel_fs_scaner_compare_gfile_by_name     (GFile *a, GFile *b);

#endif

