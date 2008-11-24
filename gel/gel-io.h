#include <gio/gio.h>
#include <gel/gel-io-recurse-tree.h>
#include <gel/gel-io-simple-ops.h>

GFile *gel_io_file_get_child_for_file_info(GFile *parent, GFileInfo *child_info);

#ifndef GEL_IO_DISABLE_DEPRECATED
#include "gel-io-async-read-op.h"
#include "gel-io-async-read-dir.h"
#endif
