/*
 * plugins/hooktest/hook-test.c
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	N_("Hook test"), N_("1.0.0"), NULL,
	"xuzo <xuzo@cuarentaydos.com>", "http://eina.sourceforge.net",

	N_("A simple test plugin for hooks"), N_("Nothing more to say"), NULL,

	hooktest_init, hooktest_exit,

	NULL, NULL, NULL
};

