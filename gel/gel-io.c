#include <gel/gel-io.h>

GFile *
gel_io_file_get_child_for_file_info(GFile *parent, GFileInfo *child_info)
{
	GFile *child;
	const gchar *name;
	
	if ((name = g_file_info_get_name(child_info)) == NULL)
		return NULL;

	child = g_file_get_child(parent, name);

	return child;
}

