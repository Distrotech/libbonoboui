`/**
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
#include <gdk/gdkprivate.h>

static GnomeObjectClass gnome_view_parent_class;

static void
impl_GNOME_View_size_allocate (PortableServer_Servant servant,
			       const CORBA_Short width,
			       const CORBA_Short height,
			       CORBA_Environment *ev)
{
	g_warning ("GNOME::View::size_allocate invoked\n");
}

GNOME_View_windowid
impl_GNOME_View_get_window (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_object_from_servant (servant));
	GdkWindowPrivate *win = (GdkWindowPrivate *) view->widget->window;

	if (win == NULL){
		g_warning ("Widget is not realized\n");
		return 0;
	}
	return win->xwindow;
}

POA_GNOME_View__epv = {
	NULL,
	&impl_GNOME_View_size_allocate,
	&impl_GNOME_View_get_window
};
	
POA_GNOME_View__vepv gnome_view_vepv = {
	&gnome_object_base_epv,
	&gnome_object_epv,
	&gnome_view_epv
};

static CORBA_Object
create_gnome_view (GnomeObject *object)
{
	POA_GNOME_View *servant;

	servant = g_new0 (POA_GNOME_View, 1);
	servant->vepv = &gnome_view_vepv;

	POA_GNOME_View__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	CORBA_free (PortableServer_POA_activate_object (
		bonobo_poa (), servant, &object->ev));

	o = PortableServer_POA_servant_to_reference (
		bonobo_poa, servant, &object->ev);

	if (o){
		gnome_object_bind_to_servant (object, servant);
		return o;
	} else
		return CORBA_OBJECT_NIL;
}

GnomeObject *
gnome_view_new (GtkWidget *widget)
{
	GnomeObject *view;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	view = gtk_type_new (gnome_view_get_type ());
	
	corba_view = create_gnome_view (view);

	if (corba_view == CORBA_OBJECT_NIL){
		gtk_object_destroy (view);
		return NULL;
	}
	
	GNOME_OBJECT (view)->object = corba_view;
	view->widget = widget;

	gtk_object_ref (GTK_OBJECT (view->widget));
	
	return object;
}

static void
gnome_view_destroy (GtkObject *object)
{
	GnomeView *view = GNOME_VIEW (object);

	gnome_object_drop_binding (object);
	if (view->widget)
		gtk_object_unref (GNOME_OBJECT (view->widget));
	
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
