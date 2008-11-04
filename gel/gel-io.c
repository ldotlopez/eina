#include <gel/gel-io.h>

GFile *
gel_io_file_get_child_for_file_info(GFile *parent, GFileInfo *child_info)
{
	GFile *child;
	gchar *name;
	
	if ((name = g_file_info_get_attribute_as_string(G_FILE_INFO(child_info), G_FILE_ATTRIBUTE_STANDARD_NAME)) == NULL)
		return NULL;

	child = g_file_get_child(parent, name);
	g_free(name);

	return child;
}

