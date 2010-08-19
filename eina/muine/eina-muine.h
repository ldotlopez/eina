#ifndef _EINA_MUINE
#define _EINA_MUINE

#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>
#include <eina/adb/eina-adb.h>
#include <eina/art/eina-art.h>

G_BEGIN_DECLS

#define EINA_TYPE_MUINE eina_muine_get_type()

#define EINA_MUINE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_MUINE, EinaMuine))

#define EINA_MUINE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_MUINE, EinaMuineClass))

#define EINA_IS_MUINE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_MUINE))

#define EINA_IS_MUINE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_MUINE))

#define EINA_MUINE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_MUINE, EinaMuineClass))

typedef struct {
	GelUIGeneric parent;
} EinaMuine;

typedef struct {
	GelUIGenericClass parent_class;
} EinaMuineClass;

typedef enum {
	EINA_MUINE_MODE_INVALID = -1,
	EINA_MUINE_MODE_ALBUM   = 0,
	EINA_MUINE_MODE_ARTIST
} EinaMuineMode;

GType eina_muine_get_type (void);

EinaMuine* eina_muine_new (void);

void
eina_muine_set_adb(EinaMuine *self, EinaAdb *adb);
EinaAdb*
eina_muine_get_adb(EinaMuine *self);

void
eina_muine_set_lomo_player(EinaMuine *self, LomoPlayer *lomo);
LomoPlayer*
eina_muine_get_lomo_player(EinaMuine *lomo);

void
eina_muine_set_art(EinaMuine *self, EinaArt *art);
EinaArt*
eina_muine_get_art(EinaMuine *self);

void
eina_muine_set_mode(EinaMuine *self, EinaMuineMode mode);
EinaMuineMode
eina_muine_get_mode(EinaMuine *self);


G_END_DECLS

#endif /* _EINA_MUINE */

