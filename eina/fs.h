#ifndef __FS_H__
#define __FS_H__

#include <glib.h>
#include <gio/gio.h>
#include <lomo/player.h>

typedef enum {
	EINA_FS_FILTER_ACCEPT,
	EINA_FS_FILTER_REJECT
} EinaFsFilterAction;

typedef EinaFsFilterAction (*EinaFsFilterFunc)(GFileInfo *fileinfo);

void
eina_fs_lomo_feed_uri_multi(LomoPlayer *lomo, GList *uris, EinaFsFilterFunc filter, GCallback callback, gpointer data);

void
eina_fs_readdir_async(gchar *uri, gpointer data);

#if 0
GList *
eina_fs_parse_playlist_buffer(gchar *buffer);
#endif

GList*
eina_fs_uri_get_children(gchar *uri);

/*
 * Utility functions to handle on-disk files, paths, dirs, etc...
 */
GList* eina_fs_readdir(gchar *path, gboolean abspath);
GList *eina_fs_recursive_readdir(gchar *path, gboolean abspath);
GList* eina_fs_prepend_dirname(gchar *dirname, GList *list);

gchar* eina_fs_utf8_to_ondisk(gchar *path);
gchar* eina_fs_ondisk_to_utf8(gchar *path); 

gboolean eina_fs_file_test(gchar *utf8_path, GFileTest test);

#if 0
#include <glib.h>
#include "libghub/ghub.h"

typedef struct _EinaFs EinaFs;
typedef enum {
	EINA_FS_FILTER_TYPE_EXTENSION,
	EINA_FS_FILTER_TYPE_MIMETYPE
} EinaFsFilterType;

typedef enum {
	EINA_FS_MODE_LOAD,
	EINA_FS_MODE_SAVE
} EinaFsMode;


/* * * * * * * * * * * * * * */
/* EinaFsFilter pseudo-class */
/* * * * * * * * * * * * * * */
typedef struct _EinaFsFilter EinaFsFilter;
typedef const gchar *(*EinaFsMimetypeFunc) (gchar *uri);

EinaFsFilter *eina_fs_filter_new
	(void);
void eina_fs_filter_free
	(EinaFsFilter *self);

void eina_fs_filter_set_mimetype_func
	(EinaFsFilter *self, EinaFsMimetypeFunc func);

void eina_fs_filter_set_suffixes
	(EinaFsFilter *self, const GSList *suffixes);
void eina_fs_filter_add_suffix
	(EinaFsFilter *self, const gchar *suffix);
const GSList *eina_fs_filter_get_suffixes
	(EinaFsFilter *self);



#if 0
void eina_fs_filter_set_patterns
	(EinaFsFilter *self, const GSList *patterns);
void eina_fs_filter_add_pattern
	(EinaFsFilter *self, const gchar *pattern);
const GSList *eina_fs_filter_get_patterns
	(EinaFsFilter *self);

void eina_fs_filter_set_mimetypes
	(EinaFsFilter *self, const GSList *mimetypes);
void eina_fs_filter_add_mimetype
	(EinaFsFilter *self, const gchar *mimetype);
const GSList *eina_fs_filter_get_mimetypes(
	EinaFsFilter *self);
#endif

GSList *eina_fs_filter_filter
(EinaFsFilter *self,
	GSList *list,
	gboolean use_suffixes,
	gboolean use_patterns,
	gboolean use_mimetypes,
	gboolean case_sensitive,
	gboolean duplicate);

/* * * * * * * * * * * */
/* EinaFs Dialog Class */
/* * * * * * * * * * * */
gboolean eina_fs_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean eina_fs_exit(gpointer data);

GtkWidget *eina_fs_create_dialog(EinaFs *self, EinaFsMode mode);

/* Scan funcitions */
GSList *eina_fs_scan_simple(gchar *uri);
GSList *eina_fs_scan       (GSList *uri_list);
GSList *eina_fs_readdir(gchar *uri, gboolean full_uri);

/* Handle internal filters */
void eina_fs_filter_reset(EinaFs *self);

#if 0
void eina_fs_filter_add_patterns(EinaFs *self, ...);
void eina_fs_filter_add_mimetypes (EinaFs *self, ...);
const GSList *eina_fs_filter_get_patterns(EinaFs *self);
const GSList *eina_fs_filter_get_mimetypes (EinaFs *self);
#endif

#endif
#endif
