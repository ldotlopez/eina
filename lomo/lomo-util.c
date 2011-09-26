/*
 * lomo/lomo-util.c
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

#include "lomo-util.h"

#include <glib/gi18n.h>
#include <gst/gst.h>

/**
 * lomo_init:
 * @argc: (inout) (transfer none): argc from main()
 * @argv: (inout) (transfer none): argv from main()
 *
 * Initializes liblomo
 **/
void lomo_init(gint* argc, gchar ***argv)
{
	gst_init(argc, argv);
}

/**
 * lomo_format_to_gst:
 * @format: input format
 * @gst_format: (out): ouput format
 *
 * Converts format from liblomo definitions to gstreamer ones
 *
 * Returns: %TRUE if conversion was done, %FALSE otherwise
 */
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

/**
 * lomo_state_to_gst:
 * @state: input state
 * @gst_state: (out): ouput state
 *
 * Converts state from liblomo definitions to gstreamer ones
 *
 * Returns: %TRUE if conversion was done, %FALSE otherwise
 */
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

/**
 * lomo_state_from_gst:
 * @state: input state
 * @lomo_state: (out): ouput state
 *
 * Converts state from gst definitions to lomo ones
 *
 * Returns: %TRUE if conversion was done, %FALSE otherwise
 */
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

/**
 * lomo_state_to_str:
 * @state: A #LomoState
 *
 * Returns the string corresponding to the @state
 *
 * Returns: The string. This string is owner by liblomo and should not be
 *          freeded
 **/
const gchar*
lomo_state_to_str(LomoState state)
{
	switch (state)
	{
	case LOMO_STATE_INVALID:
		return G_STRINGIFY(LOMO_STATE_INVALID);
	case LOMO_STATE_STOP:
		return G_STRINGIFY(LOMO_STATE_STOP);
	case LOMO_STATE_PLAY:
		return G_STRINGIFY(LOMO_STATE_PLAY);
	case LOMO_STATE_PAUSE:
		return G_STRINGIFY(LOMO_STATE_PAUSE);
	default:
		return NULL;
	}
}

/**
 * gst_state_to_str: (skip):
 * @state: #GstState state
 *
 * Returns the string corresponding to the @state
 *
 * Returns: The string. This string is owner by liblomo and should not be
 *          freeded
 **/
const gchar *
gst_state_to_str(GstState state)
{
	switch (state)
	{
	case GST_STATE_VOID_PENDING:
		return "GST_STATE_VOID_PENDING";
	case GST_STATE_NULL:
		return "GST_STATE_NULL";
	case GST_STATE_READY:
		return "GST_STATE_READY";
	case GST_STATE_PAUSED:
		return "GST_STATE_PAUSED";
	case GST_STATE_PLAYING:
		return "GST_STATE_PLAYING";
	}
	return NULL;
}

/**
 * gst_state_change_return_to_str: (skip):
 * @s: #GstStateChangeReturn state
 *
 * Returns the string corresponding to the @s
 *
 * Returns: The string. This string is owner by liblomo and should not be
 *          freeded
 **/
const gchar*
gst_state_change_return_to_str(GstStateChangeReturn s)
{
	switch(s)
	{
	case GST_STATE_CHANGE_SUCCESS:
		return "GST_STATE_CHANGE_SUCCESS";
	case GST_STATE_CHANGE_NO_PREROLL:
		return "GST_STATE_CHANGE_NO_PREROLL";
	case GST_STATE_CHANGE_ASYNC:
		return "GST_STATE_CHANGE_ASYNC";
	case GST_STATE_CHANGE_FAILURE:
		return "GST_STATE_CHANGE_FAILURE";
	}
	return NULL;
}

/**
 * lomo_create_uri:
 * @str: A filepath (relative or absolute) or an URI
 *
 * Creates an uri from @str
 *
 * Returns: (transfer full): a newly allocated uri for @str.
 **/
gchar *
lomo_create_uri(gchar *str)
{
	gchar *tmp, *tmp2;

	// Valid URI
	if ((tmp = g_uri_parse_scheme(str)) != NULL)
	{
		g_free(tmp);
		return g_strdup(str);
	}

	// Absolute URI
	if (g_path_is_absolute(str))
	{
		// Return URI even if this fails
		return g_filename_to_uri(str, NULL, NULL);
	}

	// Relative URI: create an absolute path and convert it to URI
	tmp = g_get_current_dir();
	tmp2 = g_build_filename(tmp, str, NULL);
	g_free(tmp);

	tmp = g_filename_to_uri(tmp2, NULL, NULL);
	g_free(tmp2);

	return tmp;
}

