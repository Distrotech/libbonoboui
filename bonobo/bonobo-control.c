/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME control object
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-control.h>
#include <gdk/gdkprivate.h>

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_control_parent_class;

/* The entry point vectors for the server we provide */
POA_GNOME_Control__epv gnome_control_epv;
POA_GNOME_Control__vepv gnome_control_vepv;

enum {
	SIZE_QUERY,
	LAST_SIGNAL
};

static guint control_signals [LAST_SIGNAL];

struct _GnomeControlPrivate {
	GtkWidget *plug;

	int plug_destroy_id;

	GtkWidget  *widget;
};

static void
impl_GNOME_Control_size_allocate (PortableServer_Servant servant,
				  const CORBA_short width,
				  const CORBA_short height,
				  CORBA_Environment *ev)
{
	GnomeControl *control = GNOME_CONTROL (gnome_object_from_servant (servant));
	GtkAllocation allocation;

	if (control->priv->plug == NULL)
		return;

	allocation.x = control->priv->plug->allocation.x;
	allocation.y = control->priv->plug->allocation.y;

	allocation.width = width;
	allocation.height = height;
	gtk_widget_size_allocate (control->priv->plug, &allocation);
}

/*
 * This callback is invoked when the plug is unexpectedly destroyed.
 * This may happen if, for example, the container application goes
 * away.  This callback is _not_ invoked if the GnomeControl is destroyed
 * normally, i.e. the user unrefs the GnomeControl away.
 */
static gint
plug_destroy_cb (GtkWidget *plug, GdkEventAny *event, gpointer closure)
{
	GnomeControl *control = GNOME_CONTROL (closure);

	if (control->priv->plug != plug)
		g_warning ("Destroying incorrect plug");

	/*
	 * Set the plug to NULL here so that we don't try to
	 * destroy it later.  It will get destroyed on its
	 * own.
	 */
	control->priv->plug = NULL;
	gtk_signal_disconnect (GTK_OBJECT (plug), control->priv->plug_destroy_id);
        /*
         * Destroy this plug's GnomeControl.
         */
	gnome_object_destroy (GNOME_OBJECT (control));

	return FALSE;
}

static void
impl_GNOME_Control_set_window (PortableServer_Servant servant,
			       GNOME_Control_windowid id,
			       CORBA_Environment *ev)
{
	GnomeControl *control = GNOME_CONTROL (gnome_object_from_servant (servant));

	control->priv->plug = gtk_plug_new (id);
	control->priv->plug_destroy_id = gtk_signal_connect (
		GTK_OBJECT (control->priv->plug), "delete_event",
		GTK_SIGNAL_FUNC (plug_destroy_cb), control);

	gtk_widget_show_all (control->priv->plug);

	gtk_container_add (GTK_CONTAINER (control->priv->plug), control->priv->widget);
}

static void
impl_GNOME_Control_size_query (PortableServer_Servant servant,
			       CORBA_short *desired_width,
			       CORBA_short *desired_height,
			       CORBA_Environment *ev)
{
	GnomeControl *control = GNOME_CONTROL (gnome_object_from_servant (servant));
	int dh = -1;
	int dw = -1;
	
	gtk_signal_emit (
		GTK_OBJECT (control),
		control_signals [SIZE_QUERY], &dw, &dh);

	*desired_height = dh;
	*desired_width = dw;
}

/**
 * gnome_control_corba_object_create:
 * @object: the GtkObject that will wrap the CORBA object
 *
 * Creates and activates the CORBA object that is wrapped by the
 * @object GnomeObject.
 *
 * Returns: An activated object reference to the created object
 * or %CORBA_OBJECT_NIL in case of failure.
 */
GNOME_Control
gnome_control_corba_object_create (GnomeObject *object)
{
	POA_GNOME_Control *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_Control *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_control_vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_Control__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (GNOME_Control) gnome_object_activate_servant (object, servant);
}

GnomeControl *
gnome_control_construct (GnomeControl *control, GNOME_Control corba_control, GtkWidget *widget)
{
	g_return_val_if_fail (control != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTROL (control), NULL);
	g_return_val_if_fail (corba_control != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	/*
	 * This sets up the X handler for Bonobo objects.  We basically will
	 * ignore X errors if our container dies (because X will kill the
	 * windows of the container and our container without telling us).
	 */
	bonobo_setup_x_error_handler ();

	gnome_object_construct (GNOME_OBJECT (control), corba_control);

	control->priv->widget = GTK_WIDGET (widget);
	gtk_object_ref (GTK_OBJECT (widget));

	return control;
}

/**
 * gnome_control_new:
 * @widget: a GTK widget that contains the control and will be passed to the
 * container process.
 *
 * This function creates a new GnomeControl object for @widget
 *
 * Returns: a GnomeControl object that implements the GNOME::Control CORBA
 * service that will transfer the @widget to the container process.
 */
GnomeControl *
gnome_control_new (GtkWidget *widget)
{
	GnomeControl *control;
	GNOME_Control corba_control;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	control = gtk_type_new (gnome_control_get_type ());

	corba_control = gnome_control_corba_object_create (GNOME_OBJECT (control));
	if (corba_control == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (control));
		return NULL;
	}
	
	return gnome_control_construct (control, corba_control, widget);
}

static void
gnome_control_destroy (GtkObject *object)
{
	GnomeControl *control = GNOME_CONTROL (object);
	
	/*
	 * Destroy the control's top-level widget.
	 */
	if (control->priv->widget)
		gtk_object_unref (GTK_OBJECT (control->priv->widget));

	/*
	 * If the plug still exists, destroy it.  The plug might not
	 * exist in the case where the container application died,
	 * taking the plug out with it.  In that case,
	 * plug_destroy_cb() would have been invoked, and it would
	 * have triggered the destruction of the Control.  Which is why
	 * we're here now.
	 */
	if (control->priv->plug) {
		gtk_signal_disconnect (GTK_OBJECT (control->priv->plug), control->priv->plug_destroy_id);
		gtk_object_destroy (GTK_OBJECT (control->priv->plug));
		control->priv->plug = NULL;
	}

	g_free (control->priv);
}

static void
init_control_corba_class (void)
{
	/* The entry point vectors for this GNOME::Control class */
	gnome_control_epv.size_allocate = impl_GNOME_Control_size_allocate;
	gnome_control_epv.set_window = impl_GNOME_Control_set_window;
	gnome_control_epv.size_query = impl_GNOME_Control_size_query;
	
	/* Setup the vector of epvs */
	gnome_control_vepv.GNOME_Unknown_epv = &gnome_object_epv;
	gnome_control_vepv.GNOME_Control_epv = &gnome_control_epv;
}

/**
 * gnome_control_request_resize:
 * @control: A GnomeControl object which we are requesting to resize.
 * @width: The requested width.
 * @height: The requested height.
 *
 * Asks the ControlFrame associated with @control to resize the control.  If
 * the ControlFrame acquiesces, the toplevel widget associated with @control
 * will get a "size_allocate" signal.
 */
void
gnome_control_request_resize (GnomeControl *control, int width, int height)
{
	CORBA_Environment ev;

	g_return_if_fail (control != NULL);
	g_return_if_fail (GNOME_IS_CONTROL (control));
	g_return_if_fail (control->control_frame != CORBA_OBJECT_NIL); 

	CORBA_exception_init (&ev);

	GNOME_ControlFrame_request_resize (control->control_frame,
					   width, height,
					   &ev);

	CORBA_exception_free (&ev);
}

/**
 * gnome_control_set_control_frame:
 * @control: A GnomeControl object.
 * @control_frame: A CORBA interface for the ControlFrame which contains this Controo.
 *
 * Sets the ControlFrame for @control to @control_frame.
 */
void
gnome_control_set_control_frame (GnomeControl *control, GNOME_ControlFrame control_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (control != NULL);
	g_return_if_fail (GNOME_IS_CONTROL (control));

	CORBA_exception_init (&ev);

	GNOME_Unknown_ref (control_frame, &ev);

	control->control_frame = CORBA_Object_duplicate (control_frame, &ev);
	
	CORBA_exception_free (&ev);
}

/**
 * gnome_control_get_control_frame:
 * @control: A GnomeControl object whose GNOME_ControlFrame CORBA interface is
 * being retrieved.
 *
 * Returns: The GNOME_ControlFrame CORBA object associated with @control, this is
 * a CORBA_object_duplicated object.  You need to CORBA_free it when you are
 * done with it.
 */
GNOME_ControlFrame
gnome_control_get_control_frame (GnomeControl *control)
{
	GNOME_ControlFrame control_frame;
	CORBA_Environment ev;
	
	g_return_val_if_fail (control != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_CONTROL (control), CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	control_frame = CORBA_Object_duplicate (control->control_frame, &ev);
	CORBA_exception_free (&ev);

	return control_frame;
}

static void
gnome_control_class_init (GnomeControlClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_control_parent_class = gtk_type_class (gnome_object_get_type ());

	control_signals [SIZE_QUERY] =
                gtk_signal_new ("size_query",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (GnomeControlClass, size_query), 
                                gtk_marshal_NONE__POINTER_POINTER,
                                GTK_TYPE_NONE, 2,
				GTK_TYPE_POINTER, GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, control_signals, LAST_SIGNAL);

	object_class->destroy = gnome_control_destroy;
	init_control_corba_class ();
}

static void
gnome_control_init (GnomeControl *control)
{
	control->priv = g_new0 (GnomeControlPrivate, 1);
}

/**
 * gnome_control_get_type:
 *
 * Returns: The GtkType corresponding to the GnomeControl class.
 */
GtkType
gnome_control_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/Control:1.0",
			sizeof (GnomeControl),
			sizeof (GnomeControlClass),
			(GtkClassInitFunc) gnome_control_class_init,
			(GtkObjectInitFunc) gnome_control_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}
