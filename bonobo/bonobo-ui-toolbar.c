/*
 * bonobo-ui-toolbar.c: To be clever toolbar, waiting for Ettore.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#include <config.h>
#include <gnome.h>

#include <bonobo/bonobo-ui-toolbar.h>
#include <bonobo/bonobo-ui-item.h>

enum {
	GEOMETRY_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
ui_toolbar_class_init (BonoboUIToolbarClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	/* FIXME: Emit this when we change shape significantly */
	klass->geometry_changed = NULL;
	signals [GEOMETRY_CHANGED] = gtk_signal_new (
		"geometry_changed", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIToolbarClass, geometry_changed),
		gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (
		(GtkObjectClass *)klass, signals, LAST_SIGNAL);
}

static void
ui_toolbar_init (BonoboUIToolbar *toolbar)
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
 * bonobo_ui_toolbar_get_type:
 *
 * Returns the GtkType for the BonoboUIToolbar class.
 */
GtkType
bonobo_ui_toolbar_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboUIToolbar",
			sizeof (BonoboUIToolbar),
			sizeof (BonoboUIToolbarClass),
			(GtkClassInitFunc) ui_toolbar_class_init,
			(GtkObjectInitFunc) ui_toolbar_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (GTK_TYPE_HBOX, &info);
	}

	return type;
}

void
bonobo_ui_toolbar_add (BonoboUIToolbar *toolbar,
			GtkWidget        *widget)
{
	gtk_box_pack_start (GTK_BOX (toolbar), widget,
			    FALSE, FALSE, 0);
}

GtkWidget *
bonobo_ui_toolbar_new ()
{
	return gtk_type_new (BONOBO_UI_TOOLBAR_TYPE);
}

static void
bonobo_ui_toolbar_update_style (BonoboUIToolbar *toolbar)
{
	GList *l;
	GList *children =
		gtk_container_children (GTK_CONTAINER (toolbar));
	GtkWidget *widget;

	for (l = children; l; l = l->next) {
		GtkWidget *widget = l->data;

		if (BONOBO_IS_UI_ITEM (widget))
			bonobo_ui_item_set_style (BONOBO_UI_ITEM (widget),
						   toolbar->relief,
						   toolbar->look);
	}

	g_list_free (children);
	
	if (!(widget = GTK_WIDGET (toolbar)->parent))
		widget = GTK_WIDGET (toolbar);

	gtk_widget_queue_resize (widget);
}

void
bonobo_ui_toolbar_set_relief (BonoboUIToolbar *toolbar,
			       GtkReliefStyle    relief)
{
	g_return_if_fail (toolbar != NULL);

	toolbar->relief = relief;
	bonobo_ui_toolbar_update_style (toolbar);
}

void
bonobo_ui_toolbar_set_style  (BonoboUIToolbar *toolbar,
			       GtkToolbarStyle   look)
{
	g_return_if_fail (toolbar != NULL);

	toolbar->look = look;
	bonobo_ui_toolbar_update_style (toolbar);
}

void
bonobo_ui_toolbar_set_tooltips (BonoboUIToolbar *toolbar,
				 gboolean          enable)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar));

	if (enable)
		gtk_tooltips_enable (toolbar->tooltips);
	else
		gtk_tooltips_disable (toolbar->tooltips);
}

GtkTooltips *
bonobo_ui_toolbar_get_tooltips (BonoboUIToolbar *toolbar)
{
	g_return_val_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar), NULL);

	return toolbar->tooltips;
}

void
bonobo_ui_toolbar_set_homogeneous (BonoboUIToolbar *toolbar,
				   gboolean         homogeneous)
{
	g_warning ("FIXME: implement homogeneity sensibly");
}
