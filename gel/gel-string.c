#include "gel-string.h"

struct GelStringPriv {
	guint ref_count;
};

GelString *
gel_string_new(gchar *str)
{
	GelString *self = g_new0(GelString, 1);
	self->priv = g_new0(GelStringPriv,  1);

	if (str != NULL)
		self->str = g_strdup(str);
	self->priv->ref_count = 1;

	return self;
}

void
gel_string_ref(GelString *self)
{
	self->priv->ref_count++;
}

void
gel_string_unref(GelString *self)
{
	self->priv->ref_count--;

	if (self->priv->ref_count > 0)
		return;

	g_free(self->priv);
	g_free(self->str);
	g_free(self);
}

