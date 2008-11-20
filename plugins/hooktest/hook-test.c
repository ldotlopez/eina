#define GEL_DOMAIN "Eina::Plugin::HookTest"
#include <eina/plugin.h>

void
on_hooktest_change(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self)
{
	eina_plugin_info("Change from %d to %d, me: %p (%p)", from, to, self, self->data);
}

void
on_hooktest_play(LomoPlayer *lomo, EinaPlugin *self)
{
	eina_plugin_info("Play!  ME: %p", self->data);
}

void
on_hooktest_repeat(LomoPlayer *lomo, gboolean value, EinaPlugin *self)
{
	eina_plugin_info("Repeat: %d (%p)", value, self->data);
}

gboolean
hooktest_init(EinaPlugin *self, GError **error)
{
	eina_plugin_attach_events(self,
		"change", on_hooktest_change,
		"play",   on_hooktest_change,
		"repeat", on_hooktest_repeat,
		NULL);
	eina_plugin_info("Plugin hooktest initialized");
	return TRUE;
}

gboolean
hooktest_exit(EinaPlugin *self, GError **error)
{
	eina_plugin_deattach_events(self,
		"change", on_hooktest_change,
		"play",   on_hooktest_change,
		"repeat", on_hooktest_repeat,
		NULL);
	eina_plugin_info("Plugin hooktest finalized");
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin hooktest_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Hook test"),
	N_("1.0.0"),
	N_("A simple test plugin for hooks"),
	N_("Nothing more to say"),
	NULL,
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net",

	hooktest_init,
	hooktest_exit,

	NULL, NULL
};

