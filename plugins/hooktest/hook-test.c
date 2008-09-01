#define GEL_DOMAIN "Eina::Plugin::HookTest"
#include <eina/iface.h>

void
on_hooktest_change(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self)
{
	eina_iface_info("Change from %d to %d, me: %p (%p)", from, to, self, self->data);
}

void
on_hooktest_play(LomoPlayer *lomo, EinaPlugin *self)
{
	eina_iface_info("Play!  ME: %p", self->data);
}

void
on_hooktest_repeat(LomoPlayer *lomo, gboolean value, EinaPlugin *self)
{
	eina_iface_info("Repeat: %d (%p)", value, self->data);
}

EINA_PLUGIN_FUNC gboolean
hooktest_exit(EinaPlugin *self)
{
	eina_iface_info("Plugin hooktest unloaded");
	eina_plugin_free(self);
	return TRUE;
}

EINA_PLUGIN_FUNC EinaPlugin*
hooktest_init(GelHub *app, EinaIFace *iface)
{
	EinaPlugin *self = eina_plugin_new(iface,
		"hooktest", "test-cap", (gpointer) 0xdeadbeaf, hooktest_exit,
		NULL, NULL, NULL);
	
	self->change = on_hooktest_change;
	self->play   = on_hooktest_play;
	self->repeat = on_hooktest_repeat;

	return self;
}

