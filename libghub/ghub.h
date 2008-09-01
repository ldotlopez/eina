#ifndef _G_HUB
#define _G_HUB

#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_HUB g_hub_get_type()

#define G_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HUB, GHub))

#define G_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_HUB, GHubClass))

#define G_IS_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_HUB))

#define G_IS_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_HUB))

#define G_HUB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HUB, GHubClass))

typedef struct {
	GObject parent;
	GOptionContext *opt_cntxt;
} GHub;

typedef struct {
	GObjectClass parent_class;
	void (*module_load)   (GHub *self, const gchar *name);
	void (*module_unload) (GHub *self, const gchar *name);
	void (*module_init)   (GHub *self, const gchar *name);
	void (*module_fini)   (GHub *self, const gchar *name);
	void (*module_ref)    (GHub *self, const gchar *name, guint refs);
	void (*module_unref)  (GHub *self, const gchar *name, guint refs);
} GHubClass;

GType g_hub_get_type (void);
GHub* g_hub_new (gint *argc, gchar **argv[]);

/*
typedef struct _GHubPrivate GHubPrivate;
typedef struct {
	GObject parent;
} GHub;
*/
typedef void (*GHubCallback) (GHub *hub, gpointer user_data);

void g_hub_set_dispose_callback(GHub *self, GHubCallback callback, gpointer user_data);

gboolean g_hub_load  (GHub *self, gchar *name);
gboolean g_hub_unload(GHub *self, gchar *name);
gboolean g_hub_loaded(GHub *self, gchar *name);

gpointer g_hub_shared_register  (GHub *self, gchar *name, guint size);
void     g_hub_shared_unregister(GHub *self, gchar *name);
gpointer g_hub_shared_get       (GHub *self, gchar *name);
gboolean g_hub_shared_set       (GHub *self, gchar *name, gpointer mem);

typedef gboolean (*GHubSlaveInitFunc)(GHub *hub, gint *argc, gchar ***argv);
typedef gboolean (*GHubSlaveExitFunc)(gpointer component);
typedef struct GHubSlave {
	const gchar           *name;
	GHubSlaveInitFunc  init_func;
	GHubSlaveExitFunc  exit_func;
} GHubSlave;

G_END_DECLS

#endif /* _G_HUB */
