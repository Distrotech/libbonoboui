/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Bonobo control frame object.
 *
 * Authors:
 *   Nat Friedman      (nat@helixcode.com)
 *   Miguel de Icaza   (miguel@kernel.org)
 *   Maciej Stachowiak (mjs@eazel.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 *                 2000 Eazel, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <gtk/gtkbox.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-control-frame.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdktypes.h>
#include <gtk/gtkhbox.h>
#include <bonobo/bonobo-socket.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-marshal.h>

enum {
	ACTIVATED,
	ACTIVATE_URI,
	LAST_SIGNAL
};

#define PARENT_TYPE BONOBO_OBJECT_TYPE

static guint control_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static GObjectClass *bonobo_control_frame_parent_class;

struct _BonoboControlFramePrivate {
	Bonobo_Control	   control;
	GtkWidget         *container;
	GtkWidget	  *socket;
	Bonobo_UIContainer ui_container;
	BonoboPropertyBag *propbag;
	gboolean           autoactivate;
	gboolean           autostate;
	gboolean           activated;
};

static void
impl_Bonobo_ControlFrame_activated (PortableServer_Servant  servant,
				    const CORBA_boolean     state,
				    CORBA_Environment      *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATED], state);
}

static Bonobo_UIContainer
impl_Bonobo_ControlFrame_getUIHandler (PortableServer_Servant  servant,
					 CORBA_Environment      *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	if (control_frame->priv->ui_container == NULL)
		return CORBA_OBJECT_NIL;

	return bonobo_object_dup_ref (control_frame->priv->ui_container, ev);
}

static void
get_toplevel_fn (BonoboPropertyBag *bag,
		 BonoboArg         *arg,
		 guint              arg_id,
		 CORBA_Environment *ev,
		 gpointer           user_data)
{
	BonoboControlFrame *frame = BONOBO_CONTROL_FRAME (user_data);
	GtkWidget          *toplev;
	char               *id;

	g_return_if_fail (frame != NULL);

	for (toplev = bonobo_control_frame_get_widget (frame);
	     toplev && toplev->parent;
	     toplev = toplev->parent)
		;

	g_return_if_fail (toplev != NULL);

	/* FIXME: check if toplev is a socket - if so,
	   find it's ControlFrame and ask on up the chain ? */

	id = bonobo_control_window_id_from_x11 (
		GDK_WINDOW_XWINDOW (toplev->window));

/*	g_warning ("return toplevel '%s'", id); */

	*(char **)arg->_value = CORBA_string_dup (id);

	g_free (id);
}

static void
bonobo_control_frame_destroy (BonoboObject *object)
{
	BonoboControlFrame *control_frame = (BonoboControlFrame *) object;

	if (control_frame->priv && control_frame->priv->propbag)
		bonobo_property_bag_remove (
			control_frame->priv->propbag,
			BONOBO_CONTROL_FRAME_TOPLEVEL_PROP);
}

static void
add_toplevel_prop (BonoboControlFrame *control_frame)
{
	bonobo_property_bag_add_full (
		control_frame->priv->propbag,
		BONOBO_CONTROL_FRAME_TOPLEVEL_PROP, 0,
		TC_Bonobo_Control_windowId,
		NULL, "toplevel window id", "the toplevel window X id",
		BONOBO_PROPERTY_READABLE,
		g_cclosure_new (G_CALLBACK (get_toplevel_fn),
				control_frame, NULL),
		NULL);
}

static void
check_ambient_propbag (BonoboControlFrame *control_frame)
{
	BonoboPropertyBag *pb;

	if (control_frame->priv->propbag)
		return;

	if (!(pb = bonobo_control_frame_get_propbag (control_frame))) {
		pb = bonobo_property_bag_new (NULL, NULL, NULL);
		bonobo_control_frame_set_propbag (control_frame, pb);
	}

	control_frame->priv->propbag = pb;

	add_toplevel_prop (control_frame);
}

static Bonobo_PropertyBag
impl_Bonobo_ControlFrame_getAmbientProperties (PortableServer_Servant servant,
					       CORBA_Environment     *ev)
{
	BonoboControlFrame *control_frame =
		BONOBO_CONTROL_FRAME (bonobo_object (servant));
	Bonobo_PropertyBag  corba_propbag;

	check_ambient_propbag (control_frame);

	corba_propbag = BONOBO_OBJREF (control_frame->priv->propbag);

	return bonobo_object_dup_ref (corba_propbag, ev);
}

static void
impl_Bonobo_ControlFrame_queueResize (PortableServer_Servant  servant,
				       CORBA_Environment      *ev)
{
	/*
	 * Nothing.
	 *
	 * In the Gnome implementation of Bonobo, all size negotiation
	 * is handled by GtkPlug/GtkSocket for us.
	 */
}

static void
impl_Bonobo_ControlFrame_activateURI (PortableServer_Servant  servant,
				       const CORBA_char       *uri,
				       CORBA_boolean           relative,
				       CORBA_Environment      *ev)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATE_URI],
			 (const char *) uri, (gboolean) relative);
}

static gint
bonobo_control_frame_autoactivate_focus_in (GtkWidget     *widget,
					    GdkEventFocus *focus,
					    gpointer       user_data)
{
	BonoboControlFrame *control_frame = user_data;
	
	if (! control_frame->priv->autoactivate)
		return FALSE;

	bonobo_control_frame_control_activate (control_frame);

	return FALSE;
}

static gint
bonobo_control_frame_autoactivate_focus_out (GtkWidget     *widget,
					     GdkEventFocus *focus,
					     gpointer       user_data)
{
	BonoboControlFrame *control_frame = user_data;

	if (! control_frame->priv->autoactivate)
		return FALSE;
	
	bonobo_control_frame_control_deactivate (control_frame);

	return FALSE;
}


static void
bonobo_control_frame_socket_state_changed (GtkWidget    *container,
					   GtkStateType  previous_state,
					   gpointer      user_data)
{
	BonoboControlFrame *control_frame = user_data;

	if (! control_frame->priv->autostate)
		return;

	bonobo_control_frame_control_set_state (
		control_frame,
		GTK_WIDGET_STATE (control_frame->priv->container));
}


static void
bonobo_control_frame_socket_destroy (GtkWidget          *socket,
				     BonoboControlFrame *control_frame)
{
	gtk_widget_unref (control_frame->priv->socket);
	control_frame->priv->socket = NULL;
}


static void
bonobo_control_frame_set_remote_window (GtkWidget          *socket,
					BonoboControlFrame *control_frame)
{
	Bonobo_Control control = bonobo_control_frame_get_control (control_frame);
	CORBA_Environment ev;
	Bonobo_Control_windowId id;

	/*
	 * If we are not yet bound to a remote control, don't do
	 * anything.
	 */
	if (control == CORBA_OBJECT_NIL || !socket)
		return;

	/*
	 * Sync the server, since the XID may have been created on the
	 * client side without communication with the X server.
	 */
	/* FIXME: not very convinced about this flush */
	gdk_flush ();

	/*
	 * Otherwise, pass the window ID of our GtkSocket to the
	 * remote Control.
	 */
	CORBA_exception_init (&ev);
	id = bonobo_control_window_id_from_x11 (
		GDK_WINDOW_XWINDOW (socket->window));

	Bonobo_Control_setWindowId (control, id, &ev);
	CORBA_free (id);
	if (BONOBO_EX (&ev))
		bonobo_object_check_env (BONOBO_OBJECT (control_frame), control, &ev);
	CORBA_exception_free (&ev);
}

static void
bonobo_control_frame_create_socket (BonoboControlFrame *control_frame)
{
	/*
	 * Now create the GtkSocket which will be used to embed
	 * the Control.
	 */
	control_frame->priv->socket = bonobo_socket_new ();
	gtk_widget_show (control_frame->priv->socket);

	bonobo_socket_set_control_frame (
		BONOBO_SOCKET (control_frame->priv->socket),
		control_frame);

	/*
	 * Connect to the focus events on the socket so
	 * that we can provide the autoactivation feature.
	 */
	gtk_signal_connect (GTK_OBJECT (control_frame->priv->socket),
			    "focus_in_event",
			    GTK_SIGNAL_FUNC (bonobo_control_frame_autoactivate_focus_in),
			    control_frame);

	gtk_signal_connect (GTK_OBJECT (control_frame->priv->socket),
			    "focus_out_event",
			    GTK_SIGNAL_FUNC (bonobo_control_frame_autoactivate_focus_out),
			    control_frame);

	/*
	 * Setup a handler for socket destroy.
	 */
	gtk_signal_connect (GTK_OBJECT (control_frame->priv->socket),
			    "destroy",
			    GTK_SIGNAL_FUNC (bonobo_control_frame_socket_destroy),
			    control_frame);

	/*
	 * Ref the socket so we can handle the case of the control destroying it for bypass.
	 */
	gtk_object_ref (GTK_OBJECT (control_frame->priv->socket));


	/*
	 * Pack into the hack box
	 */
	gtk_box_pack_start (GTK_BOX (control_frame->priv->container),
			    control_frame->priv->socket,
			    TRUE, TRUE, 0);
}
				

/**
 * bonobo_control_frame_construct:
 * @control_frame: The #BonoboControlFrame object to be initialized.
 * @ui_container: A CORBA object for the UIContainer for the container application.
 *
 * Initializes @control_frame with the parameters.
 *
 * Returns: the initialized BonoboControlFrame object @control_frame that implements the
 * Bonobo::ControlFrame CORBA service.
 */
BonoboControlFrame *
bonobo_control_frame_construct (BonoboControlFrame *control_frame,
				Bonobo_UIContainer  ui_container)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	/*
	 * See ui-faq.txt if this dies on you.
	 */
	if (ui_container != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		g_assert (CORBA_Object_is_a (ui_container, "IDL:Bonobo/UIContainer:1.0", &ev));
		control_frame->priv->ui_container = bonobo_object_dup_ref (ui_container, &ev);
		CORBA_exception_free (&ev);
	} else
		control_frame->priv->ui_container = ui_container;

	/*
	 * Finally, create a box to hold the socket; this no-window
	 * container is needed solely for the sake of bypassing
	 * plug/socket in the local case.
	 */
	control_frame->priv->container = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (control_frame->priv->container), 0);
	gtk_widget_ref (control_frame->priv->container);
	gtk_object_sink (GTK_OBJECT (control_frame->priv->container));
	gtk_widget_show (control_frame->priv->container);

	/*
	 * Setup a handler to proxy state from the container.
	 */
	gtk_signal_connect (GTK_OBJECT (control_frame->priv->container),
			    "state_changed",
			    GTK_SIGNAL_FUNC (bonobo_control_frame_socket_state_changed),
			    control_frame);


	bonobo_control_frame_create_socket (control_frame);

	return control_frame;
}

/**
 * bonobo_control_frame_new:
 * @ui_container: The #Bonobo_UIContainer for the container application.
 *
 * Returns: BonoboControlFrame object that implements the
 * Bonobo::ControlFrame CORBA service. 
 */
BonoboControlFrame *
bonobo_control_frame_new (Bonobo_UIContainer ui_container)
{
	BonoboControlFrame *control_frame;

	control_frame = g_object_new (BONOBO_CONTROL_FRAME_TYPE, NULL);

	return bonobo_control_frame_construct (control_frame, ui_container);
}

static void
bonobo_control_frame_finalize (GObject *object)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (object);

	gtk_widget_destroy (control_frame->priv->container);

	if (control_frame->priv->control != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		Bonobo_Control_setFrame (control_frame->priv->control,
					 CORBA_OBJECT_NIL, &ev);
		CORBA_exception_free (&ev);
		bonobo_object_release_unref (control_frame->priv->control, NULL);
	}
	control_frame->priv->control = CORBA_OBJECT_NIL;

	if (control_frame->priv->socket) {
		bonobo_socket_set_control_frame (
			BONOBO_SOCKET (control_frame->priv->socket), NULL);
		gtk_signal_disconnect_by_data (
			GTK_OBJECT (control_frame->priv->socket),
			control_frame);
		gtk_widget_unref (control_frame->priv->socket);
		control_frame->priv->socket = NULL;
	}

	if (control_frame->priv->ui_container != CORBA_OBJECT_NIL)
		bonobo_object_release_unref (control_frame->priv->ui_container, NULL);
	control_frame->priv->ui_container = CORBA_OBJECT_NIL;

	g_free (control_frame->priv);
	control_frame->priv = NULL;
	
	bonobo_control_frame_parent_class->finalize (object);
}

static void
bonobo_control_frame_activated (BonoboControlFrame *control_frame, gboolean state)
{
	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	control_frame->priv->activated = state;
}

static void
bonobo_control_frame_class_init (BonoboControlFrameClass *klass)
{
	GObjectClass      *object_class = (GObjectClass *) klass;
	BonoboObjectClass *bobject_class = (BonoboObjectClass *) klass;
	POA_Bonobo_ControlFrame__epv *epv = &klass->epv;

	bonobo_control_frame_parent_class = g_type_class_peek_parent (klass);

	control_frame_signals [ACTIVATED] =
		g_signal_new ("activated",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BonoboControlFrameClass, activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);

	
	control_frame_signals [ACTIVATE_URI] =
		g_signal_new ("activate_uri",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BonoboControlFrameClass, activate_uri),
			      NULL, NULL,
			      bonobo_marshal_VOID__STRING_BOOLEAN,
			      G_TYPE_NONE, 2,
			      G_TYPE_STRING, G_TYPE_BOOLEAN);
	
	klass->activated = bonobo_control_frame_activated;

	object_class->finalize = bonobo_control_frame_finalize;
	bobject_class->destroy = bonobo_control_frame_destroy;

	epv->activated            = impl_Bonobo_ControlFrame_activated;
	epv->getUIHandler         = impl_Bonobo_ControlFrame_getUIHandler;
	epv->queueResize          = impl_Bonobo_ControlFrame_queueResize;
	epv->activateURI          = impl_Bonobo_ControlFrame_activateURI;
	epv->getAmbientProperties = impl_Bonobo_ControlFrame_getAmbientProperties;
}

static void
bonobo_control_frame_init (BonoboObject *object)
{
	BonoboControlFrame *control_frame = BONOBO_CONTROL_FRAME (object);

	control_frame->priv               = g_new0 (BonoboControlFramePrivate, 1);
	control_frame->priv->autoactivate = FALSE;
	control_frame->priv->autostate    = TRUE;
}

BONOBO_TYPE_FUNC_FULL (BonoboControlFrame, 
			   Bonobo_ControlFrame,
			   PARENT_TYPE,
			   bonobo_control_frame);

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

	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Check that this ControLFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	Bonobo_Control_activate (control_frame->priv->control, TRUE, &ev);

	if (BONOBO_EX (&ev)) {
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

	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Check that this ControlFrame actually has a Control associated
	 * with it.
	 */
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	Bonobo_Control_activate (control_frame->priv->control, FALSE, &ev);

	if (BONOBO_EX (&ev)) {
		bonobo_object_check_env (
			BONOBO_OBJECT (control_frame),
			(CORBA_Object) control_frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * bonobo_control_frame_set_autoactivate:
 * @control_frame: A BonoboControlFrame object.
 * @autoactivate: A flag which indicates whether or not the
 * ControlFrame should automatically perform activation on the Control
 * to which it is bound.
 *
 * Modifies the autoactivate behavior of @control_frame.  If
 * @control_frame is set to autoactivate, then it will automatically
 * send an "activate" message to the Control to which it is bound when
 * it gets a focus-in event, and a "deactivate" message when it gets a
 * focus-out event.  Autoactivation is off by default.
 */
void
bonobo_control_frame_set_autoactivate (BonoboControlFrame  *control_frame,
				       gboolean             autoactivate)
{
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	control_frame->priv->autoactivate = autoactivate;
}


/**
 * bonobo_control_frame_get_autoactivate:
 * @control_frame: A #BonoboControlFrame object.
 *
 * Returns: A boolean which indicates whether or not @control_frame is
 * set to automatically activate its Control.  See
 * bonobo_control_frame_set_autoactivate().
 */
gboolean
bonobo_control_frame_get_autoactivate (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), FALSE);

	return control_frame->priv->autoactivate;
}

static Bonobo_Control_State
bonobo_control_frame_state_to_corba (const GtkStateType state)
{
	switch (state) {
	case GTK_STATE_NORMAL:
		return Bonobo_Control_StateNormal;

	case GTK_STATE_ACTIVE:
		return Bonobo_Control_StateActive;

	case GTK_STATE_PRELIGHT:
		return Bonobo_Control_StatePrelight;

	case GTK_STATE_SELECTED:
		return Bonobo_Control_StateSelected;

	case GTK_STATE_INSENSITIVE:
		return Bonobo_Control_StateInsensitive;

	default:
		g_warning ("bonobo_control_frame_state_to_corba: Unknown state: %d", (gint) state);
		return Bonobo_Control_StateNormal;
	}
}

/**
 * bonobo_control_frame_control_set_state:
 * @control_frame: A #BonoboControlFrame object which is bound to a
 * remote #BonoboControl.
 * @state: A #GtkStateType value, specifying the widget state to apply
 * to the remote control.
 *
 * Proxies @state to the control bound to @control_frame.
 */
void
bonobo_control_frame_control_set_state (BonoboControlFrame  *control_frame,
					GtkStateType         state)
{
	Bonobo_Control_State  corba_state;
	CORBA_Environment     ev;

	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);

	corba_state = bonobo_control_frame_state_to_corba (state);

	CORBA_exception_init (&ev);

	Bonobo_Control_setState (control_frame->priv->control, corba_state, &ev);

	if (BONOBO_EX (&ev)) {
		bonobo_object_check_env (
			BONOBO_OBJECT (control_frame),
			control_frame->priv->control, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * bonobo_control_frame_set_autostate:
 * @control_frame: A #BonoboControlFrame object.
 * @autostate: Whether or not GtkWidget state changes should be
 * automatically propagated down to the Control.
 *
 * Changes whether or not @control_frame automatically proxies
 * state changes to its associated control.  The default mode
 * is for the control frame to autopropagate.
 */
void
bonobo_control_frame_set_autostate (BonoboControlFrame  *control_frame,
				    gboolean             autostate)
{
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	control_frame->priv->autostate = autostate;
}

/**
 * bonobo_control_frame_get_autostate:
 * @control_frame: A #BonoboControlFrame object.
 *
 * Returns: Whether or not this control frame will automatically
 * proxy GtkState changes to its associated Control.
 */
gboolean
bonobo_control_frame_get_autostate (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), FALSE);

	return control_frame->priv->autostate;
}


/**
 * bonobo_control_frame_get_ui_container:
 * @control_frame: A BonoboControlFrame object.

 * Returns: The Bonobo_UIContainer object reference associated with this
 * ControlFrame.  This ui_container is specified when the ControlFrame is
 * created.  See bonobo_control_frame_new().
 */
Bonobo_UIContainer
bonobo_control_frame_get_ui_container (BonoboControlFrame *control_frame)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), CORBA_OBJECT_NIL);

	return control_frame->priv->ui_container;
}

/**
 * bonobo_control_frame_set_ui_container:
 * @control_frame: A BonoboControlFrame object.
 * @uic: A Bonobo_UIContainer object reference.
 *
 * Associates a new %Bonobo_UIContainer object with this ControlFrame. This
 * is only allowed while the Control is deactivated.
 */
void
bonobo_control_frame_set_ui_container (BonoboControlFrame *control_frame, Bonobo_UIContainer ui_container)
{
	Bonobo_UIContainer old_ui_container;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (control_frame->priv->activated == FALSE);

	old_ui_container = control_frame->priv->ui_container;

	if (ui_container != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		g_assert (CORBA_Object_is_a (ui_container, "IDL:Bonobo/UIContainer:1.0", &ev));
		control_frame->priv->ui_container = bonobo_object_dup_ref (ui_container, &ev);
		CORBA_exception_free (&ev);
	} else
		control_frame->priv->ui_container = CORBA_OBJECT_NIL;

	if (old_ui_container)
		bonobo_object_release_unref (old_ui_container, NULL);
}


/**
 * bonobo_control_frame_bind_to_control:
 * @control_frame: A BonoboControlFrame object.
 * @control: The CORBA object for the BonoboControl embedded
 * in this BonoboControlFrame.
 * @opt_ev: Optional exception environment
 *
 * Associates @control with this @control_frame.
 */
void
bonobo_control_frame_bind_to_control (BonoboControlFrame *control_frame,
				      Bonobo_Control      control,
				      CORBA_Environment  *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;

	g_return_if_fail (control != CORBA_OBJECT_NIL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));

	/*
	 * Keep a local handle to the Control.
	 */
	if (control_frame->priv->control != CORBA_OBJECT_NIL)
		g_warning ("FIXME: leaking control reference");

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;

	control_frame->priv->control = bonobo_object_dup_ref (control, ev);

	/*
	 * Introduce ourselves to the Control.
	 */
	Bonobo_Control_setFrame (control, BONOBO_OBJREF (control_frame), ev);

	if (BONOBO_EX (ev))
		bonobo_object_check_env (BONOBO_OBJECT (control_frame), control, ev);

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);

	/* 
	 * Re-create the socket if it got destroyed by the Control before.
	 */
	if (control_frame->priv->socket == NULL) 
		bonobo_control_frame_create_socket (control_frame);

	/*
	 * If the socket is realized, then we transfer the
	 * window ID to the remote control.
	 */
	if (GTK_WIDGET_REALIZED (control_frame->priv->socket))
		bonobo_control_frame_set_remote_window (
			control_frame->priv->socket, control_frame);
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
	BonoboPropertyBag *old_pb;

	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (BONOBO_IS_PROPERTY_BAG (propbag));

	old_pb = control_frame->priv->propbag;

	control_frame->priv->propbag = propbag;

	add_toplevel_prop (control_frame);

	if (old_pb)
		bonobo_object_unref (BONOBO_OBJECT (old_pb));
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
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	return control_frame->priv->propbag;
}

/**
 * bonobo_control_frame_get_control_property_bag:
 * @control_frame: the control frame
 * @ev: CORBA exception environment
 * 
 * This retrives a Bonobo_PropertyBag reference from its
 * associated Bonobo Control
 *
 * Return value: CORBA property bag reference or CORBA_OBJECT_NIL
 **/
Bonobo_PropertyBag
bonobo_control_frame_get_control_property_bag (BonoboControlFrame *control_frame,
					       CORBA_Environment  *ev)
{
	Bonobo_PropertyBag pbag;
	Bonobo_Control control;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame), NULL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	control = control_frame->priv->control;

	pbag = Bonobo_Control_getProperties (control, real_ev);

	if (BONOBO_EX (real_ev)) {
		if (!ev)
			CORBA_exception_free (&tmp_ev);
		pbag = CORBA_OBJECT_NIL;
	}

	return pbag;
}

void
bonobo_control_frame_size_request (BonoboControlFrame *control_frame,
				   int                *desired_width,
				   int                *desired_height,
				   CORBA_Environment  *opt_ev)
{
	CORBA_Environment *ev, tmp_ev;
	CORBA_short width, height;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (control_frame));
	g_return_if_fail (control_frame->priv->control != CORBA_OBJECT_NIL);
	g_return_if_fail (desired_width != NULL);
	g_return_if_fail (desired_height != NULL);

	if (!opt_ev) {
		CORBA_exception_init (&tmp_ev);
		ev = &tmp_ev;
	} else
		ev = opt_ev;
		
	Bonobo_Control_getDesiredSize (control_frame->priv->control,
				       &width, &height, ev);

	if (BONOBO_EX (ev)) {
		bonobo_object_check_env (
			BONOBO_OBJECT (control_frame),
			(CORBA_Object) control_frame->priv->control, ev);

		width = height = 0;
	}

	*desired_width = width;
	*desired_height = height;

	if (!opt_ev)
		CORBA_exception_free (&tmp_ev);
}

/*
 *  These methods are used by the bonobo-socket to
 * sync the X pipe with the CORBA connection.
 */
void
bonobo_control_frame_sync_realize (BonoboControlFrame *frame)
{
	Bonobo_Control control;
	CORBA_Environment ev;

	g_return_if_fail (BONOBO_IS_CONTROL_FRAME (frame));
	
	if (!frame->priv || frame->priv->control == CORBA_OBJECT_NIL)
		return;

	control = frame->priv->control;

	bonobo_control_frame_set_remote_window (
		frame->priv->socket, frame);

	/*
	 * We sync here so that we make sure that if the XID for
	 * our window is passed to another application, SubstructureRedirectMask
	 * will be set by the time the other app creates its window.
	 */
	gdk_flush ();

	if (control == CORBA_OBJECT_NIL)
		return;

	CORBA_exception_init (&ev);

	Bonobo_Control_realize (control, &ev);
	if (BONOBO_EX (&ev))
		g_warning ("Exception on realize '%s'",
			   bonobo_exception_get_text (&ev));

	CORBA_exception_free (&ev);

	gdk_flush ();
}

void
bonobo_control_frame_sync_unrealize (BonoboControlFrame *frame)
{
	Bonobo_Control control;
	CORBA_Environment ev;

	if (!frame->priv || frame->priv->control == CORBA_OBJECT_NIL)
		return;

	control = frame->priv->control;

	gdk_flush ();

	if (control == CORBA_OBJECT_NIL)
		return;

	CORBA_exception_init (&ev);

	Bonobo_Control_unrealize (control, &ev);
	if (BONOBO_EX (&ev))
		g_warning ("Exception on unrealize '%s'",
			   bonobo_exception_get_text (&ev));

	CORBA_exception_free (&ev);

	gdk_flush ();
}

/**
 * bonobo_control_frame_focus:
 * @frame: A control frame.
 * @direction: Direction in which to change focus.
 * 
 * Proxies a #GtkContainer::focus() request to the embedded control.  This is an
 * internal function and it should only really be ever used by the #BonoboSocket
 * implementation.
 * 
 * Return value: TRUE if the child kept the focus, FALSE if focus should be
 * passed on to the next widget.
 **/
gboolean
bonobo_control_frame_focus (BonoboControlFrame *frame, GtkDirectionType direction)
{
	BonoboControlFramePrivate *priv;
	CORBA_Environment ev;
	gboolean result;
	Bonobo_Control_FocusDirection corba_direction;

	g_return_val_if_fail (frame != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_CONTROL_FRAME (frame), FALSE);

	priv = frame->priv;

	if (priv->control == CORBA_OBJECT_NIL)
		return FALSE;

	switch (direction) {
	case GTK_DIR_TAB_FORWARD:
		corba_direction = Bonobo_Control_TAB_FORWARD;
		break;

	case GTK_DIR_TAB_BACKWARD:
		corba_direction = Bonobo_Control_TAB_BACKWARD;
		break;

	case GTK_DIR_UP:
		corba_direction = Bonobo_Control_UP;
		break;

	case GTK_DIR_DOWN:
		corba_direction = Bonobo_Control_DOWN;
		break;

	case GTK_DIR_LEFT:
		corba_direction = Bonobo_Control_LEFT;
		break;

	case GTK_DIR_RIGHT:
		corba_direction = Bonobo_Control_RIGHT;
		break;

	default:
		g_assert_not_reached ();
		return FALSE;
	}

	CORBA_exception_init (&ev);

	result = Bonobo_Control_focus (priv->control, corba_direction, &ev);
	if (BONOBO_EX (&ev)) {
		g_message ("bonobo_control_frame_focus(): Exception while issuing focus "
			   "request: `%s'", bonobo_exception_get_text (&ev));
		result = FALSE;
	}

	CORBA_exception_free (&ev);
	return result;
}
