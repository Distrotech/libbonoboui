/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME control object
 *
 * Author:
 *   Nat Friedman (nat@helixcode.com)
 *   Miguel de Icaza (miguel@helixcode.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#include <config.h>
#include <stdlib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-control.h>
#include <gdk/gdkprivate.h>

enum {
	ACTIVATE,
	UNDO_LAST_OPERATION,
	LAST_SIGNAL
};

static guint control_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static BonoboObjectClass *bonobo_control_parent_class;

/* The entry point vectors for the server we provide */
POA_Bonobo_Control__vepv bonobo_control_vepv;

struct _BonoboControlPrivate {
	GtkWidget          *plug;

	int                 plug_destroy_id;
	gboolean            is_local;

	GtkWidget          *widget;

	Bonobo_ControlFrame  control_frame;

	BonoboUIHandler     *uih;
	BonoboPropertyBag   *propbag;
};

/**
 * window_id_demangle:
 * @id: CORBA_char *
 * 
 * De-mangle a window id string,
 * fields are separated by ':' character,
 * currently only the first field is used.
 * 
 * Return value: the X11 window id.
 **/
inline static guint32
window_id_demangle (Bonobo_Control_windowid id)
{
	guint32 x11_id;
	char **elements;
	
/*	printf ("ID string '%s'\n", id);*/

	elements = g_strsplit (id, ":", -1);
	if (elements && elements [0])
		x11_id = strtol (elements [0], NULL, 10);
	else {
		g_warning ("Serious X id mangling error");
		x11_id = 0;
	}
	g_strfreev (elements);

/*	printf ("x11 : %d\n", x11_id);*/

	return x11_id;
}

/**
 * bonobo_control_windowid_from_x11:
 * @x11_id: the x11 window id.
 * 
 * This mangles the X11 name into the ':' delimited
 * string format "X-id: ..."
 * 
 * Return value: the string; free after use.
 **/
Bonobo_Control_windowid
bonobo_control_windowid_from_x11 (guint32 x11_id)
{
	CORBA_char *str;

	str = g_strdup_printf ("%d", x11_id);

/*	printf ("Mangled %d to '%s'\n", x11_id, str);*/
	return str;
}

/*
 * This callback is invoked when the plug is unexpectedly destroyed.
 * This may happen if, for example, the container application goes
 * away.  This callback is _not_ invoked if the BonoboControl is destroyed
 * normally, i.e. the user unrefs the BonoboControl away.
 */
static gint
bonobo_control_plug_destroy_cb (GtkWidget *plug, GdkEventAny *event, gpointer closure)
{
	BonoboControl *control = BONOBO_CONTROL (closure);

	if (control->priv->plug != plug)
		g_warning ("Destroying incorrect plug");

	/*
	 * Set the plug to NULL here so that we don't try to
	 * destroy it later.  It will get destroyed on its
	 * own.
	 */
	control->priv->plug = NULL;

	/*
	 * Destroy this plug's BonoboControl.
	 */
	bonobo_object_destroy (BONOBO_OBJECT (control));

	return FALSE;
}

static void
impl_Bonobo_Control_activate (PortableServer_Servant servant,
			     CORBA_boolean activated,
			     CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control), control_signals [ACTIVATE], (gboolean) activated);
}


static void
impl_Bonobo_Control_reactivate_and_undo (PortableServer_Servant servant,
					CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control), control_signals [ACTIVATE], TRUE);
	gtk_signal_emit (GTK_OBJECT (control), control_signals [UNDO_LAST_OPERATION]);
}
	
static void
impl_Bonobo_Control_set_frame (PortableServer_Servant servant,
			      Bonobo_ControlFrame frame,
			      CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	bonobo_control_set_control_frame (control, frame);
}



GtkWidget *bonobo_gtk_widget_from_x11_id(guint32 xid)
{
	GdkWindow *window;
	gpointer data;

	window = gdk_window_lookup (xid);
	
	if (!window) {
		return NULL;
	}

	gdk_window_get_user_data(window, &data);

	if (!data || !GTK_IS_WIDGET(data)) {
		return NULL;
	} else {
		return GTK_WIDGET(data);
	}
}

static void
impl_Bonobo_Control_set_window (PortableServer_Servant servant,
			       Bonobo_Control_windowid id,
			       CORBA_Environment *ev)
{
	guint32 x11_id;
	GtkWidget *local_socket;
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	x11_id = window_id_demangle (id);

	local_socket = bonobo_gtk_widget_from_x11_id(x11_id);

	if (local_socket) {
		GtkWidget *socket_parent;
		control->priv->is_local = TRUE;
		socket_parent = local_socket->parent;
		gtk_container_remove (GTK_CONTAINER (socket_parent), local_socket);
		gtk_container_add (GTK_CONTAINER (socket_parent), control->priv->widget);
		gtk_widget_show_all (control->priv->widget);
	} else {
		control->priv->plug = gtk_plug_new (x11_id);
		control->priv->plug_destroy_id = gtk_signal_connect (
		        GTK_OBJECT (control->priv->plug), "destroy_event",
		        GTK_SIGNAL_FUNC (bonobo_control_plug_destroy_cb), control);
		gtk_container_add (GTK_CONTAINER (control->priv->plug), control->priv->widget);
		gtk_widget_show_all (control->priv->plug);
	}
}

static void
impl_Bonobo_Control_size_allocate (PortableServer_Servant servant,
				  const CORBA_short width,
				  const CORBA_short height,
				  CORBA_Environment *ev)
{
	/*
	 * Nothing.
	 *
	 * In the Gnome implementation of Bonobo, all size negotiation
	 * is handled by GtkPlug/GtkSocket for us, or GtkFrame in the
	 * local case.
	 */
}

static void
impl_Bonobo_Control_size_request (PortableServer_Servant servant,
				 CORBA_short *desired_width,
				 CORBA_short *desired_height,
				 CORBA_Environment *ev)
{
	/*
	 * Nothing.
	 */
}

static Bonobo_PropertyBag
impl_Bonobo_Control_get_property_bag (PortableServer_Servant servant,
				     CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));
	Bonobo_PropertyBag corba_propbag;

	if (control->priv->propbag == NULL)
		return CORBA_OBJECT_NIL;

	corba_propbag = (Bonobo_PropertyBag)
		bonobo_object_corba_objref (BONOBO_OBJECT (control->priv->propbag));

	corba_propbag = CORBA_Object_duplicate (corba_propbag, ev);

	return corba_propbag;
}

/**
 * bonobo_control_corba_object_create:
 * @object: the GtkObject that will wrap the CORBA object
 *
 * Creates and activates the CORBA object that is wrapped by the
 * @object BonoboObject.
 *
 * Returns: An activated object reference to the created object
 * or %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_Control
bonobo_control_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_Control *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_Control *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_control_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_Control__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (Bonobo_Control) bonobo_object_activate_servant (object, servant);
}

BonoboControl *
bonobo_control_construct (BonoboControl *control, Bonobo_Control corba_control, GtkWidget *widget)
{
	g_return_val_if_fail (control != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);
	g_return_val_if_fail (corba_control != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	/*
	 * This sets up the X handler for Bonobo objects.  We basically will
	 * ignore X errors if our container dies (because X will kill the
	 * windows of the container and our container without telling us).
	 */
	bonobo_setup_x_error_handler ();

	bonobo_object_construct (BONOBO_OBJECT (control), corba_control);

	control->priv->widget = GTK_WIDGET (widget);
	gtk_object_ref (GTK_OBJECT (widget));

	return control;
}

/**
 * bonobo_control_new:
 * @widget: a GTK widget that contains the control and will be passed to the
 * container process.
 *
 * This function creates a new BonoboControl object for @widget.
 *
 * Returns: a BonoboControl object that implements the Bonobo::Control CORBA
 * service that will transfer the @widget to the container process.
 */
BonoboControl *
bonobo_control_new (GtkWidget *widget)
{
	BonoboControl *control;
	Bonobo_Control corba_control;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	control = gtk_type_new (bonobo_control_get_type ());

	corba_control = bonobo_control_corba_object_create (BONOBO_OBJECT (control));
	if (corba_control == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (control));
		return NULL;
	}
	
	return bonobo_control_construct (control, corba_control, widget);
}

static void
bonobo_control_destroy (GtkObject *object)
{
	BonoboControl *control = BONOBO_CONTROL (object);
	
	/*
	 * If we have a UIHandler, destroy it.
	 */
	if (control->priv->uih != NULL) {
		bonobo_ui_handler_unset_container (control->priv->uih);
		bonobo_object_destroy (BONOBO_OBJECT (control->priv->uih));
	}

	/*
	 * Destroy the control's top-level widget.
	 */
	if (control->priv->widget)
		gtk_object_destroy (GTK_OBJECT (control->priv->widget));

	/*
	 * If the plug still exists, destroy it.  The plug might not
	 * exist in the case where the container application died,
	 * taking the plug out with it, or the optimized local case
	 * where the plug/socket mechanism was bypassed.  In the
	 * formaer case, plug_destroy_cb() would have been invoked,
	 * and it would have triggered the destruction of the Control,
	 * which is why we're here now. In the latter case, it's not
	 * needed because there is no plug.  
	 */

	if (control->priv->plug) {
		gtk_signal_disconnect (GTK_OBJECT (control->priv->plug), control->priv->plug_destroy_id);
		gtk_object_destroy (GTK_OBJECT (control->priv->plug));
		control->priv->plug = NULL;
	}

	g_free (control->priv);

	if (GTK_OBJECT_CLASS (bonobo_control_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (bonobo_control_parent_class)->destroy) (object);
}

/**
 * bonobo_control_get_epv:
 *
 */
POA_Bonobo_Control__epv *
bonobo_control_get_epv (void)
{
	POA_Bonobo_Control__epv *epv;

	epv = g_new0 (POA_Bonobo_Control__epv, 1);

	epv->reactivate_and_undo = impl_Bonobo_Control_reactivate_and_undo;
	epv->activate            = impl_Bonobo_Control_activate;
	epv->size_allocate       = impl_Bonobo_Control_size_allocate;
	epv->set_window          = impl_Bonobo_Control_set_window;
	epv->set_frame           = impl_Bonobo_Control_set_frame;
	epv->size_request        = impl_Bonobo_Control_size_request;
	epv->get_property_bag    = impl_Bonobo_Control_get_property_bag;

	return epv;
}

static void
init_control_corba_class (void)
{
	/* Setup the vector of epvs */
	bonobo_control_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_control_vepv.Bonobo_Control_epv = bonobo_control_get_epv ();
}

/**
 * bonobo_control_set_control_frame:
 * @control: A BonoboControl object.
 * @control_frame: A CORBA interface for the ControlFrame which contains this Controo.
 *
 * Sets the ControlFrame for @control to @control_frame.
 */
void
bonobo_control_set_control_frame (BonoboControl *control, Bonobo_ControlFrame control_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (control != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));

	CORBA_exception_init (&ev);

	Bonobo_Unknown_ref (control_frame, &ev);

	control->priv->control_frame = CORBA_Object_duplicate (control_frame, &ev);
	
	CORBA_exception_free (&ev);
}

/**
 * bonobo_control_get_control_frame:
 * @control: A BonoboControl object whose Bonobo_ControlFrame CORBA interface is
 * being retrieved.
 *
 * Returns: The Bonobo_ControlFrame CORBA object associated with @control, this is
 * a CORBA_object_duplicated object.  You need to CORBA_free it when you are
 * done with it.
 */
Bonobo_ControlFrame
bonobo_control_get_control_frame (BonoboControl *control)
{
	Bonobo_ControlFrame control_frame;
	CORBA_Environment ev;
	
	g_return_val_if_fail (control != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	control_frame = CORBA_Object_duplicate (control->priv->control_frame, &ev);
	CORBA_exception_free (&ev);

	return control_frame;
}

/**
 * bonobo_view_set_ui_handler:
 * @view: A BonoboView object.
 * @uih: A BonoboUIHandler object.
 *
 * Sets the BonoboUIHandler for @view to @uih.  This provides a
 * convenient way for a component to store the BonoboUIHandler which it
 * will use to merge menus and toolbars.
 */
void
bonobo_control_set_ui_handler (BonoboControl *control, BonoboUIHandler *uih)
{
	g_return_if_fail (control != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));

	control->priv->uih = uih;
}

/**
 * bonobo_control_get_ui_handler:
 * @control: A BonoboControl object for which a BonoboUIHandler has been
 * created and set.
 *
 * Returns: The BonoboUIHandler which was associated with @control using
 * bonobo_control_set_ui_handler().
 */
BonoboUIHandler *
bonobo_control_get_ui_handler (BonoboControl *control)
{
	g_return_val_if_fail (control != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);


	return control->priv->uih;
}

/**
 * bonobo_control_set_property_bag:
 * @control: A #BonoboControl object.
 * @pb: A #BonoboPropertyBag.
 *
 * Binds @pb to @control.  When a remote object queries @control
 * for its property bag, @pb will be used in the responses.
 */
void
bonobo_control_set_property_bag (BonoboControl *control, BonoboPropertyBag *pb)
{
	g_return_if_fail (control != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));
	g_return_if_fail (pb != NULL);
	g_return_if_fail (BONOBO_IS_PROPERTY_BAG (pb));

	control->priv->propbag = pb;
}

/**
 * bonobo_control_get_property_bag:
 * @control: A #BonoboControl whose PropertyBag has already been set.
 *
 * Returns: The #BonoboPropertyBag bound to @control.
 */
BonoboPropertyBag *
bonobo_control_get_property_bag (BonoboControl *control)
{
	g_return_val_if_fail (control != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	return control->priv->propbag;
}

/**
 * bonobo_control_get_ambient_properties:
 * @control: A #BonoboControl which is bound to a remote
 * #BonoboControlFrame.
 *
 * Returns: A #BonoboPropertyBagClient bound to the bag of ambient
 * properties associated with this #Control's #ControlFrame.
 */
BonoboPropertyBagClient *
bonobo_control_get_ambient_properties (BonoboControl *control)
{
	Bonobo_ControlFrame control_frame;
	Bonobo_PropertyBag pbag;
	CORBA_Environment ev;

	g_return_val_if_fail (control != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	control_frame = control->priv->control_frame;

	if (control_frame == CORBA_OBJECT_NIL)
		return NULL;

	CORBA_exception_init (&ev);

	pbag = Bonobo_ControlFrame_get_ambient_properties (control_frame, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (BONOBO_OBJECT (control), control_frame, &ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	return bonobo_property_bag_client_new (pbag);
}

/**
 * bonobo_control_get_remote_ui_handler:

 * @control: A BonoboControl object which is associated with a remote
 * ControlFrame.
 *
 * Returns: The Bonobo_UIHandler CORBA server for the remote BonoboControlFrame.
 */
Bonobo_UIHandler
bonobo_control_get_remote_ui_handler (BonoboControl *control)
{
	CORBA_Environment ev;
	Bonobo_UIHandler uih;

	g_return_val_if_fail (control != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	uih = Bonobo_ControlFrame_get_ui_handler (control->priv->control_frame, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (control), control->priv->control_frame, &ev);

	CORBA_exception_free (&ev);

	return uih;
}

/**
 * bonobo_control_activate_notify:
 * @control: A #BonoboControl object which is bound
 * to a remote ControlFrame.
 * @activated: Whether or not @control has been activated.
 *
 * Notifies the remote ControlFrame which is associated with
 * @control that @control has been activated/deactivated.
 */
void
bonobo_control_activate_notify (BonoboControl *control,
			       gboolean      activated)
{
	CORBA_Environment ev;

	g_return_if_fail (control != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));
	g_return_if_fail (control->priv->control_frame != CORBA_OBJECT_NIL);
	
	CORBA_exception_init (&ev);

	Bonobo_ControlFrame_activated (control->priv->control_frame, activated, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (control), control->priv->control_frame, &ev);

	CORBA_exception_free (&ev);
}

static void
bonobo_control_class_init (BonoboControlClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;

	bonobo_control_parent_class = gtk_type_class (bonobo_object_get_type ());

	control_signals [ACTIVATE] =
                gtk_signal_new ("activate",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboControlClass, activate),
                                gtk_marshal_NONE__BOOL,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	control_signals [UNDO_LAST_OPERATION] =
                gtk_signal_new ("undo_last_operation",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboControlClass, undo_last_operation),
                                gtk_marshal_NONE__NONE,
                                GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, control_signals, LAST_SIGNAL);

	object_class->destroy = bonobo_control_destroy;
	init_control_corba_class ();
}

static void
bonobo_control_init (BonoboControl *control)
{
	control->priv = g_new0 (BonoboControlPrivate, 1);
}

/**
 * bonobo_control_get_type:
 *
 * Returns: The GtkType corresponding to the BonoboControl class.
 */
GtkType
bonobo_control_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/Control:1.0",
			sizeof (BonoboControl),
			sizeof (BonoboControlClass),
			(GtkClassInitFunc) bonobo_control_class_init,
			(GtkObjectInitFunc) bonobo_control_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

