#include "eina-cover-clutter.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaCoverClutter, eina_cover_clutter, GTK_CLUTTER_TYPE_EMBED)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_CLUTTER, EinaCoverClutterPrivate))

#define EFFECT_DURATION 1000
#define ZOOM_IN_MODE  CLUTTER_LINEAR
#define ZOOM_OUT_MODE CLUTTER_LINEAR
#define SPIN_MODE     CLUTTER_LINEAR

typedef struct _EinaCoverClutterPrivate EinaCoverClutterPrivate;

enum { CURR, NEXT, N_PBS };
enum { SPIN, ZOOM_IN, ZOOM_OUT, N_EFF };

struct _EinaCoverClutterPrivate {
	ClutterActor *stage;
	ClutterActor *texture;

	ClutterAnimation *anims[N_EFF];
	GdkPixbuf *pbs[N_PBS];
};

enum {
	PROPERTY_COVER = 1
};

static void
actor_print_size(ClutterActor *actor)
{	
	gfloat h, w;
	clutter_actor_get_size(actor, &w, &h);
	g_printf("== Actor %p: %fx%f\n", actor, w, h);
}

static gboolean
configure_event_cb(GtkWidget *self, GdkEventConfigure *ev, gpointer data);
static void
hierarchy_changed_cb(GtkWidget *self, GtkWidget *prev_tl, gpointer data);

static void
animation_completed_cb(ClutterAnimation *animation, EinaCoverClutter *self);

static void
eina_cover_clutter_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_COVER:
		eina_cover_clutter_set_from_pixbuf(EINA_COVER_CLUTTER(object), GDK_PIXBUF(g_value_get_object(value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_cover_clutter_parent_class)->dispose (object);
}

static void
eina_cover_clutter_class_init (EinaCoverClutterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCoverClutterPrivate));

	object_class->get_property = eina_cover_clutter_get_property;
	object_class->set_property = eina_cover_clutter_set_property;
	object_class->dispose = eina_cover_clutter_dispose;

    g_object_class_install_property(object_class, PROPERTY_COVER,
		g_param_spec_object("cover", "Cover", "Cover pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	gint r = gtk_clutter_init(NULL, NULL);
	if (r != CLUTTER_INIT_SUCCESS)
		g_warning("Unable to init clutter-gtk: %d", r);
}

static void
eina_cover_clutter_init (EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	priv->texture = clutter_texture_new();
	clutter_container_add_actor((ClutterContainer *) gtk_clutter_embed_get_stage((GtkClutterEmbed *) self), priv->texture);

	g_signal_connect(self, "configure-event",   (GCallback) configure_event_cb, NULL);
	g_signal_connect(self, "hierarchy-changed", (GCallback) hierarchy_changed_cb, NULL);
}

EinaCoverClutter*
eina_cover_clutter_new (void)
{
	return g_object_new (EINA_TYPE_COVER_CLUTTER, NULL);
}

static void
start_animation(EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);
	if (!gtk_widget_get_visible((GtkWidget *) self) || !priv->texture ||
		priv->anims[SPIN] || priv->anims[ZOOM_IN] || priv->anims[ZOOM_OUT])
		return;
	
	priv->anims[ZOOM_OUT] = clutter_actor_animate(priv->texture, ZOOM_OUT_MODE, EFFECT_DURATION / 2,
		"scale-x", 0.5,
		"scale-y", 0.5,
		"signal-after::completed", animation_completed_cb, self,
		NULL);
	priv->anims[SPIN] = clutter_actor_animate(priv->texture, SPIN_MODE, EFFECT_DURATION,
		"rotation-angle-y", 180.0,
		"signal-after::completed", animation_completed_cb, self,
		NULL);

}

void
eina_cover_clutter_set_from_pixbuf(EinaCoverClutter *self, GdkPixbuf *pixbuf)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(pixbuf != NULL);

#if 1
	if (priv->pbs[CURR])
		g_object_unref(priv->pbs[CURR]);
	priv->pbs[CURR] = pixbuf;
	gtk_widget_queue_resize((GtkWidget *) self);
	start_animation(self);
	return;
#else
	// Put on next?
	if (priv->pbs[CURR])
	{
		if (priv->pbs[NEXT])
			g_object_unref(priv->pbs[NEXT]);
		priv->pbs[NEXT] = pixbuf;
		start_animation(self);
		return;
	}
	else
	{
		priv->pbs[CURR] = pixbuf;
		gtk_widget_queue_resize((GtkWidget *) self);
	}
#endif
}

static gboolean
configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, gpointer data)
{
	EinaCoverClutter *self = EINA_COVER_CLUTTER(widget);
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	if (priv->pbs[CURR] == NULL)
		return FALSE;

	if ((ev->width <= 1) || (ev->height <= 1))
		return FALSE;

	// gint pbw = gdk_pixbuf_get_width (priv->pbs[CURR]);
	// gint pbh = gdk_pixbuf_get_height(priv->pbs[CURR]);
	// clutter_actor_set_size(priv->texture, pbw, pbh);
	gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->texture, priv->pbs[CURR], NULL);
	clutter_actor_set_size(priv->texture, ev->width, ev->height);

	clutter_actor_set_anchor_point_from_gravity(priv->texture, CLUTTER_GRAVITY_CENTER);
	clutter_actor_set_position(priv->texture, ev->width / (gfloat) 2, ev->width / (gfloat) 2);

	actor_print_size((ClutterActor *) gtk_clutter_embed_get_stage((GtkClutterEmbed *) self));
	actor_print_size(priv->texture);
	return FALSE;
}

static void
hierarchy_changed_cb(GtkWidget *self, GtkWidget *prev_tl, gpointer data)
{
	if (prev_tl != NULL)
		return;

	GdkColor gc = gtk_widget_get_style(gtk_widget_get_parent(self))->bg[GTK_STATE_NORMAL];
	ClutterStage *stage = (ClutterStage *) gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self));
	ClutterColor cc = { gc.red / 255, gc.green / 255, gc.blue / 255, 255 };
	clutter_stage_set_color(stage, &cc);
}

static void
animation_completed_cb(ClutterAnimation *animation, EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);
	g_warning("Animation %p completed", animation);
	if (animation == priv->anims[ZOOM_OUT])
	{
		priv->anims[ZOOM_OUT] = NULL;
		priv->anims[ZOOM_IN] = clutter_actor_animate(priv->texture, ZOOM_IN_MODE, EFFECT_DURATION / 2,
			"scale-x", 1.0,
			"scale-y", 1.0,
			"signal-after::completed", animation_completed_cb, self,
			NULL);
	}
	else if (animation == priv->anims[ZOOM_IN])
		priv->anims[ZOOM_IN] = NULL;
	else if (animation == priv->anims[SPIN])
	{
		priv->anims[SPIN] = NULL;
		clutter_actor_set_rotation(priv->texture, CLUTTER_Y_AXIS, 0.0, 0, 1, 0);
	}
}

#if 0
static void
mark_reached_cb(ClutterTimeline *tl, gchar *marker, gint msecs, EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	gdouble sx, sy;
	clutter_actor_get_scale(priv->texture, &sx, &sy);
	g_warning("Got half! scale: %f,%f", sx, sy);
}
#endif
