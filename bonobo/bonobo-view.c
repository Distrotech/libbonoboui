/**
 * GNOME view object
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkwidget.h>
#include "gnome-object.h"
#include "gnome-view.h"

static GnomeObjectClass gnome_view_parent_class;

static void
gnome_view_destroy (GtkObject *object)
{
	gnome_view_parent_class->destroy (object);
}

static void
gnome_view_class_init (GnomeViewClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_view_parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = gnome_view_destroy;
}

static void
gnome_view_init (GnomeObject *object)
{
}

GnomeObject *
gnome_view_new (GtkWidget *widget)
{
	GnomeView *view;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	view = gtk_type_new (gnome_view_get_type ());

 FIXME: Create the GNOME::View servant, set the GnomeObject->object
        reference. 

	view->widget = widget;
	return object;
}

GtkType
gnome_view_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/View:1.0",
			sizeof (GnomeObject),
			sizeof (GnomeObjectClass),
			(GtkClassInitFunc) gnome_view_class_init,
			(GtkObjectInitFunc) gnome_view_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}
