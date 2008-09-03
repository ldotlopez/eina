#ifndef _EINA_BASE_H
#define _EINA_BASE_H

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <liblomo/player.h>
#include <gel/gel.h>

G_BEGIN_DECLS

/* Base type for all components/plugins */
typedef struct EinaBase {
	gchar      *name;
	GelHub       *hub;
	LomoPlayer *lomo;
	GtkBuilder *ui;
} EinaBase;

typedef enum {
	EINA_BASE_NONE,
	EINA_BASE_GTK_UI
} EinaBaseFlag;

gboolean
eina_base_init(EinaBase *self, GelHub *hub, gchar *name, EinaBaseFlag flags);
void
eina_base_fini(EinaBase *self);

GelHub*
eina_base_get_hub (EinaBase *self);
LomoPlayer*
eina_base_get_lomo(EinaBase *self);
GtkBuilder*
eina_base_get_ui  (EinaBase *self);

#define EINA_BASE(s) ((EinaBase *)s)
#define LOMO(s)      ((EinaBase *)s)->lomo
#define HUB(s)       ((EinaBase *)s)->hub
#define UI(s)        ((EinaBase *)s)->ui
#define W(s,n)       ((GtkWidget *)gtk_builder_get_object(UI(s),n))
#define OBJ(s,n)     ((GtkObject *)gtk_builder_get_object(UI(s),n))

/*
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif
*/
G_END_DECLS

#endif // _EINA_BASE_H

