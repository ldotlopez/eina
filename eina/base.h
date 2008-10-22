#ifndef _BASE_H
#define _BASE_H

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>

G_BEGIN_DECLS

typedef struct EinaBase {
	gchar      *name;
	GelHub     *hub;
	LomoPlayer *lomo;
	GtkBuilder *ui;
} EinaBase;

typedef enum {
	EINA_BASE_NONE,
	EINA_BASE_GTK_UI
} EinaBaseFlag;

gboolean eina_base_init(EinaBase *self, GelHub *hub, gchar *name, EinaBaseFlag flags);
void     eina_base_fini(EinaBase *self);

GelHub*     eina_base_get_hub (EinaBase *self);
LomoPlayer* eina_base_get_lomo(EinaBase *self);
GtkBuilder* eina_base_get_ui  (EinaBase *self);

#define EINA_BASE(s) ((EinaBase *)s)
#define LOMO(s)      ((EinaBase *)s)->lomo
#define HUB(s)       ((EinaBase *)s)->hub
#define UI(s)        ((EinaBase *)s)->ui
#define W(s,n)         W_TYPED(s,GTK_WIDGET,n)
#define W_TYPED(s,t,n) t(gtk_builder_get_object(UI(s),n))

#define OBJ(s,n)     G_OBJECT(gtk_builder_get_object(UI(s),n))

G_END_DECLS

#endif // _BASE_H

