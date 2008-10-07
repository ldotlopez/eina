#define GEL_DOMAIN "Eina::EinaPreferencesDialog"
#include "eina-preferences-dialog.h"
#include <gel/gel.h>

#define EINA_HIG_BOX_SPACING 5

G_DEFINE_TYPE (EinaPreferencesDialog, eina_preferences_dialog, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogPrivate))

typedef struct _EinaPreferencesDialogPrivate EinaPreferencesDialogPrivate;

struct _EinaPreferencesDialogPrivate {
	GtkNotebook *notebook;
};

static void
eina_preferences_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose (object);
}

static void
eina_preferences_dialog_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize (object);
}

static void
eina_preferences_dialog_class_init (EinaPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPreferencesDialogPrivate));

	object_class->dispose = eina_preferences_dialog_dispose;
	object_class->finalize = eina_preferences_dialog_finalize;
}

static void
eina_preferences_dialog_init (EinaPreferencesDialog *self)
{
}

EinaPreferencesDialog*
eina_preferences_dialog_new (void)
{
	EinaPreferencesDialog *self = g_object_new (EINA_TYPE_PREFERENCES_DIALOG, NULL);
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);

	priv->notebook = (GtkNotebook *) gtk_notebook_new();
	gtk_notebook_set_show_tabs(priv->notebook, FALSE);

	return self;
}

void
eina_preferences_dialog_add_tab(EinaPreferencesDialog *self, GtkImage *icon, GtkLabel *label, GtkWidget *tab)
{
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);

	GtkHBox *box = (GtkHBox *) gtk_hbox_new(FALSE, EINA_HIG_BOX_SPACING);

	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(icon),  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), FALSE, FALSE, 0);

	gtk_notebook_append_page(priv->notebook, GTK_WIDGET(tab), GTK_WIDGET(box));

	if (gtk_notebook_get_n_pages(priv->notebook) > 1)
		gtk_notebook_set_show_tabs(priv->notebook, TRUE);
}

void
eina_preferences_dialog_remove_tab(EinaPreferencesDialog *self, GtkWidget *widget)
{
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	gint n = gtk_notebook_page_num(priv->notebook, widget);
	if (n == -1)
	{
		gel_warn("Widget is not on notebook");
		return;
	}

	gtk_notebook_remove_page(priv->notebook, n);

	if (gtk_notebook_get_n_pages(priv->notebook) <= 1)
		gtk_notebook_set_show_tabs(priv->notebook, FALSE);
}

