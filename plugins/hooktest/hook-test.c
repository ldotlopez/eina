#define GEL_DOMAIN "Eina::Plugin::HookTest"
#include <eina/iface.h>

void
on_hooktest_change(LomoPlayer *lomo, gint from, gint to, EinaPluginV2 *self)
{
	eina_iface_info("Change from %d to %d, me: %p (%p)", from, to, self, self->data);
}

void
on_hooktest_play(LomoPlayer *lomo, EinaPluginV2 *self)
{
	eina_iface_info("Play!  ME: %p", self->data);
}

void
on_hooktest_repeat(LomoPlayer *lomo, gboolean value, EinaPluginV2 *self)
{
	eina_iface_info("Repeat: %d (%p)", value, self->data);
}

gboolean
hooktest_init(EinaPluginV2 *self, GError **error)
{
	eina_plugin_attach_events(self,
		"change", on_hooktest_change,
		"play",   on_hooktest_change,
		"repeat", on_hooktest_repeat,
		NULL);
	eina_iface_info("Plugin hooktest initialized");
	return TRUE;
}

gboolean
hooktest_exit(EinaPluginV2 *self, GError **error)
{
	eina_plugin_deattach_events(self,
		"change", on_hooktest_change,
		"play",   on_hooktest_change,
		"repeat", on_hooktest_repeat,
		NULL);
	eina_iface_info("Plugin hooktest finalized");
	return TRUE;
}

G_MODULE_EXPORT EinaPluginV2 hooktest_plugin = {
	N_("Hook test"),
	N_("A simple test plugin for hooks"),
	N_("Nothing more to say"),
	NULL,
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net",

	hooktest_init,
	hooktest_exit,

	NULL, NULL
};

