#ifndef _GEL_IO_MISC
#define _GEL_IO_MISC

#include <gio/gio.h>

G_BEGIN_DECLS

GFile *
gel_io_file_get_child_for_file_info(GFile *parent, GFileInfo *child_info);

G_END_DECLS

#endif
