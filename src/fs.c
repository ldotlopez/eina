#define GEL_DOMAIN "Eina::Fs"

#include <gel/gel.h>
#include "fs.h"

/* Functions to feed liblomo */
void
eina_fs_lomo_feed_uri_multi(LomoPlayer *lomo, GList *uris , EinaFsFilterFunc filter, GCallback callback, gpointer user_data)
{
	// GIO related 
	GFile *f = NULL;
	GFileInfo *f_info = NULL;
	GFileType  f_type;
	GError *err = NULL;

	// Iterators
	GList *l = uris;
	GList *files = NULL, *directories = NULL;

	while (l)
	{
		// Fetch info
		f = g_file_new_for_uri((const gchar *) l->data);
		f_info = g_file_query_info(f, "standard::*", 0, NULL, &err);
		if (err != NULL)
		{
			gel_error(err->message);
			g_error_free(err);
			goto eina_fs_lomo_feed_uri_multi_next_iter;
		}

		// Filter
		if ((filter != NULL) && (filter(f_info) == EINA_FS_FILTER_REJECT)) {
				gel_warn("Rejected '%s'", (gchar *) l->data);
				goto  eina_fs_lomo_feed_uri_multi_next_iter;
		}

		// Discriminate by type
		f_type = g_file_info_get_file_type(f_info);
		switch (f_type)
		{
			case G_FILE_TYPE_REGULAR:
				files = g_list_prepend(files, g_strdup(l->data));
				break;
			case G_FILE_TYPE_DIRECTORY:
				directories = g_list_prepend(directories, g_strdup(l->data));
				break;
			default:
				goto eina_fs_lomo_feed_uri_multi_next_iter;
		}

		// Idempotent next
eina_fs_lomo_feed_uri_multi_next_iter:
		if (f_info)
			g_object_unref(f_info);
		if (f)
			g_object_unref(f);

		l = l->next;
	}

	// Add files, recurse on directories
	lomo_player_add_uri_multi(lomo, g_list_reverse(files));

	l = g_list_reverse(directories);
	while (l) {
		GList *children;
		children = eina_fs_uri_get_children(l->data);
		eina_fs_lomo_feed_uri_multi(lomo, children, filter, callback, user_data);

		g_list_foreach(children, (GFunc) g_free, NULL);
		g_list_free(children);

		l = l->next;
	}

	// Final clean up
	g_list_foreach(files, (GFunc) g_free, NULL);
	g_list_foreach(directories, (GFunc) g_free, NULL);
	g_list_free(files);
	g_list_free(directories);
}

GList *
eina_fs_parse_playlist_buffer(gchar *buffer)
{
	gchar **lines = NULL;
	gint i;
	GList *ret = NULL;

	lines = g_strsplit_set(buffer, "\n\r", 0);
	for (i = 0; lines[i] != NULL; i++)
	{
		if ((lines[i][0] == '\0') || g_str_equal(lines[i], ""))
			continue;
		ret = g_list_prepend(ret, g_strdup(lines[i]));
	}

	g_strfreev(lines);
	return g_list_reverse(ret);
}

// Return a list with children's URIs from URI
// Returned list and list data must be free
GList*
eina_fs_uri_get_children(gchar *uri)
{
	GFile *f = NULL;
	GFileEnumerator *f_enum = NULL;
	GFileInfo *f_inf = NULL;
	GError *err = NULL;
	GList *ret = NULL;

	GFile *child = NULL;

	// Get an enumerator
	f = g_file_new_for_uri(uri);
	f_enum = g_file_enumerate_children(f, "standard::*", 0, NULL, &err);
	if (f == NULL) {
		gel_error(err->message);
		g_error_free(err);
		g_object_unref(f);
		return NULL;
	}

	// each child...
	while ((f_inf = g_file_enumerator_next_file(f_enum, NULL, &err)) != NULL)
	{
		if (f_inf == NULL) {
			gel_error(err->message);
			g_error_free(err);
			continue;
		}

		// Build URI, add to result and free temporal child's GFile
		// URI string is added to result with newly allocated memory
		child = g_file_get_child(f, g_file_info_get_name(f_inf));
		ret = g_list_prepend(ret, g_file_get_uri(child));
		g_object_unref(child);
	}

	// free resources: parent GFile, enumerator
	g_file_enumerator_close(f_enum, NULL, NULL);
	g_object_unref(f_enum);
	g_object_unref(f);

	return g_list_reverse(ret);
}

GList *eina_fs_readdir(gchar *path, gboolean abspath) {
	/* Path is UTF8, g_dir_open require on-disk encoding (note: on windows is
	 * UTF8
	 */
	GDir *dir = NULL;
	const gchar* child;
	gchar *utf8_child;
	GList *ret = NULL, *ret2 = NULL;
	GError *err = NULL;

	gchar *real_path = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert UTF8 path '%s' to on-disk encoding: %s",
			path, err->message);
		goto epic_fail;
	}

	dir = g_dir_open(real_path, 0, &err);
	if (err != NULL) {
		gel_error("Cannot open dir '%s': '%s'", real_path, err->message);
		goto epic_fail;
	}

	while ((child = g_dir_read_name(dir)) != NULL) {
		utf8_child = g_filename_to_utf8(child, -1, NULL, NULL, &err);
		if (err != NULL) {
			gel_error("Cannot convert on-disk encoding '%s' to UTF8: %s",
				path, err->message);
			g_error_free(err);
			err = NULL;
			continue;
		}

		ret = g_list_prepend(ret, utf8_child);
	}
	g_dir_close(dir);

	if (abspath) {
		ret2 = eina_fs_prepend_dirname(path, ret);
		g_list_free(ret);
		ret = ret2;
	}

	return g_list_reverse(ret);

epic_fail:
	if (real_path != NULL)
		g_free(real_path);

	if (dir != NULL)
		g_dir_close(dir);

	if (err != NULL)
		g_error_free(err);

	return NULL;
}

GList *eina_fs_recursive_readdir(gchar *path, gboolean abspath) {
	GList *ret = NULL;
	GList *children, *l;

	l = children = eina_fs_readdir(path, abspath);
	while (l) {
		if (eina_fs_file_test((gchar*) l->data, G_FILE_TEST_IS_DIR)) 
			ret = g_list_concat(ret, eina_fs_recursive_readdir((gchar*) l->data, abspath));
		else
			ret = g_list_append(ret, (gchar*) l->data);
	}
	g_list_free(children);

	return ret;
}

GList*
eina_fs_prepend_dirname(gchar *dirname, GList *list)
{
	GList *ret = NULL;
	GList *l;

	l = list;
	while (l) {
		ret = g_list_append(ret, g_build_filename(dirname, l->data, NULL));
		l = l->next;
	}
	
	return g_list_reverse(ret);
}

gchar* eina_fs_utf8_to_ondisk(gchar *path)
{
	gchar *ret;
	GError *err = NULL;

	ret = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert UTF8 path '%s' to on-disk encoding: %s", path, err->message);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

gchar* eina_fs_ondisk_to_utf8(gchar *path)
{
	gchar *ret;
	GError *err = NULL;

	ret = g_filename_to_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert on-disk encoding '%s' to UTF8 path: %s", path, err->message); g_error_free(err);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

gboolean eina_fs_file_test(gchar *utf8_path, GFileTest test)
{
	gboolean ret;
	gchar *real_path = eina_fs_utf8_to_ondisk(utf8_path);
	ret = g_file_test(real_path, test);
	g_free(real_path);
	return ret;
}

#if 0
#include "base.h"
#include "fs.h"
#include <gmodule.h>
#include <libutil/libutil.h>
#include <string.h>

/* * * * * * * * * * */
/* Define ourselves  */
/* * * * * * * * * * */
struct _EinaFsFilter {
	GSList *suffixes, *suffixes_i;
	GSList *patterns,   *patterns_spec;
	GSList *mimetypes,  *mimetypes_spec;
	GSList *patterns_i, *patterns_spec_i;

	gboolean suffixes_updated, mimetypes_updated, patterns_updated;
	
	/* External functions */
	EinaFsMimetypeFunc get_mimetype;
	gpointer extern_data;
};

const gchar *_eina_fs_filter_default_mimetype_func
(EinaFsFilter *self, gchar *uri);

/* * * * * * * * * * * * * */
/* EinaFsFilter functions  */
/* * * * * * * * * * * * * */
GSList *_filter_list_by_patterns
(GSList *list, GSList *patterns);

EinaFsFilter *eina_fs_filter_new
(void)
{
	EinaFsFilter *self;
	
	self = g_new0(EinaFsFilter, 1);
	self->patterns_updated  = TRUE;
	self->mimetypes_updated = TRUE;
	self->suffixes_updated = TRUE;

	// self->get_mimetype = _eina_fs_filter_default_mimetype_func;
	return self;
}

void eina_fs_filter_free
(EinaFsFilter *self)
{

	/* Suffixes */
	g_slist_free(self->suffixes);
	g_ext_slist_free(self->suffixes_i, g_free);

	/* Patterns & mimetypes */
	g_ext_slist_free(self->patterns_i,      g_free);
	g_ext_slist_free(self->patterns_spec,   g_pattern_spec_free);
	g_ext_slist_free(self->patterns_spec_i, g_pattern_spec_free);
	g_ext_slist_free(self->mimetypes_spec,  g_pattern_spec_free);

	g_slist_free(self->patterns);
	g_slist_free(self->mimetypes);

	g_free(self);
}

GSList *_eina_fs_create_lowered_slist
(GSList *src)
{
	GSList *ret = NULL, *l;

	l = src;
	while(l) {
		ret = g_slist_prepend(ret, g_utf8_strdown((gchar *) l->data, -1));
		l = l->next;
	}
	ret = g_slist_reverse(ret);

	return ret;
}

void eina_fs_filter_set_mimetype_func
(EinaFsFilter *self, EinaFsMimetypeFunc func)
{
	self->get_mimetype = func;
}

/* Suffixes */
void eina_fs_filter_set_suffixes
(EinaFsFilter *self, const GSList *suffixes)
{
	self->suffixes = (GSList *) suffixes;
	self->suffixes_updated = FALSE;
}

void eina_fs_filter_add_suffix
(EinaFsFilter *self, const gchar *suffix)
{
	self->suffixes = g_slist_append(self->suffixes, (gchar *) suffix);
	self->suffixes_updated = FALSE;
}

const GSList *eina_fs_filter_get_suffixes
(EinaFsFilter *self)
{
	return self->suffixes;
}

#if 0
/* Patterns */
void eina_fs_filter_set_patterns
(EinaFsFilter *self, const GSList *patterns)
{
	self->patterns_updated = FALSE;
	self->patterns = (GSList *) patterns;
}

void eina_fs_filter_add_pattern
(EinaFsFilter *self, const gchar *pattern)
{
	self->patterns_updated = FALSE;
	self->patterns = g_slist_append(self->patterns, (gchar *) pattern);
}

const GSList *eina_fs_filter_get_patterns
(EinaFsFilter *self)
{
	return self->patterns;
}

/* Mimetypes */
const gchar *_eina_fs_filter_default_mimetype_func
(EinaFsFilter *self, gchar *uri)
{
	return "application/octet-stream";
}

void eina_fs_filter_set_mimetypes
(EinaFsFilter *self, const GSList *mimetypes)
{
	self->mimetypes_updated = FALSE;
	self->mimetypes = (GSList *) mimetypes;
}

void eina_fs_filter_add_mimetype
(EinaFsFilter *self, const gchar *mimetype)
{
	self->mimetypes_updated = FALSE;
	self->mimetypes = g_slist_append(self->mimetypes, (gchar *) mimetype);
}

const GSList *eina_fs_filter_get_mimetypes
(EinaFsFilter *self)
{
	return self->mimetypes;
}

#endif

GSList *eina_fs_filter_filter_by_suffixes
(EinaFsFilter *self, GSList *items, gboolean case_sensitive)
{
	GSList *ret = NULL;
	GSList *sources, *targets;
	GSList *iter, *real_iter, *s;
	gchar  *item, *suffix;

	sources = items;
	targets = self->suffixes;

	/* 
	 * Change sources and targets GSList if case in-sensitive
	 * match is used
	 */
	if (case_sensitive == FALSE) {
		sources = _eina_fs_create_lowered_slist(items);

		/* Update suffixes_i if needed */
		if (self->suffixes_updated == FALSE) {
			g_ext_slist_free(self->suffixes_i, g_free);
			self->suffixes_i = _eina_fs_create_lowered_slist(self->suffixes);
			self->suffixes_updated = TRUE;
		}

		targets = self->suffixes_i;
	}

	/*
	 * Try to match targets against sources
	 */
	real_iter = items; // Hold a reference to original list
	iter = sources;

	/* Foreach source (item)... */
	while (iter) {
		item = (gchar *) iter->data;

		/* ...and foreach target (suffix)... */
		s = targets;
		while (s) {
			suffix = (gchar *) s->data;

			/* ...if item has suffix 'suffix' then add the orignal data
			 * to returned value */
			gel_info("Matching: '%s' vs '%s' case %ssensitive",
				(gchar *) real_iter->data,
				suffix,
				case_sensitive ? "" : "in");
			if (g_str_has_suffix(item, suffix)) {
				gel_info("  match!");
				ret = g_slist_prepend(ret, real_iter->data);
				break;
			}	

			s = s->next;
		}

		iter      = iter->next;
		real_iter = real_iter->next;
	}

	if (case_sensitive == FALSE)
		g_ext_slist_free(sources, g_free);

	return g_slist_reverse(ret);
}

GSList *eina_fs_filter_filter
(EinaFsFilter *self,
	GSList *list,
	gboolean use_suffixes,
	gboolean use_patterns,
	gboolean use_mimetypes,
	gboolean case_sensitive,
	gboolean duplicate)
{
	GSList *ret = NULL, *utf8_list = NULL, *l;
	GSList *duplicates = NULL;

	if (case_sensitive == FALSE) {
		case_sensitive = TRUE;
		gel_warn(
			"For now, EinaFsFilter dont support"
			"case in-sensitive filtering");
	}


	/*
	 * UTF-8 filtering
	 */
	l = list;
	while (l) {
		if (!g_utf8_validate((gchar *) l->data, -1, NULL)) {
			gel_error("'%s' is not valid UTF-8 data, deleting from input",
				(gchar *) l->data);
		} else {
			utf8_list = g_slist_prepend(utf8_list, l->data);
		}
		l = l->next;
	}
	utf8_list = g_slist_reverse(utf8_list);
	
	/*
	 * Do suffix based filtering
	 */
	if (use_suffixes) {
		ret = g_slist_concat(
			ret,
			eina_fs_filter_filter_by_suffixes(self, utf8_list, case_sensitive));
	}

	if (use_mimetypes) {
		e_implement("EinaFsFilter lacks mimetype support for now");
	}
	
	if (use_patterns) {
		e_implement("EinaFsFilter lacks pattern support for now");
	}
	
	/* 
	 * After all if duplicate is TRUE, replicate data on the list
	 */
	if (duplicate) {
		gel_info("Duplicating");
		l = ret;

		while (l) {
			duplicates = g_slist_prepend(
				duplicates,
				g_strdup((gchar *) l->data));
			l = l->next;
		}

		g_slist_free(ret);
		ret = g_slist_reverse(duplicates);
	}

	g_slist_free(utf8_list);
	return ret;
}

/*
 * Init/Exit functions 
 */

struct _EinaFs {
	EinaBase   parent;
	GtkWidget *widget;
};

G_MODULE_EXPORT gboolean eina_fs_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaFs *self;

	self = g_new0(EinaFs, 1);
	if(!eina_base_init((EinaBase *) self, hub, "fs", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_fs_exit
(gpointer data)
{
	EinaFs *self = (EinaFs *) data;
	if (self->widget)
		g_object_unref(self->widget);

	eina_base_fini((EinaBase *) self);
	return TRUE;
}

#define EINA_FS_MAX_SYMLINK_LEVEL 8
gchar *_eina_fs_resolve_symlink(gchar *path) {
	gint try = 0;
	GError *error;
	gchar *tmp, *tmp2;

	tmp = g_strdup(path);
	while (
		(try <= EINA_FS_MAX_SYMLINK_LEVEL) &&
		!g_file_test(tmp, G_FILE_TEST_IS_SYMLINK)
		) {

		tmp2 = g_file_read_link(tmp, &error);
		g_free(tmp);
		tmp = tmp2;

		/* Here no memleaks can occur, so check errors and exit */
		if (tmp == NULL) {
			gel_error("Error reading symlink %s (from '%s'): %s",
				tmp, path, error->message);
			g_error_free(error);
			return NULL;
		}
		
		path = tmp;

		try++;
	}

	if (try >= EINA_FS_MAX_SYMLINK_LEVEL) {
		g_free(tmp);
		gel_warn("Max symlink level reached for %s (last '%s')",
			path, tmp);
		return NULL;
	}
	
	return tmp;
}

GSList *eina_fs_scan_simple(gchar *uri) {
	GSList *tmp_list = NULL, *ret = NULL;

	/* Create a temporal list with the item, scan and free */
	tmp_list = g_slist_append(tmp_list, uri);
	ret = eina_fs_scan(tmp_list);
	g_slist_free(tmp_list);

	return ret;
}

GSList *eina_fs_scan(GSList *uri_list) {
	GSList *l;              // Iterator for input list
	gchar  *uri;            // Shortcut for l
	GSList *ret     = NULL; // Return value
	GSList *dirs    = NULL; // List for dirs in scan
	GSList *files   = NULL; // List for files in scan
	GDir   *dir_ent = NULL; // Dir handler for scan
	GError *err     = NULL; // Holds errors

	const gchar *e;
	gchar *e_full;

	l = uri_list;
	while (l) {
		uri = (gchar *) l->data;
		/*
		 * 1. Its a contanier (folder, mp3u, pls)
		 *    or item (ogg, mp3, flac, HTTP stream, ...)?
		 * 2. Expand recursive
		 * 3. Filter items based on patterns/mimetype
		 *    criteria
		 */

		/* 
		 * uri MUST start with file:// to allow us to check if
		 * it is a container or not. If has another prefix add has item
		 */
		if (!g_str_has_prefix(uri, "file://")) {
			goto go_next_in_uri_list;
		}

		/* Strip the file:// prefix and check if its a folder */
		uri = uri + 7;

		/* 
		 * This uri is a regular file, append to return value 
		 * with newly allocated memory
		 */
		if (g_file_test(uri, G_FILE_TEST_IS_REGULAR)) {
			ret = g_slist_append(ret, g_strconcat("file://", uri, NULL));
		}

		/*
		 * uri is a directory, we have to do more operations
		 */
		else if (g_file_test(uri, G_FILE_TEST_IS_DIR)) {

			/* Read directory spliting between subdirs and files */
			if ((dir_ent = g_dir_open(uri, 0, &err)) == NULL) {
				g_error_free(err);
				goto go_next_in_uri_list;
			}

			/* Process dir */
			while ((e = g_dir_read_name(dir_ent)) != NULL) {
				e_full = g_build_filename(uri, e, NULL);

				if (g_file_test(e_full, G_FILE_TEST_IS_REGULAR)) {
					g_printf("F '%s'\n", e_full);
					files = g_slist_append(files,
						g_strconcat("file://", e_full, NULL));
					g_free(e_full);
				}

				else if (g_file_test(e_full, G_FILE_TEST_IS_DIR)) {
					g_printf("+ '%s'\n", e_full);
					dirs = g_slist_append( dirs,
						g_strconcat("file://", e_full, NULL));
					g_free(e_full);
				}
				
				else {
					g_free(e_full);
				}
			}

			/* Close dir */
			g_dir_close(dir_ent);

			/* Re-scan subdirs */
			ret = g_slist_concat(ret, eina_fs_scan(dirs));
			ret = g_slist_concat(ret, files);
		}

go_next_in_uri_list:
		l = l->next;
		dirs = NULL;
		files = NULL;
	}
	return ret;
}

GSList *eina_fs_readdir(gchar *uri, gboolean full_uri) {
	GSList *ret = NULL;
	GDir   *d;
	gchar  *path;
	const gchar *e;
	GError *error = NULL;

	/* Ensure we get a full uri */
	if (!g_str_has_prefix(uri, "file:///")) {
		return NULL;
	}
	
	/* uri -> path conversion */
	path = uri + (strlen("file://") * sizeof(char));
	
	if ((d = g_dir_open(path, 0, &error)) == NULL) {
		gel_error(error->message);
		g_error_free(error);
		return NULL;
	}

	while ((e = g_dir_read_name(d)) != NULL) {
		if (full_uri)
			ret = g_slist_prepend(ret, g_strconcat(uri, "/", e, NULL));
		else
			ret = g_slist_prepend(ret, g_strdup(e));
	}
	g_dir_close(d);
	ret = g_slist_reverse(ret);

	return ret;
}

GtkWidget *eina_fs_create_dialog(EinaFs *self, EinaFsMode mode) {
	if (self->widget == NULL ) {
		/* Create the widget with apropiate backend */
		self->widget = gtk_file_chooser_dialog_new_with_backend(
	        "",
	        NULL,
	        GTK_FILE_CHOOSER_ACTION_OPEN,
			NULL,
	        GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			/*
	        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			*/
	   	    NULL);
		g_signal_connect(self->widget, "destroy",
			G_CALLBACK(gtk_widget_hide_on_delete), NULL);
		g_signal_connect(self->widget, "response",
			G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	}

	switch (mode) {
		case EINA_FS_MODE_LOAD:
			break;

		default:
			gel_warn("Unknow mode %d", mode);
			break;
	}

	return self->widget;
}

G_MODULE_EXPORT GelHubSlave fs_connector = {
	"fs",
	&eina_fs_init,
	&eina_fs_exit
};
#endif
