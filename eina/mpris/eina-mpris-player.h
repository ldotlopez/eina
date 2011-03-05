
/* eina-mpris-player.h */

#ifndef _EINA_MPRIS_PLAYER
#define _EINA_MPRIS_PLAYER

#include <glib-object.h>
#include <eina/ext/eina-application.h>

G_BEGIN_DECLS

#define EINA_TYPE_MPRIS_PLAYER            eina_mpris_player_get_type()
#define EINA_MPRIS_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayer))
#define EINA_MPRIS_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerClass))
#define EINA_IS_MPRIS_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_MPRIS_PLAYER))
#define EINA_IS_MPRIS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_MPRIS_PLAYER))
#define EINA_MPRIS_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerClass))

typedef struct _EinaMprisPlayerPrivate EinaMprisPlayerPrivate;
typedef struct {
	GObject parent;
	EinaMprisPlayerPrivate *priv;
} EinaMprisPlayer;

typedef struct {
	GObjectClass parent_class;
} EinaMprisPlayerClass;

GType eina_mpris_player_get_type (void);

EinaMprisPlayer* eina_mpris_player_new (EinaApplication *app, const gchar *bus_name_suffix);

EinaApplication*  eina_mpris_player_get_application(EinaMprisPlayer *self);
const gchar*      eina_mpris_player_get_bus_name_suffix(EinaMprisPlayer *self);

#if 0
gboolean     eina_mpris_player_get_can_quit(EinaMprisPlayer *self);
gboolean     eina_mpris_player_get_can_raise(EinaMprisPlayer *self);
gboolean     eina_mpris_player_get_has_track_list(EinaMprisPlayer *self);
const gchar* eina_mpris_player_get_identify(EinaMprisPlayer *self);
const gchar* eina_mpris_player_get_desktop_entry(EinaMprisPlayer *self);
const gchar* const *
             eina_mpris_player_get_supported_uri_schemes(EinaMprisPlayer *self);
const gchar* const *
             eina_mpris_player_get_supported_mime_types(EinaMprisPlayer *self);

void eina_mpris_player_raise(EinaMprisPlayer *self);
void eina_mpris_player_quit (EinaMprisPlayer *self);
#endif

G_END_DECLS

#endif /* _EINA_MPRIS_PLAYER */