/*
 * gnome-app-toolbar.c: Sample toolbar-system based App implementation
 *
 * This is the toolbar based App implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <gnome.h>
#include "bonobo-app-toolbar.h"

enum {
	STYLE_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
app_toolbar_class_init (BonoboAppToolbarClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	klass->style_changed = NULL;
	signals [STYLE_CHANGED] = gtk_signal_new (
		"style_changed", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboAppToolbarClass, style_changed),
		gtk_marshal_NONE__INT, GTK_TYPE_NONE, 1, GTK_TYPE_INT);

	gtk_object_class_add_signals (
		(GtkObjectClass *)klass, signals, LAST_SIGNAL);
}

static void
app_toolbar_init (BonoboAppToolbar *toolbar)
{
	toolbar->tooltips = gtk_tooltips_new ();
}

/**
 * bonobo_app_toolbar_get_type:
 *
 * Returns the GtkType for the BonoboAppToolbar class.
 */
GtkType
bonobo_app_toolbar_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboAppToolbar",
			sizeof (BonoboAppToolbar),
			sizeof (BonoboAppToolbarClass),
			(GtkClassInitFunc) app_toolbar_class_init,
			(GtkObjectInitFunc) app_toolbar_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (GTK_TYPE_HBOX, &info);
	}

	return type;
}

void
bonobo_app_toolbar_update (BonoboAppToolbar *toolbar,
			   GtkToolbarStyle   style)
{
	gtk_signal_emit (GTK_OBJECT (toolbar),
			 signals [STYLE_CHANGED], style);
}

void
bonobo_app_toolbar_add (BonoboAppToolbar *toolbar,
			GtkWidget        *widget,
			const char       *descr)
{
	gtk_box_pack_end_defaults (GTK_BOX (toolbar), widget);
	if (descr)
		gtk_tooltips_set_tip (toolbar->tooltips, widget,
				      descr, NULL);
}

GtkWidget *
bonobo_app_toolbar_new ()
{
	return gtk_type_new (BONOBO_APP_TOOLBAR_TYPE);
}
