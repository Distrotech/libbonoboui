/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME control frame object.
 *
 * Authors:
 *   Nat Friedman    (nat@helixcode.com)
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <gtk/gtkframe.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-control-frame.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdktypes.h>
#include <gtk/gtksocket.h>

enum {
	ACTIVATED,
	UNDO_LAST_OPERATION,
	ACTIVATE_URI,
	LAST_SIGNAL
};

static guint control_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static BonoboObjectClass *bonobo_control_frame_parent_class;

/* The entry point vectors for the server we provide */
POA_Bonobo_ControlFrame__vepv bonobo_control_frame_vepv;

struct _BonoboControlFramePrivate {
	Bonobo_Control	  control;
	GtkWidget        *container;
	GtkWidget	 *socket;
	BonoboUIHandler   *uih;
	BonoboPropertyBag *propbag;
};

static void
impl_Bonobo_ControlFrame_activated (PortableServer_Servant servant,
				   const CORBA_boolean state,
				   CORBA_Environment *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATED], state);
}

static void
impl_Bonobo_ControlFrame_deactivate_and_undo (PortableServer_Servant servant,
					     CORBA_Environment *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATED], FALSE);

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [UNDO_LAST_OPERATION]);
}


static Bonobo_UIHandler
impl_Bonobo_ControlFrame_get_ui_handler (PortableServer_Servant servant,
					CORBA_Environment *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	if (control_frame->priv->uih == NULL)
		return CORBA_OBJECT_NIL;
	
	return CORBA_Object_duplicate (
		bonobo_object_corba_objref (BONOBO_OBJECT (control_frame->priv->uih)), ev);
}

static Bonobo_PropertyBag
impl_Bonobo_ControlFrame_get_ambient_properties (PortableServer_Servant servant,
						CORBA_Environment *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));
	Bonobo_PropertyBag corba_propbag;

	if (control_frame->priv->propbag == NULL)
		return CORBA_OBJECT_NIL;

	corba_propbag = (Bonobo_PropertyBag)
		bonobo_object_corba_objref (BONOBO_OBJECT (control_frame->priv->propbag));

	corba_propbag = CORBA_Object_duplicate (corba_propbag, ev);

	return corba_propbag;
}

static void
impl_Bonobo_ControlFrame_queue_resize (PortableServer_Servant servant,
				      CORBA_Environment *ev)
{
	/*
	 * Nothing.
	 *
	 * In the Gnome implementation of Bonobo, all size negotiation
	 * is handled by GtkPlug/GtkSocket for us.
	 */
}

static void
impl_Bonobo_ControlFrame_activate_uri (PortableServer_Servant servant,
				      const CORBA_char *uri,
				      CORBA_boolean relative,
				      CORBA_Environment *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATE_URI],
			 (const char *) uri, (gboolean) relative);
}

static CORBA_Object
create_bonobo_control_frame (BonoboObject *object)
{
	POA_Bonobo_ControlFrame *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_ControlFrame *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_control_frame_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_ControlFrame__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return bonobo_object_activate_servant (object, servant);
}

static void
bonobo_control_frame_set_remote_window (GtkWidget *socket, BonoboControlFrame *control_frame)
{
	Bonobo_Control control = bonobo_control_frame_get_control (control_frame);
	CORBA_Environment ev;
	Bonobo_Control_windowid id;

	/*
	 * If we are not yet bound to a remote control, don't do
	 * anything.
	 */
	if (control == CORBA_OBJECT_NIL)
		return;

	/*
	 * Otherwise, pass the window ID of our GtkSocket to the
	 * remote Control.
	 */
	CORBA_exception_init (&ev);
	id = bonobo_control_windowid_from_x11 (GDK_WINDOW_XWINDOW (socket->window));
	Bonobo_Control_set_window (control, id, &ev);
	g_free (id);
	if (ev._major != CORBA_NO_EXCEPTION)
		bonobo_object_check_env (BONOBO_OBJECT (control_frame), control, &ev);
	CORBA_exception_free (&ev);
}

/**
 * bonobo_control_frame_construct:
 * @control_frame: The BonoboControlFrame object to be initialized.
 * @corba_control_frame: A CORBA object for the Bonobo_ControlFrame interface.
 *
 * Initializes @control_frame with the parameters.
 *
 * Returns: the initialized BonoboControlFrame object @control_frame that implements the
 * Bonobo::ControlFrame CORBA service.
 */
BonoboControlFrame *
bonobo_control_frame_construct (BonoboControlFrame *control_frame,
			       Bonobo_ControlFrame corba_control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	bonobo_object_construct (BONOBO_OBJECT (control_frame), corba_control_frame);

	/*
	 * Now create the GtkSocket which will be used to embed
	 * the Control.
	 */
	control_frame->priv->socket = gtk_socket_new ();
	gtk_widget_show (control_frame->priv->socket);

	/*
	 * Finally, create a frame to hold the socket; this no-window
	 * container is needed solely for the sake of bypassing
	 * plug/socket in the local case.
	 */
	control_frame->priv->container = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (control_frame->priv->container), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (control_frame->priv->container), 0);
	gtk_container_add (GTK_CONTAINER (control_frame->priv->container),
			   control_frame->priv->socket);
	gtk_widget_show (control_frame->priv->container);

	/*
	 * When the socket is realized, we pass its Window ID to our
	 * Control.
	 */
	gtk_signal_connect (GTK_OBJECT (control_frame->priv->socket),
			    "realize",
			    GTK_SIGNAL_FUNC (bonobo_control_frame_set_remote_window),
			    control_frame);

	return control_frame;
}

/**
 * bonobo_control_frame_new:
 *
 * Returns: BonoboControlFrame object that implements the
 * Bonobo::ControlFrame CORBA service. 
 */
BonoboControlFrame *
bonobo_control_frame_new (void)
{
	Bonobo_ControlFrame corba_control_frame;
	BonoboControlFrame *control_frame;

	control_frame = gtk_type_new (BONOBO_CONTROL_FRAME_TYPE);

	corba_control_frame = create_bonobo_control_frame (BONOBO_OBJECT (control_frame));
	if (corba_control_frame == CORBA_OBJECT_NIL) {
		gtk_object_destroy (GTK_OBJECT (control_frame));
		return NULL;
	}

	return bonobo_control_frame_construct (control_frame, corba_control_frame);
}

static void
bonobo_control_frame_destroy (GtkObject *object)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (object);

	if (control_frame->priv->control != CORBA_OBJECT_NIL){
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		Bonobo_Control_unref (control_frame->priv->control, &ev);
		CORBA_Object_release (control_frame->priv->control, &ev);
		CORBA_exception_free (&ev);
	}
	
	g_free (control_frame->priv);
	
	GTK_OBJECT_CLASS (bonobo_control_frame_parent_class)->destroy (object);
}

/**
 * bonobo_control_frame_get_epv:
 */
POA_Bonobo_ControlFrame__epv *
bonobo_control_frame_get_epv (void)
{
	POA_Bonobo_ControlFrame__epv *epv;

	epv = g_new0 (POA_Bonobo_ControlFrame__epv, 1);

	epv->activated              = impl_Bonobo_ControlFrame_activated;
	epv->deactivate_and_undo    = impl_Bonobo_ControlFrame_deactivate_and_undo;
	epv->get_ui_handler         = impl_Bonobo_ControlFrame_get_ui_handler;
	epv->queue_resize           = impl_Bonobo_ControlFrame_queue_resize;
	epv->activate_uri           = impl_Bonobo_ControlFrame_activate_uri;
	epv->get_ambient_properties = impl_Bonobo_ControlFrame_get_ambient_properties;

	return epv;
}

static void
init_control_frame_corba_class (void)
{
	/* Setup the vector of epvs */
	bonobo_control_frame_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_control_frame_vepv.Bonobo_ControlFrame_epv = bonobo_control_frame_get_epv ();
}

typedef void (*GnomeSignal_NONE__STRING_BOOL) (GtkObject *, const char *, gboolean, gpointer);

static void
gnome_marshal_NONE__STRING_BOOL (GtkObject     *object,
				 GtkSignalFunc  func,
				 gpointer       func_data,
				 GtkArg        *args)
{
	GnomeSignal_NONE__STRING_BOOL rfunc;

	rfunc = (GnomeSignal_NONE__STRING_BOOL) func;
	(*rfunc)(object, GTK_VALUE_STRING (args [0]), GTK_VALUE_BOOL (args [1]), func_data);
}

static void
bonobo_control_frame_class_init (BonoboControlFrameClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;

	bonobo_control_frame_parent_class = gtk_type_class (BONOBO_OBJECT_TYPE);

	control_frame_signals [ACTIVATED] =
		gtk_signal_new ("activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboControlFrameClass, activated),
				gtk_marshal_NONE__BOOL,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	
	control_frame_signals [UNDO_LAST_OPERATION] =
		gtk_signal_new ("undo_last_operation",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboControlFrameClass, undo_last_operation),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	control_frame_signals [ACTIVATE_URI] =
		gtk_signal_new ("activate_uri",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboControlFrameClass, activate_uri),
				gnome_marshal_NONE__STRING_BOOL,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_STRING, GTK_TYPE_BOOL);
	
	gtk_object_class_add_signals (
		object_class,
		control_frame_signals,
		LAST_SIGNAL);

	object_class->destroy = bonobo_control_frame_destroy;

	init_control_frame_corba_class ();
}

static void
bonobo_control_frame_init (BonoboObject *object)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (object);

	control_frame->priv = g_new0 (BonoboControlFramePrivate, 1);
}

/**
 * bonobo_control_frame_get_type:
 *
 * Returns: The GtkType for the BonoboControlFrame class.  */
GtkType
bonobo_control_frame_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboControlFrame",
			sizeof (BonoboControlFrame),
			sizeof (BonoboControlFrameClass),
			(GtkClassInitFunc) bonobo_control_frame_class_init,
			(GtkObjectInitFunc) bonobo_control_frame_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_control_frame_control_activate:
 * @control_frame: The BonoboControlFrame object whose control should be
 * activated.
 *
 * Activates the BonoboControl embedded in @control_frame by calling the
 * activate() #Bonobo_Control interface method on it.
 */
void
bonobo_control_frame_control_activate (BonoboControlFrame *control_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Check that this ControLFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	Bonobo_Control_activate (control_frame->priv->control, TRUE, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (control_frame),
			(CORBA_Object) control_frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}


/**
 * bonobo_control_frame_control_deactivate:
 * @control_frame: The BonoboControlFrame object whose control should be
 * deactivated.
 *
 * Deactivates the BonoboControl embedded in @control_frame by calling
 * the activate() CORBA method on it with the parameter %FALSE.
 */
void
bonobo_control_frame_control_deactivate (BonoboControlFrame *control_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Check that this ControlFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	Bonobo_Control_activate (control_frame->priv->control, FALSE, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (control_frame),
			(CORBA_Object) control_frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * bonobo_control_frame_bind_to_control:
 * @control_frame: A BonoboControlFrame object.
 * @control: The CORBA object for the BonoboControl embedded
 * in this BonoboControlFrame.
 *
 * Associates @control with this @control_frame.
 */
void
bonobo_control_frame_bind_to_control (BonoboControlFrame *control_frame, Bonobo_Control control)
{
	CORBA_Environment ev;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Keep a local handle to the Control.
	 */
	CORBA_exception_init (&ev);
	Bonobo_Control_ref (control, &ev);
	control_frame->priv->control = CORBA_Object_duplicate (control, &ev);
	CORBA_exception_free (&ev);

	/*
	 * Introduce ourselves to the Control.
	 */
	CORBA_exception_init (&ev);
	Bonobo_Control_set_frame (control,
				 bonobo_object_corba_objref (BONOBO_OBJECT (control_frame)),
				 &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		bonobo_object_check_env (BONOBO_OBJECT (control_frame), control, &ev);
	CORBA_exception_free (&ev);

	/*
	 * If the socket is realized, then we transfer the
	 * window ID to the remote control.
	 */
	if (GTK_WIDGET_REALIZED (control_frame->priv->socket)) {
		bonobo_control_frame_set_remote_window (control_frame->priv->socket,
						       control_frame);
	}
}

/**
 * bonobo_control_frame_get_control:
 * @control_frame: A BonoboControlFrame which is bound to a remote
 * BonoboControl.
 *
 * Returns: The Bonobo_Control CORBA interface for the remote Control
 * which is bound to @frame.  See also
 * bonobo_control_frame_bind_to_control().
 */
Bonobo_Control
bonobo_control_frame_get_control (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), CORBA_OBJECT_NIL);

	return control_frame->priv->control;
}

/**
 * bonobo_control_frame_get_widget:
 * @frame: The BonoboControlFrame whose widget is being requested.a
 *
 * Use this function when you want to embed a BonoboControl into your
 * container's widget hierarchy.  Once you have bound the
 * BonoboControlFrame to a remote BonoboControl, place the widget
 * returned by bonobo_control_frame_get_widget() into your widget
 * hierarchy and the control will appear in your application.
 *
 * Returns: A GtkWidget which has the remote BonoboControl physically
 * inside it.
 */
GtkWidget *
bonobo_control_frame_get_widget (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	return control_frame->priv->container;
}

/**
 * bonobo_control_frame_set_propbag:
 * @control_frame: A BonoboControlFrame object.
 * @propbag: A BonoboPropertyBag which will hold @control_frame's
 * ambient properties.
 *
 * Makes @control_frame use @propbag for its ambient properties.  When
 * @control_frame's Control requests the ambient properties, it will
 * get them from @propbag.
 */

void
bonobo_control_frame_set_propbag (BonoboControlFrame  *control_frame,
				 BonoboPropertyBag   *propbag)
{
	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (propbag != NULL);
	g_return_if_fail (BONOBO_IS_PROPERTY_BAG (propbag));

	control_frame->priv->propbag = propbag;
}

/**
 * bonobo_control_frame_get_propbag:
 * @control_frame: A BonoboControlFrame object whose PropertyBag has
 * been set.
 *
 * Returns: The BonoboPropertyBag object which has been associated with
 * @control_frame.
 */
BonoboPropertyBag *
bonobo_control_frame_get_propbag (BonoboControlFrame  *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	return control_frame->priv->propbag;
}

/**
 * bonobo_control_frame_set_ui_handler:
 * @control_frame: A BonoboControlFrame object.
 * @uih: A BonoboUIHandler object to be associated with this ControlFrame.
 *
 * Sets the BonoboUIHandler object for this ControlFrame.  When the
 * ControlFrame's Control requests its container's UIHandler
 * interface, the ControlFrame will pass it the UIHandler specified
 * here.  See also bonobo_control_frame_get_ui_handler().
 */
void
bonobo_control_frame_set_ui_handler (BonoboControlFrame *control_frame, BonoboUIHandler *uih)
{
	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	control_frame->priv->uih = uih;
}

/**
 * bonobo_control_frame_get_ui_handler:
 * @control_frame: A BonoboControlFrame object.
 *
 * Returns: The BonoboUIHandler associated with this COntrolFrame.  See
 * also bonobo_control_frame_set_ui_handler().
 */
BonoboUIHandler *
bonobo_control_frame_get_ui_handler (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	return control_frame->priv->uih;
}

/**
 * bonobo_control_frame_get_control_property_bag:
 */
BonoboPropertyBagClient *
bonobo_control_frame_get_control_property_bag (BonoboControlFrame *control_frame)
{
	Bonobo_PropertyBag pbag;
	CORBA_Environment ev;
	Bonobo_Control control;

	control = control_frame->priv->control;

	CORBA_exception_init (&ev);

	pbag = Bonobo_Control_get_property_bag (control, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (BONOBO_OBJECT (control_frame), control, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	return bonobo_property_bag_client_new (pbag);
}
