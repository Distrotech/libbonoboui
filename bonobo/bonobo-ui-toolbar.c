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
#include "bonobo-app-item.h"

enum {
	GEOMETRY_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
app_toolbar_class_init (BonoboAppToolbarClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	/* FIXME: Emit this when we change shape significantly */
	klass->geometry_changed = NULL;
	signals [GEOMETRY_CHANGED] = gtk_signal_new (
		"geometry_changed", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboAppToolbarClass, geometry_changed),
		gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (
		(GtkObjectClass *)klass, signals, LAST_SIGNAL);
}

static void
app_toolbar_init (BonoboAppToolbar *toolbar)
{
	toolbar->tooltips = gtk_tooltips_new ();

	if (gnome_preferences_get_toolbar_relief_btn ())
		toolbar->relief = GTK_RELIEF_NORMAL;
	else
		toolbar->relief = GTK_RELIEF_NONE;

	if (gnome_preferences_get_toolbar_labels ())
		toolbar->look = GTK_TOOLBAR_BOTH;
	else
		toolbar->look = GTK_TOOLBAR_ICONS;

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

static void
bonobo_app_toolbar_update_style (BonoboAppToolbar *toolbar)
{
	GList *l;
	GList *children =
		gtk_container_children (GTK_CONTAINER (toolbar));
	GtkWidget *widget;

	for (l = children; l; l = l->next) {
		GtkWidget *widget = l->data;

		if (BONOBO_IS_APP_ITEM (widget))
			bonobo_app_item_set_style (BONOBO_APP_ITEM (widget),
						   toolbar->relief,
						   toolbar->look);
	}

	g_list_free (children);
	
	if (!(widget = GTK_WIDGET (toolbar)->parent))
		widget = GTK_WIDGET (toolbar);

	gtk_widget_queue_resize (widget);
}

void
bonobo_app_toolbar_set_relief (BonoboAppToolbar *toolbar,
			       GtkReliefStyle    relief)
{
	g_return_if_fail (toolbar != NULL);

	toolbar->relief = relief;
	bonobo_app_toolbar_update_style (toolbar);
}

void
bonobo_app_toolbar_set_style  (BonoboAppToolbar *toolbar,
			       GtkToolbarStyle   look)
{
	g_return_if_fail (toolbar != NULL);

	toolbar->look = look;
	bonobo_app_toolbar_update_style (toolbar);
}

