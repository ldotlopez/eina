#include <gst/gst.h>
#include "util.h"

gboolean
lomo_format_to_gst(LomoFormat format, GstFormat *gst_format)
{
	switch (format)
	{
	case LOMO_FORMAT_TIME:
		*gst_format = GST_FORMAT_TIME;
		break;

	case LOMO_FORMAT_PERCENT:
		*gst_format = GST_FORMAT_PERCENT;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

gboolean
lomo_state_to_gst(LomoState state, GstState *gst_state)
{
	switch (state)
	{
	case LOMO_STATE_PLAY:
		*gst_state = GST_STATE_PLAYING;
		break;

	case LOMO_STATE_PAUSE:
		*gst_state = GST_STATE_PAUSED;
		break;

	case LOMO_STATE_STOP:
		*gst_state = GST_STATE_READY;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

gboolean
lomo_state_from_gst(GstState state, LomoState *lomo_state)
{
	switch (state) {
	case GST_STATE_VOID_PENDING:
	case GST_STATE_NULL:
	case GST_STATE_READY:
		*lomo_state = LOMO_STATE_STOP;
		break;

	case GST_STATE_PAUSED:
		*lomo_state = LOMO_STATE_PAUSE;
		break;

	case GST_STATE_PLAYING:
		*lomo_state = LOMO_STATE_PLAY;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

const gchar *
gst_state_to_str(GstState state)
{
	switch (state)
	{
	case GST_STATE_VOID_PENDING:
		return "no pending state";
	case GST_STATE_NULL:
		return "NULL state or initial state of an element";
	case GST_STATE_READY:
		return "the element is ready to go to PAUSED";
	case GST_STATE_PAUSED:
		return "the element is PAUSED";
	case GST_STATE_PLAYING:
		return "the element is PLAYING";
	}
	return NULL;
}

