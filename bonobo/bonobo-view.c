/**
 * GNOME view object
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-view.h>
#include <gdk/gdkprivate.h>

/* Parent object class in GTK hierarchy */
static GnomeUnknownClass *gnome_view_parent_class;

/* The entry point vectors for the server we provide */
static POA_GNOME_View__epv gnome_view_epv;
static POA_GNOME_View__vepv gnome_view_vepv;

enum {
	DO_VERB,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL];

static void
impl_GNOME_View_do_verb (PortableServer_Servant servant,
				 const CORBA_char *verb_name,
				 CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_unknown_from_servant (servant));

	gtk_signal_emit (
		GTK_OBJECT (view),
		view_signals [DO_VERB],
		(gchar *) verb_name);
}

static void
impl_GNOME_View_size_allocate (PortableServer_Servant servant,
			       const CORBA_short width,
			       const CORBA_short height,
			       CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_unknown_from_servant (servant));
	GtkAllocation allocation;

	allocation.x = view->plug->allocation.x;
	allocation.y = view->plug->allocation.y;
	allocation.width = width;
	allocation.height = height;
	
	gtk_widget_size_allocate (view->plug, &allocation);
}

static void
impl_GNOME_View_set_window (PortableServer_Servant servant, GNOME_View_windowid id, CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_unknown_from_servant (servant));
	GdkWindowPrivate *win;

	view->plug = gtk_plug_new (id);

	gtk_widget_show_all (view->plug);

	gtk_container_add (GTK_CONTAINER (view->plug), view->widget);
}

static CORBA_Object
create_gnome_view (GnomeUnknown *object)
{
	POA_GNOME_View *servant;
	CORBA_Object o;
	
	servant = (POA_GNOME_View *) g_new0 (GnomeUnknownServant, 1);
	servant->vepv = &gnome_view_vepv;

	POA_GNOME_View__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_unknown_activate_servant (object, servant);
	
}

GnomeView *
gnome_view_construct (GnomeView *view, GNOME_View corba_view, GtkWidget *widget)
{
	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW (view), NULL);
	g_return_val_if_fail (corba_view != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	gnome_unknown_construct (GNOME_UNKNOWN (view), corba_view);
	
	view->widget = widget;

	gtk_object_ref (GTK_OBJECT (view->widget));
	
	return view;
}

/**
 * gnome_view_new:
 * @widget: a GTK widget that represents the view to pass to the container process
 *
 * Returns a GnomeView object that implements the GNOME::View CORBA service that
 * will transfer the @widget to the container process.
 */
GnomeView *
gnome_view_new (GtkWidget *widget)
{
	GNOME_View corba_view;
	GnomeView *view;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	view = gtk_type_new (gnome_view_get_type ());

	corba_view = create_gnome_view (GNOME_UNKNOWN (view));
	if (corba_view == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (view));
		return NULL;
	}
	

	return gnome_view_construct (view, corba_view, widget);
}

static void
gnome_view_destroy (GtkObject *object)
{
	GnomeView *view = GNOME_VIEW (object);

	if (view->widget)
		gtk_object_unref (GTK_OBJECT (view->widget));

	GTK_OBJECT_CLASS (gnome_view_parent_class)->destroy (object);
}

static void
init_view_corba_class (void)
{
	/* The entry point vectors for this GNOME::View class */
	gnome_view_epv.size_allocate = impl_GNOME_View_size_allocate;
	gnome_view_epv.set_window = impl_GNOME_View_set_window;
	gnome_view_epv.do_verb = impl_GNOME_View_do_verb;

	/* Setup the vector of epvs */
	gnome_view_vepv.GNOME_Unknown_epv = &gnome_unknown_epv;
	gnome_view_vepv.GNOME_View_epv = &gnome_view_epv;
}

static void
gnome_view_class_init (GnomeViewClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_view_parent_class = gtk_type_class (gnome_unknown_get_type ());

	view_signals [DO_VERB] =
                gtk_signal_new ("do_verb",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeViewClass, do_verb), 
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, view_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_view_destroy;

	init_view_corba_class ();
}

static void
gnome_view_init (GnomeUnknown *object)
{
}

GtkType
gnome_view_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/View:1.0",
			sizeof (GnomeView),
			sizeof (GnomeViewClass),
			(GtkClassInitFunc) gnome_view_class_init,
			(GtkObjectInitFunc) gnome_view_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_unknown_get_type (), &info);
	}

	return type;
}







