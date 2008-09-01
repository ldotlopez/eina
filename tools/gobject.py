#! /usr/bin/env python
#
# Ross Burton <ross@burtonini.com>
# Dafydd Harries <daf@rhydd.org>

import cgi, os
import cgitb; cgitb.enable()

# TODO:
# toggle for property skeletons
# signals

h_template = """\
/* %(filename)s.h */

#ifndef %(header_guard)s
#define %(header_guard)s

#include <glib-object.h>

G_BEGIN_DECLS

#define %(package_upper)s_TYPE_%(object_upper)s %(class_lower)s_get_type()

#define %(package_upper)s_%(object_upper)s(obj) \\
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), %(package_upper)s_TYPE_%(object_upper)s, %(class_camel)s))

#define %(package_upper)s_%(object_upper)s_CLASS(klass) \\
  (G_TYPE_CHECK_CLASS_CAST ((klass), %(package_upper)s_TYPE_%(object_upper)s, %(class_camel)sClass))

#define %(package_upper)s_IS_%(object_upper)s(obj) \\
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), %(package_upper)s_TYPE_%(object_upper)s))

#define %(package_upper)s_IS_%(object_upper)s_CLASS(klass) \\
  (G_TYPE_CHECK_CLASS_TYPE ((klass), %(package_upper)s_TYPE_%(object_upper)s))

#define %(package_upper)s_%(object_upper)s_GET_CLASS(obj) \\
  (G_TYPE_INSTANCE_GET_CLASS ((obj), %(package_upper)s_TYPE_%(object_upper)s, %(class_camel)sClass))

typedef struct {
  %(parent_camel)s parent;
} %(class_camel)s;

typedef struct {
  %(parent_camel)sClass parent_class;
} %(class_camel)sClass;

GType %(class_lower)s_get_type (void);

%(class_camel)s* %(class_lower)s_new (void);

G_END_DECLS

#endif /* %(header_guard)s */
"""

c_template = """\
/* %(filename)s.c */

#include "%(filename)s.h"

G_DEFINE_TYPE (%(class_camel)s, %(class_lower)s, %(parent)s)

%(extra)s
%(class_init)s

static void
%(class_lower)s_init (%(class_camel)s *self)
{
}

%(class_camel)s*
%(class_lower)s_new (void)
{
  return g_object_new (%(package_upper)s_TYPE_%(object_upper)s, NULL);
}
"""

private_template = """\
#define GET_PRIVATE(o) \\
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), %(package_upper)s_TYPE_%(object_upper)s, %(class_camel)sPrivate))

typedef struct _%(class_camel)sPrivate %(class_camel)sPrivate;

struct _%(class_camel)sPrivate {
};
"""

prop_template = """\
static void
%(class_lower)s_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
%(class_lower)s_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}
"""

dispose_template = """\
static void
%(class_lower)s_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (%(class_lower)s_parent_class)->dispose)
    G_OBJECT_CLASS (%(class_lower)s_parent_class)->dispose (object);
}
"""

finalize_template = """\
static void
%(class_lower)s_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (%(class_lower)s_parent_class)->finalize)
    G_OBJECT_CLASS (%(class_lower)s_parent_class)->finalize (object);
}
"""

def make_class_init(data):
    lines = [
        'static void',
        '%(class_lower)s_class_init (%(class_camel)sClass *klass)',
        '{'
        ]

    if data['dispose'] or data['finalize'] or data['private']:
        lines.append('  GObjectClass *object_class = G_OBJECT_CLASS (klass);')
        lines.append('')

    if data['private']:
        lines.append(
            '  g_type_class_add_private (klass, '
            'sizeof (%(class_camel)sPrivate));')

    if data['dispose'] or data['finalize'] or data['props']:
        lines.append('')

    if data['props']:
        lines.append('  object_class->get_property = %(class_lower)s_get_property;')
        lines.append('  object_class->set_property = %(class_lower)s_set_property;')
    
    if data['dispose']:
        lines.append('  object_class->dispose = %(class_lower)s_dispose;')

    if data['finalize']:
        lines.append('  object_class->finalize = %(class_lower)s_finalize;')

    lines.append('}')
    return ''.join([line % data + '\n' for line in lines])

def handle_get():
    print "Content-Type: text/html\n\n"
    print """
    <?xml version="1.0"?>
    <html>
    <head>
    <title>GObject Generator</title>
    </head>
    <body>
    <form method="post" action="">
    <table>
    <tbody>
        <tr>
            <td>Full class name (<code>CamelCase</code>)</td>
            <td><input name="class_camel" value="GtkWidget" size="30" /></td>
        </tr>
        <tr>
            <td>class name (<code>lower_case</code>)</td>
            <td><input name="class_lower" value="gtk_widget" size="30" /></td>
        </tr>
        <tr>
            <td>Package name (<code>UPPER_CASE</code>)</td>
            <td><input name="package_upper" value="GTK" size="30" /></td>
        </tr>
        <tr>
            <td>Object name (<code>UPPER_CASE</code>)</td>
            <td><input name="object_upper" value="WIDGET" size="30" /></td>
        </tr>
        <tr>
            <td>Parent (<code>TYPE_NAME</code>)</td>
            <td><input name="parent" value="G_TYPE_OBJECT" size="30" /></td>
        </tr>
        <tr>
            <td>Parent (<code>CamelCase</code>)</td>
            <td><input name="parent_camel" value="GObject" size="30" /></td>
        </tr>
        <tr>
            <td>Include private struct?</td>
            <td><input type="checkbox" name="private" /></td>
        </tr>
        <tr>
            <td>Include GObject property get/set?</td>
            <td><input type="checkbox" name="props" /></td>
        </tr>
        <tr>
            <td>Include finalize?</td>
            <td><input type="checkbox" name="finalize" /></td>
        </tr>
        <tr>
            <td>Include dispose?</td>
            <td><input type="checkbox" name="dispose" /></td>
        </tr>
    </tbody>
    </table>

    <button type="submit">Generate</button>
    </form>
    </body>
    </html>
    """

def handle_post():
    print "Content-Type: text/plain\n\n"
    form = cgi.FieldStorage()
    keys = ("class_camel", "class_lower", "package_upper", "object_upper",
        "parent", "parent_camel", "props", "finalize", "dispose", "private");
    data = {}

    for key in keys:
        # TODO: sanity check against nulls
        data[key] = form.getfirst(key)

    data['filename'] = data['class_lower'].replace('_', "-")
    data['class_init'] = make_class_init(data).strip()
    data['header_guard'] = "_" + data['filename'].upper().replace('.', '_').replace('-', '_')
    extra = []

    if data['private']:
        extra.append(private_template)

    if data['props']:
        extra.append(prop_template)

    if data['dispose']:
        extra.append(dispose_template)

    if data['finalize']:
        extra.append(finalize_template)

    data['extra'] = '\n'.join([x % data for x in extra])

    print h_template % data
    print c_template % data

def main():
    if os.environ['REQUEST_METHOD'] == "POST":
        handle_post()
    else:
        handle_get()

main()
