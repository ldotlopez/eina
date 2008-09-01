// http://www.ajaxload.info/
#define GEL_DOMAIN "Eina::Cover"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include "base.h"
#include "player.h"
#include "fs.h"
#include "cover.h"
#include "config.h"


/*
 * Workflow:
 * 1. Connect to lomo's on_change signal.
 * 2. Look for cover in disk/adb.
 *    Found?
 *    2.1 Use it, update timestamp?
 *    Not found?
 *    2.2 Check www.last.fm for a cover.
 *        Found?
 *        2.2.1 Save it to disk/adb.
 *        2.2.2 Use it. Goto 2?
 *        Not found?
 *        2.2.1 Set default logo/cover.
 *
 * Other cases:
 * - Set default logo/cover on:
 *   clear signal
 *   startup
 *   first file added (I think lomo emits change signal automatically)
 *
 * Util functions:
 *  _eina_cover_set_from_filename(EinaCover *self, gchar *path);
 *  _eina_cover_set_from_pixbuf(EinaCover *self, GdkPixpub *pb);
 * 
 */

/* * * * * * * * * * */
/* Define ourselves  */
/* * * * * * * * * * */
struct _EinaCover {
	EinaBase           parent;
	const LomoStream  *stream;
	gchar             *artist;
	gchar             *album;
	GtkWidget         *cover_img, *player_window;
	GdkPixbuf         *default_img;

	/* CURL Stuff */
	CURLM *curl_m;
	CURL  *curl_s;
	gchar *curl_url;
	enum {
		EINA_COVER_CURL_PHASE_NONE,
		EINA_COVER_CURL_PHASE_HTML,
		EINA_COVER_CURL_PHASE_HTML2, // Fallback to artist generic cover
		EINA_COVER_CURL_PHASE_COVER
	} curl_phase;
	gchar *curl_buffer;
	size_t curl_buffersize;
};

/* * * * * * * * */
/* Internal API  */
/* * * * * * * * */
void _eina_cover_save_cover
(EinaCover *self);

void _eina_cover_parse_html
(EinaCover *self);

void eina_cover_set_from_filename
(EinaCover *self, gchar *path);

void eina_cover_set_from_inet
(EinaCover *self);

void eina_cover_update_cover
(EinaCover *self);

/* * * * * * * * * */
/* Lomo Callbacks  */
/* * * * * * * * * */
void on_cover_lomo_change
(LomoPlayer *lomo, gint from, gint to, EinaCover *self);

void on_cover_lomo_clear
(LomoPlayer *lomo, EinaCover *self);

void on_cover_lomo_all_tags
(LomoPlayer *lomo, const LomoStream *stream, EinaCover *self);

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
void on_cover_cover_img_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaCover        *self);

/* * * * * * * * * * * */
/* Init/Exit functions */
/* * * * * * * * * * * */
G_MODULE_EXPORT gboolean cover_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaCover  *self;
	EinaPlayer *player;
	gchar      *path;

	/* Load components and create some auxiliar objects */
	self = g_new0(EinaCover, 1);
	if(!eina_base_init((EinaBase *) self, hub, "cover", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	if ((player = gel_hub_shared_get(HUB(self), "player")) == NULL) {
		gel_error("Cannot get player component");
		return FALSE;
	}

	if ((self->player_window = W(player, "main-window")) == NULL) {
		gel_error("Cannot get main-window widget");
	}

	if ((self->cover_img = W(player, "cover-img")) == NULL) {
		gel_error("Cannot get cover widget");
		return FALSE;
	}

	/*
	if (!gtk_ext_make_widget_dropable(W(player, "cover-img-ebox"), NULL)) {
		e_warn("Cannot make cover-img-ebox widget dropable");
	}
	*/
	/* Init curl */
	self->curl_m = curl_multi_init();
	if (!self->curl_m) {
		gel_warn("Cannot init curl\n");
	}

	/* Prepare default cover */
	// path = g_ext_find_file(G_EXT_FILE_TYPE_PIXMAP, "icon.png");
	path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "icon.png");
	self->default_img = gdk_pixbuf_new_from_file_at_size(path,
		48, 48,
		NULL);
	g_free(path);
	g_object_ref(self->default_img);

	/* Create the cover cache dir */
	path = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "covers", NULL);
	if (!g_file_test(path,G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) && (g_mkdir_with_parents(path, 0700) == -1)) {
		gel_warn("Cannot create cover cache path: '%s'", path);
	}
	g_free(path);

	/* Connect signals */
	g_signal_connect(LOMO(self), "change",
		G_CALLBACK(on_cover_lomo_change), self);

	g_signal_connect(LOMO(self), "all-tags",
		G_CALLBACK(on_cover_lomo_all_tags), self);
	
	g_signal_connect(LOMO(self), "clear",
		G_CALLBACK(on_cover_lomo_clear), self);

	g_signal_connect(W(player, "cover-img-ebox"), "drag-data-received",
		G_CALLBACK(on_cover_cover_img_drag_data_received), self);
	
	self->stream = lomo_player_get_stream(LOMO(self));
	eina_cover_update_cover(self);

	return TRUE;
}

G_MODULE_EXPORT gboolean cover_exit
(gpointer data)
{
	EinaCover *self = (EinaCover *) data;

	/* CURL stuff */
	if (self->curl_s) {
		curl_multi_remove_handle(self->curl_m, self->curl_s);
		curl_easy_cleanup(self->curl_s);
	}
	curl_multi_cleanup(self->curl_m);
	if (self->curl_buffer)
		g_free(self->curl_buffer);
	if (self->curl_url)
		g_free(self->curl_url);

	g_object_unref(self->default_img);

	eina_base_fini((EinaBase *) self);
	return TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * */
/* Main function, follows workflow described above */
/* * * * * * * * * * * * * * * * * * * * * * * * * */
#define EXPERIMENTAL_COVER 0
#if EXPERIMENTAL_COVER
void eina_cover_update_cover(EinaCover *self) {
	
}
#endif

/* * * * * * * * * * */
/* libcurl handlers  */
/* * * * * * * * * * */

void _eina_cover_curl_stop
(EinaCover *self)
{
	if (self->curl_phase == EINA_COVER_CURL_PHASE_NONE)
		return;

	/* Stop transfers if any */
	if (self->curl_s) {
		curl_multi_remove_handle(self->curl_m, self->curl_s);
		curl_easy_cleanup(self->curl_s);
		self->curl_s = NULL;
	}

	/* Free internal buffers */
	if (self->curl_buffer) {
		g_free(self->curl_buffer);
		self->curl_buffer = NULL;
		self->curl_buffersize = 0;
	}

	/* Our job is done */
	self->curl_phase = EINA_COVER_CURL_PHASE_NONE;
}

size_t _eina_cover_curl_write
(void *ptr, size_t size, size_t nmemb, void *data)
{
	EinaCover *self = (EinaCover *) data;
	gpointer *dst;
    gint bytes = size * nmemb;

	if (bytes < 0)
		return bytes;

	switch (self->curl_phase) {
		case EINA_COVER_CURL_PHASE_HTML:
		case EINA_COVER_CURL_PHASE_COVER:
			self->curl_buffer = g_try_renew(gchar, self->curl_buffer, self->curl_buffersize + bytes);
			if (!self->curl_buffer) {
				gel_error("Cannot reallocate CURL buffer");
				return 0;
			}

			dst = ((gpointer) self->curl_buffer) + self->curl_buffersize;
			memcpy(dst, ptr, (size*nmemb));

			self->curl_buffersize += (size*nmemb);
			break;

		default:
			gel_warn("Unknow curl phase");
			break;
	}

    return bytes;
}

void _eina_cover_curl_finalize
(EinaCover *self)
{
	switch (self->curl_phase) {
		case EINA_COVER_CURL_PHASE_HTML:
			self->curl_buffer = g_try_renew(gchar, self->curl_buffer, self->curl_buffersize + 1);
			self->curl_buffer[self->curl_buffersize+1] = '\0';
			_eina_cover_parse_html(self);
			break;

		case EINA_COVER_CURL_PHASE_COVER:
			_eina_cover_save_cover(self);
			curl_multi_remove_handle(self->curl_m, self->curl_s);
			curl_easy_cleanup(self->curl_s);
			self->curl_s = NULL;

			g_free(self->curl_url);
			self->curl_url = NULL;

			self->curl_s = NULL;
		
			break;

		default:
			gel_warn("Unknow curl phase");
			break;
	}
}

gboolean _eina_cover_curl_tick
(gpointer data)
{
	EinaCover *self = (EinaCover *) data;
    CURLMcode r;
    gint i;

    do {
        r = curl_multi_perform(self->curl_m, &i);
    } while (r == CURLM_CALL_MULTI_PERFORM);

    if (i > 0)
		return TRUE;
	else {
		_eina_cover_curl_finalize(self);
        return FALSE;
	}
}

/* * * * * * * * * * * * * * * */
/* Internal API implementation */
/* * * * * * * * * * * * * * * */
void eina_cover_set_from_gdk_pixbuf
(EinaCover *self, GdkPixbuf *pb)
{
	GdkPixbuf *window_icon;

	if (pb == NULL)
		pb = self->default_img;

	gtk_image_set_from_pixbuf(GTK_IMAGE(self->cover_img), pb);

	if (self->player_window != NULL) {
		window_icon = gdk_pixbuf_copy(pb);
		gtk_window_set_icon(GTK_WINDOW(self->player_window), window_icon);
	}
}

void eina_cover_set_from_filename
(EinaCover *self, gchar *path)
{
	GdkPixbuf *pb;
	GError    *error = NULL;

	if (path == NULL) {
		eina_cover_set_from_gdk_pixbuf(self, NULL);
		return;
	}
	
	pb = gdk_pixbuf_new_from_file_at_size(path, 48, 48, &error);
	if (pb == NULL) {
		gel_error("Cannot load cover from '%s': %s",
			path, error->message);
		g_error_free(error);
	}

	eina_cover_set_from_gdk_pixbuf(self, pb);
}


void eina_cover_set_from_inet
(EinaCover *self)
{
	gint i;
	self->curl_s = curl_easy_init();
	if (!self->curl_s) {
		gel_error("Cannot init curl easy");
		return;
	}

	self->curl_url = g_strconcat("http://www.last.fm/music/", self->artist, "/", self->album, NULL);
	for (i = 0; self->curl_url[i]; i++) {
		if (self->curl_url[i] == ' ')
			self->curl_url[i] = '+';
	}

	// e_info("Fetch URL: %s", self->curl_url);

	self->curl_phase = EINA_COVER_CURL_PHASE_HTML;
	self->curl_s = curl_easy_init();
	curl_easy_setopt(self->curl_s, CURLOPT_URL, self->curl_url);
	curl_easy_setopt(self->curl_s, CURLOPT_WRITEFUNCTION, _eina_cover_curl_write);
	curl_easy_setopt(self->curl_s, CURLOPT_WRITEDATA, self);
	curl_easy_setopt(self->curl_s, CURLOPT_AUTOREFERER, 1);
	curl_easy_setopt(self->curl_s, CURLOPT_FOLLOWLOCATION, 1);
    curl_multi_add_handle(self->curl_m, self->curl_s);
    g_timeout_add(100, _eina_cover_curl_tick, self);
}

/* * * * * * * * * */
/* Lomo Callbacks  */
/* * * * * * * * * */
void on_cover_lomo_change
(LomoPlayer *player, gint from, gint to, EinaCover *self)
{
	/* Set to default cover if we cannot get the stream */
	if ((self->stream = lomo_player_get_nth(player, to)) == NULL) {
		eina_cover_set_from_filename(self, NULL);
	} else {
		eina_cover_update_cover(self);
	}
}

void on_cover_lomo_clear
(LomoPlayer *lomo, EinaCover *self)
{
	eina_cover_set_from_filename(self, NULL);
}

void on_cover_lomo_all_tags
(LomoPlayer *lomo, const LomoStream *stream, EinaCover *self)
{
	if (self->stream != stream)
		return;

	/* Current stream has got all tags signal,
	 * we need to update cover if needed */
	eina_cover_update_cover(self);
}

void eina_cover_update_cover
(EinaCover *self) 
{
	gchar *path;
	gchar *dirname;
	GList *ls, *l;
	const gchar *covers[] = { "cover", "Cover", "Front", "front", NULL };
	gint i;

	if (self->stream == NULL) {
		eina_cover_set_from_filename(self, NULL);
		return;
	}

	self->artist = lomo_stream_get(self->stream, LOMO_TAG_ARTIST);
	self->album  = lomo_stream_get(self->stream, LOMO_TAG_ALBUM);

	if ((self->artist == NULL) || (self->album == NULL)) {
		/*
		e_info("Streams(%s) need artist(%s) and album(%s) to get cover",
			lomo_stream_get(self->stream, LOMO_TAG_URI),
			self->artist,
			self->album);
		*/
		eina_cover_set_from_filename(self, NULL);
		return;
	}

	/* Check in cache */
	path = g_strconcat(g_get_home_dir(),
		G_DIR_SEPARATOR_S, "." PACKAGE_NAME,
		G_DIR_SEPARATOR_S, "covers",
		G_DIR_SEPARATOR_S, self->artist, " - ", self->album, ".jpg",
		NULL);

	if (g_file_test(path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR)) {
		eina_cover_set_from_filename(self, path);
		g_free(path);
		return;
	}

	/* Check from fs */
	dirname = g_path_get_dirname(lomo_stream_get(self->stream, LOMO_TAG_URI));
	path = g_filename_from_uri(dirname, NULL, NULL);
	g_free(dirname);
	if (path != NULL ) {
		l = ls = eina_fs_readdir(path , TRUE);
		g_free(path);

		while(l) {
			for (i = 0; covers[i] != NULL; i++) {
				if (strstr((gchar *) l->data, covers[i]) != NULL) {
					// eina_cover_set_from_filename(self, (gchar *) l->data + strlen("file://"));
					eina_cover_set_from_filename(self, (gchar *) l->data);
					gel_glist_free(ls, (GFunc) g_free, NULL);
					return;
				}
			}

			l = l->next;
		}
		gel_glist_free(ls, (GFunc) g_free, NULL);
	}
	
	
	/* Fetch from inet */
	eina_cover_set_from_inet(self);
	
	return;
}

/* * * * * * * * * * * * * * * * * */
/* Handle fetching cover from inet */
/* * * * * * * * * * * * * * * * * */
void _eina_cover_save_cover
(EinaCover *self)
{
	gint fd;
	GdkPixbuf *pb;
	gchar *aa_str;
	gchar *output;

	aa_str = g_strconcat(
		"covers", G_DIR_SEPARATOR_S,
		self->artist, " - ", self->album,
		".jpg",
		NULL);
	output = gel_app_userdir_get_pathname(PACKAGE_NAME, aa_str, TRUE, 0700);
	g_free(aa_str);
	
	/* Dump to disk */
	fd = g_open(output, O_CREAT | O_WRONLY, 0600);
	write(fd, self->curl_buffer, self->curl_buffersize);
	close(fd);

	/* Resize */
	pb = gdk_pixbuf_new_from_file_at_size(output, 48, 48, NULL);
	gdk_pixbuf_save(pb, output, "jpeg", NULL, NULL);
	eina_cover_set_from_gdk_pixbuf(self, pb);

	g_free(output);
}

void _eina_cover_parse_html
(EinaCover *self) 
{
	gchar *tmp, *tmp2;
	gchar *cover_url;

	gchar *buffer = self->curl_buffer;
	if (buffer == NULL) {
		eina_cover_set_from_filename(self, NULL);
		return;
	}

	if ((tmp = strstr(buffer, "<span class=\"art\">")) == NULL) {
		// gel_error("Cannot find token A");
		eina_cover_set_from_filename(self, NULL);
		return;
	}
		
	if ((tmp2 = strstr(tmp, "src=\"")) == NULL) {
		// gel_error("Cannot find token B");
		eina_cover_set_from_filename(self, NULL);
		return;
	}
	tmp2 += (sizeof(char) * 5);
	// We have tmp2 pointed and begin of cover URL
	// Use tmp to cut the img src
	tmp = strstr(tmp2, "\"");
	tmp[0] = '\0';

	// e_info("Cover URL: %s\n", tmp2); /// OK !!!

	/* Complete URL */
	if (strstr(tmp2, "http://") == tmp2) {
		cover_url = g_strdup(tmp2);
	} 

	/* Absolute URL */
	else if (tmp2[0] == '/') {
		cover_url = g_strconcat("http://www.last.fm", tmp2, NULL);
	}

	/* Relative URL */
	else {
		gel_implement("Build relative URLs");
		cover_url = NULL;
	}

	if (!cover_url)
		return;

	/* Stop HTML curl stuff */
	curl_multi_remove_handle(self->curl_m, self->curl_s);
	curl_easy_cleanup(self->curl_s);
	g_free(self->curl_buffer);
	g_free(self->curl_url);
	self->curl_buffersize = 0;
	self->curl_buffer = NULL;

	/* Start cover fetching */
	self->curl_phase = EINA_COVER_CURL_PHASE_COVER;
	self->curl_s = curl_easy_init();
	self->curl_url = cover_url;
	curl_easy_setopt(self->curl_s, CURLOPT_URL, self->curl_url);
	curl_easy_setopt(self->curl_s, CURLOPT_WRITEFUNCTION, _eina_cover_curl_write);
	curl_easy_setopt(self->curl_s, CURLOPT_WRITEDATA, self);
	curl_easy_setopt(self->curl_s, CURLOPT_AUTOREFERER, 1);
	curl_easy_setopt(self->curl_s, CURLOPT_FOLLOWLOCATION, 1);
    curl_multi_add_handle(self->curl_m, self->curl_s);
    g_timeout_add(100, _eina_cover_curl_tick, self);
}


/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
void on_cover_cover_img_drag_data_received
(GtkWidget *widget,
	GdkDragContext   *drag_context,
	gint              x,
	gint              y,
	GtkSelectionData *data,
	guint             info,
	guint             time,
	EinaCover        *self)
{

	GdkPixbuf *pb;
	gchar    **uris = NULL;
	gchar     *filename = NULL;
	gchar     *host;
	gchar     *path  = NULL;
	GError    *error = NULL;

	/* If we dont have artist or album we cannot save the cover */
	if ((self->artist == NULL) || (self->album == NULL)) {
		gel_error("Only streams with artist and album tags can be saved");
		goto fail;
	}

	/* Load URI into pixbuf */
	uris = g_uri_list_extract_uris((const gchar *) data->data);
	if ((filename = g_filename_from_uri(uris[0], &host, NULL)) == NULL) {
		goto fail;
	}

	if (host != NULL) {
		gel_error("Only local files can be loaded into cover");
		goto fail;
	}

	pb = gdk_pixbuf_new_from_file_at_size(filename, 48, 48, &error);
	if (pb == NULL) {
		gel_error("Cannot load '%s' into a GdkPixbuf: '%s'",
			filename,
			error->message);
		goto fail;
	}

	path = g_strconcat(g_get_home_dir(),
		G_DIR_SEPARATOR_S, "." PACKAGE_NAME,
		G_DIR_SEPARATOR_S, "covers",
		G_DIR_SEPARATOR_S, self->artist, " - ", self->album, ".jpg",
		NULL);

	gdk_pixbuf_save(pb, path, "jpeg", NULL, NULL);
	eina_cover_set_from_gdk_pixbuf(self, pb);

fail:
	if (uris != NULL)
		g_strfreev(uris);

	if (host != NULL)
		g_free(host);
	
	if (filename != NULL)
		g_free(filename);

	if (path != NULL)
		g_free(path);

	return;
}

/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GelHubSlave cover_connector = {
	"cover",
	&cover_init,
	&cover_exit
};

