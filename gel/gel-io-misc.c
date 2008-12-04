#include <gel/gel-io-misc.h>

GFile *
gel_io_file_get_child_for_file_info(GFile *parent, GFileInfo *child_info)
{
	const gchar *name;
	
	if ((name = g_file_info_get_name(child_info)) == NULL)
		return NULL;

	return g_file_get_child(parent, name);
}

