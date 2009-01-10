#ifndef _BASE_H
#define _BASE_H

// #if USE_GEL_APP
#if 0
// Compatibility layer
G_BEGIN_DECLS

#include <eina/eina-obj.h>

typedef EinaObj EinaBase;
typedef EinaObjFlag EinaBaseFlag;
#define EINA_BASE_NONE   EINA_OBJ_NONE
#define EINA_BASE_GTK_UI EINA_OBJ_GTK_UI

#define eina_base_init(self,hub,name,flags) eina_obj_init(self,hub,name,flags,NULL)
#define eina_base_fini(self)                eina_obj_fini(self,NULL)
#define eina_base_require(self,comp)        eina_obj_require(self,comp,NULL)
#define eina_base_get_hub(self)             eina_obj_get_app(self)
#define eina_base_get_lomo(self)            eina_obj_get_lomo(self)
#define eina_base_get_ui(self)              eina_obj_get_ui(self)
#define EINA_BASE(self)                     EINA_OBJ(self)
#define LOMO(self)                          eina_obj_get_lomo(self)
#define HUB(self)                           eina_obj_get_app(self)
#define UI(self)                            eina_obj_get_ui(self)
#define W(self,name)                        eina_obj_get_widget(self,name)
#define W_TYPED(self,type,name)             eina_obj_get_typed(self,type,name)

G_END_DECLS

#else

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

gpointer eina_base_require(EinaBase *self, gchar *component);

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
#endif

#endif // _BASE_H

