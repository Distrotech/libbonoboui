/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-property-control.c: Bonobo PropertyControl implementation
 *
 * Author:
 *      Iain Holmes  <iain@helixcode.com>
 *
 * Copyright 2000 Helix Code, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtktypeutils.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-property-control.h>
#include "Bonobo.h"

struct _BonoboPropertyControlPrivate {
	BonoboPropertyControlGetControlFn get_fn;

	void *closure;
};

enum {
	ACTION,
	LAST_SIGNAL
};

static BonoboObjectClass *parent_class;

static POA_Bonobo_PropertyControl__vepv bonobo_property_control_vepv;
static guint32 signals[LAST_SIGNAL] = { 0 };

static Bonobo_Control
impl_Bonobo_PropertyControl_getControl (PortableServer_Servant servant,
					CORBA_Environment *ev)
{
	BonoboObject *bonobo_object;
	BonoboPropertyControl *property_control;
	BonoboPropertyControlPrivate *priv;
	BonoboControl *control;

	bonobo_object = bonobo_object_from_servant (servant);
	property_control = BONOBO_PROPERTY_CONTROL (bonobo_object);
	priv = property_control->priv;

	control = (* priv->get_fn) (property_control, 
				    priv->closure);

	if (control == NULL)
		return CORBA_OBJECT_NIL;

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static void
impl_Bonobo_PropertyControl_notifyAction (PortableServer_Servant servant,
					  Bonobo_PropertyControl_Action action,
					  CORBA_Environment *ev)
{
	BonoboObject *bonobo_object;
	
	bonobo_object = bonobo_object_from_servant (servant);

	gtk_signal_emit (GTK_OBJECT (bonobo_object), signals [ACTION], action);
}

static void
bonobo_property_control_destroy (GtkObject *object)
{
	BonoboPropertyControl *property_control;
	
	property_control = BONOBO_PROPERTY_CONTROL (object);
	if (property_control->priv == NULL)
		return;

	g_free (property_control->priv);
	property_control->priv = NULL;

	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
corba_class_init (void)
{
	bonobo_property_control_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_property_control_vepv.Bonobo_PropertyControl_epv = bonobo_property_control_get_epv ();
}

static void
bonobo_property_control_class_init (BonoboPropertyControlClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);
	object_class->destroy = bonobo_property_control_destroy;

	signals [ACTION] = gtk_signal_new ("action",
					 GTK_RUN_FIRST, object_class->type,
					 GTK_SIGNAL_OFFSET (BonoboPropertyControlClass, action),
					 gtk_marshal_NONE__INT, GTK_TYPE_NONE,
					 1, GTK_TYPE_ENUM);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
				     
	parent_class = gtk_type_class (bonobo_object_get_type ());

	corba_class_init ();
}

static void
bonobo_property_control_init (BonoboPropertyControl *property_control)
{
	BonoboPropertyControlPrivate *priv;

	priv = g_new (BonoboPropertyControlPrivate, 1);
	priv->get_fn = NULL;
	priv->closure = NULL;

	property_control->priv = priv;
}

/**
 * bonobo_property_control_get_epv:
 *
 */
POA_Bonobo_PropertyControl__epv *
bonobo_property_control_get_epv (void)
{
	POA_Bonobo_PropertyControl__epv *epv;

	epv = g_new0 (POA_Bonobo_PropertyControl__epv, 1);
	epv->getControl = impl_Bonobo_PropertyControl_getControl;
	epv->notifyAction = impl_Bonobo_PropertyControl_notifyAction;

	return epv;
}

/**
 * bonobo_property_control_get_type:
 *
 * Returns: The GtkType for the BonoboPropertyControl class.
 */
GtkType
bonobo_property_control_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboPropertyControl",
			sizeof (BonoboPropertyControl),
			sizeof (BonoboPropertyControlClass),
			(GtkClassInitFunc) bonobo_property_control_class_init,
			(GtkObjectInitFunc) bonobo_property_control_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

Bonobo_PropertyControl
bonobo_property_control_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_PropertyControl *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_PropertyControl *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_property_control_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_PropertyControl__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	return bonobo_object_activate_servant (object, servant);
}

/**
 * bonobo_property_control_construct:
 * @property_control: A BonoboPropertyControl object.
 * @get_fn: Creation routine.
 * @closure: Data passed to closure routine.
 *
 * Initialises the BonoboPropertyControl object.
 *
 * Returns: The newly constructed BonoboPropertyControl.
 */
BonoboPropertyControl *
bonobo_property_control_construct (BonoboPropertyControl *property_control,
				   Bonobo_PropertyControl corba_control,
				   BonoboPropertyControlGetControlFn get_fn,
				   void *closure)
{
	BonoboPropertyControlPrivate *priv;

	g_return_val_if_fail (property_control != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_PROPERTY_CONTROL (property_control), NULL);
	g_return_val_if_fail (corba_control != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (property_control), 
				 corba_control);

	priv = property_control->priv;
	priv->get_fn = get_fn;
	priv->closure = closure;

	return property_control;
}

/**
 * bonobo_property_control_new:
 * @get_fn: The function to be called when the getControl method is called.
 * @closure: The data to be passed into the @get_fn routine
 *
 * Creates a BonoboPropertyControl object.
 *
 * Returns: A pointer to a newly created BonoboPropertyControl object.
 */
BonoboPropertyControl *
bonobo_property_control_new (BonoboPropertyControlGetControlFn get_fn,
			     void *closure)
{
	BonoboPropertyControl *property_control;
	Bonobo_PropertyControl corba_control;

	property_control = gtk_type_new (bonobo_property_control_get_type ());
	corba_control = bonobo_property_control_corba_object_create
		(BONOBO_OBJECT (property_control));
	if (corba_control == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (property_control));
		return NULL;
	}
								     
	return bonobo_property_control_construct (property_control, corba_control,
						  get_fn, closure);
}