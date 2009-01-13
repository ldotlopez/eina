#ifndef _TEMPLATE_H
#define _TEMPLATE_H

// Gel library (GLib extension library)
#include <gel/gel.h>
// GelIO library (GIO extension library)
#include <gel/gel-io.h>
// GelUI library (Gtk extension library)
#include <gel/gel-ui.h>
// EinaObj is a base pseudo-class which do things more easy
#include <eina/eina-obj.h>

G_BEGIN_DECLS

// Do some defines to make easy to access your plugin from other plugins
#define EINA_TEMPLATE(s)           ((EinaTemplate *) s)
#define GEL_APP_GET_TEMPLATE(app)  EINA_TEMPLATE(gel_app_shared_get(app,"template"))
#define EINA_OBJ_GET_TEMPLATE(obj) GEL_APP_GET_TEMPLATE(eina_obj_get_app(obj))

// Define an opaque struct if you want to expose an API to the world
typedef struct _EinaTemplate EinaTemplate;

// --
// Define your API here
//
// void template_do_something(EinaTemplate *self, ...); 
// --

G_END_DECLS

#endif
