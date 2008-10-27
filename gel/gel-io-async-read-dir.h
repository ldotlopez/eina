#ifndef GEL_IO_DISABLE_DEPRECATED
#ifndef _GEL_IO_ASYNC_READ_DIR
#define _GEL_IO_ASYNC_READ_DIR

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GEL_IO_TYPE_ASYNC_READ_DIR gel_io_async_read_dir_get_type()

#define GEL_IO_ASYNC_READ_DIR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_IO_TYPE_ASYNC_READ_DIR, GelIOAsyncReadDir))

#define GEL_IO_ASYNC_READ_DIR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GEL_IO_TYPE_ASYNC_READ_DIR, GelIOAsyncReadDirClass))

#define GEL_IO_IS_ASYNC_READ_DIR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_IO_TYPE_ASYNC_READ_DIR))

#define GEL_IO_IS_ASYNC_READ_DIR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_IO_TYPE_ASYNC_READ_DIR))

#define GEL_IO_ASYNC_READ_DIR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_IO_TYPE_ASYNC_READ_DIR, GelIOAsyncReadDirClass))

typedef struct {
  GObject parent;
} GelIOAsyncReadDir;

typedef struct {
  GObjectClass parent_class;
  void (*cancel) (GelIOAsyncReadDir *self);
  void (*error)  (GelIOAsyncReadDir *self, GError *error);
  void (*finish) (GelIOAsyncReadDir *self, GList *results);
} GelIOAsyncReadDirClass;

GType gel_io_async_read_dir_get_type (void);

GelIOAsyncReadDir* gel_io_async_read_dir_new (void);

void
gel_io_async_read_dir_scan(GelIOAsyncReadDir *self, GFile *file, const gchar *attributes);

void
gel_io_async_read_dir_cancel(GelIOAsyncReadDir *self);


G_END_DECLS

#endif /* _GEL_IO_ASYNC_READ_DIR */
#endif
