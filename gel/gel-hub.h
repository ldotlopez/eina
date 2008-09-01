#ifndef _GEL_HUB
#define _GEL_HUB

#include <glib-object.h>

G_BEGIN_DECLS

#define GEL_TYPE_HUB gel_hub_get_type()

#define GEL_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_TYPE_HUB, GelHub))

#define GEL_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GEL_TYPE_HUB, GelHubClass))

#define GEL_IS_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_TYPE_HUB))

#define GEL_IS_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_TYPE_HUB))

#define GEL_HUB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_TYPE_HUB, GelHubClass))

typedef struct {
	GObject parent;
	GOptionContext *opt_cntxt;
} GelHub;

typedef struct {
	GObjectClass parent_class;
	void (*module_load)   (GelHub *self, const gchar *name);
	void (*module_unload) (GelHub *self, const gchar *name);
	void (*module_init)   (GelHub *self, const gchar *name);
	void (*module_fini)   (GelHub *self, const gchar *name);
	void (*module_ref)    (GelHub *self, const gchar *name, guint refs);
	void (*module_unref)  (GelHub *self, const gchar *name, guint refs);
} GelHubClass;

GType gel_hub_get_type (void);
GelHub* gel_hub_new (gint *argc, gchar **argv[]);

/*
typedef struct _GelHubPrivate GelHubPrivate;
typedef struct {
	GObject parent;
} GelHub;
*/
typedef void (*GelHubCallback) (GelHub *hub, gpointer user_data);

void gel_hub_set_dispose_callback(GelHub *self, GelHubCallback callback, gpointer user_data);

gboolean gel_hub_load  (GelHub *self, gchar *name);
gboolean gel_hub_unload(GelHub *self, gchar *name);
gboolean gel_hub_loaded(GelHub *self, gchar *name);

gpointer gel_hub_shared_register  (GelHub *self, gchar *name, guint size);
void     gel_hub_shared_unregister(GelHub *self, gchar *name);
gpointer gel_hub_shared_get       (GelHub *self, gchar *name);
gboolean gel_hub_shared_set       (GelHub *self, gchar *name, gpointer mem);

typedef gboolean (*GelHubSlaveInitFunc)(GelHub *hub, gint *argc, gchar ***argv);
typedef gboolean (*GelHubSlaveExitFunc)(gpointer component);
typedef struct GelHubSlave {
	const gchar           *name;
	GelHubSlaveInitFunc  init_func;
	GelHubSlaveExitFunc  exit_func;
} GelHubSlave;

G_END_DECLS

#endif /* _GEL_HUB */
