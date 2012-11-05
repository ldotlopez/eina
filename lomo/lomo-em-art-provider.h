/* lomo-em-art-provider.h */

#ifndef __LOMO_EM_ART_PROVIDER_H__
#define __LOMO_EM_ART_PROVIDER_H__

#include <glib-object.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define LOMO_TYPE_EM_ART_PROVIDER lomo_em_art_provider_get_type()

#define LOMO_EM_ART_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_EM_ART_PROVIDER, LomoEMArtProvider))
#define LOMO_EM_ART_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LOMO_TYPE_EM_ART_PROVIDER, LomoEMArtProviderClass))
#define LOMO_IS_EM_ART_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_EM_ART_PROVIDER))
#define LOMO_IS_EM_ART_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LOMO_TYPE_EM_ART_PROVIDER))
#define LOMO_EM_ART_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LOMO_TYPE_EM_ART_PROVIDER, LomoEMArtProviderClass))

typedef struct _LomoEMArtProviderPrivate LomoEMArtProviderPrivate;
typedef struct {
	/* <private> */
	GObject parent;
	LomoEMArtProviderPrivate *priv;
} LomoEMArtProvider;

typedef struct {
	/* <private> */
	GObjectClass parent_class;
} LomoEMArtProviderClass;

void lomo_em_art_provider_set_default_cover(const gchar *default_uri);
void lomo_em_art_provider_set_loading_cover(const gchar *loading_uri);

const gchar *lomo_em_art_provider_get_default_cover(void);
const gchar *lomo_em_art_provider_get_loading_cover(void);

/**
 * LOMO_STREAM_EM_ART_DATA:
 *
 * Defines the key for extended metadata
 */
#define LOMO_STREAM_EM_ART_DATA "art-data"

#define lomo_em_art_value_equal_to(v,x) \
	((v && G_VALUE_HOLDS_STRING(v)) ? g_str_equal(g_value_get_string(v), x) : FALSE)

#define lomo_em_art_provider_value_is_default(v) \
	lomo_em_art_value_equal_to(v, lomo_em_art_provider_get_default_cover())

#define lomo_em_art_provider_value_is_loading(v) \
	lomo_em_art_value_equal_to(v, lomo_em_art_provider_get_loading_cover())

GType lomo_em_art_provider_get_type (void);

LomoEMArtProvider* lomo_em_art_provider_new (void);
void               lomo_em_art_provider_set_player(LomoEMArtProvider *self, LomoPlayer *lomo);

void               lomo_em_art_provider_init_stream(LomoEMArtProvider *self, LomoStream *stream);

G_END_DECLS

#endif /* __LOMO_EM_ART_PROVIDER_H__ */
