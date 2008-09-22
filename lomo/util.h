#ifndef __LOMO_UTIL
#define __LOMO_UTIL

#include <gst/gst.h>
#include "player.h"

G_BEGIN_DECLS

#define lomo_nanosecs_to_secs(x) ((gint64)(x/1000000000L))
#define lomo_secs_to_nanosecs(x) ((gint64)(x*1000000000L))

gboolean lomo_format_to_gst(LomoFormat format, GstFormat *gst_format);
gboolean lomo_state_to_gst(LomoState state, GstState *gst_state);
gboolean lomo_state_from_gst(GstState state, LomoState *lomo_state);
GstStateChangeReturn lomo_state_change_return_to_gst(GstStateChangeReturn ret);

const gchar *gst_state_to_str(GstState state);

G_END_DECLS

#endif // _LOMO_UTL
