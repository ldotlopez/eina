#ifndef __CLASS_CONF_FILE_H__
#define __CLASS_CONF_FILE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_CONF eina_conf_get_type()
#define EINA_CONF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  EINA_TYPE_CONF, EinaConf))
#define EINA_CONF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  EINA_TYPE_CONF, EinaConfClass))
#define EINA_IS_CONF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  EINA_TYPE_CONF))
#define EINA_IS_CONF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  EINA_TYPE_CONF))
#define EINA_CONF_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  EINA_TYPE_CONF, EinaConfClass))

typedef struct {
	GObject parent;
} EinaConf;

typedef struct {
	GObjectClass parent_class;
	void (*change) (EinaConf *self, const gchar *key);
} EinaConfClass;

GType eina_conf_get_type (void);
EinaConf* eina_conf_new (void);

void eina_conf_set_filename(EinaConf *self, gchar *filename);
const gchar *eina_conf_get_filename(EinaConf *self);

void eina_conf_load(EinaConf *self);
gboolean eina_conf_dump(EinaConf *self);

void eina_conf_set_bool (EinaConf *self, gchar *key, gboolean val);
void eina_conf_set_int  (EinaConf *self, gchar *key, gint val);
void eina_conf_set_float(EinaConf *self, gchar *key, gfloat val);
void eina_conf_set_str  (EinaConf *self, gchar *key, gchar *val);

gboolean     eina_conf_get_bool (EinaConf *self, gchar *key, gboolean def);
gint         eina_conf_get_int  (EinaConf *self, gchar *key, gint def);
gfloat       eina_conf_get_float(EinaConf *self, gchar *key, gfloat def);
const gchar *eina_conf_get_str  (EinaConf *self, gchar *key, const gchar *def);

G_END_DECLS

#endif
