/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  Bonobo::ZoomableFrame - container side part of Bonobo::Zoomable.
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *                2000 SuSE GmbH.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Maciej Stachowiak <mjs@eazel.com>
 *           Martin Baulig <baulig@suse.de>
 *
 */

#include <config.h>
#include <bonobo/bonobo-zoomable-frame.h>
#include <gtk/gtksignal.h>

#undef ZOOMABLE_DEBUG

static BonoboObjectClass   *bonobo_zoomable_frame_parent_class;
static BonoboZoomableFrameClass *bonobo_zoomable_frame_class;

struct _BonoboZoomableFramePrivate {
	Bonobo_Zoomable			 zoomable;
};

enum {
	ZOOM_LEVEL_CHANGED,
	ZOOM_PARAMETERS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

typedef struct {
	POA_Bonobo_ZoomableFrame	 servant;

	BonoboZoomableFrame		*gtk_object;
} impl_POA_Bonobo_ZoomableFrame;

POA_Bonobo_ZoomableFrame__vepv bonobo_zoomable_frame_vepv;

#define CLASS(o) BONOBO_ZOOMABLE_FRAME_CLASS(GTK_OBJECT(o)->klass)

static inline BonoboZoomableFrame *
bonobo_zoomable_frame_from_servant (PortableServer_Servant _servant)
{
	if (!BONOBO_IS_ZOOMABLE_FRAME (bonobo_object_from_servant (_servant)))
		return NULL;
	else
		return BONOBO_ZOOMABLE_FRAME (bonobo_object_from_servant (_servant));
}


/**
 * bonobo_zoomable_frame_get_epv:
 */
POA_Bonobo_ZoomableFrame__epv *
bonobo_zoomable_frame_get_epv (void)
{
	POA_Bonobo_ZoomableFrame__epv *epv;

	epv = g_new0 (POA_Bonobo_ZoomableFrame__epv, 1);

	return epv;
}

static void
init_zoomable_corba_class (void)
{
	/* The VEPV */
	bonobo_zoomable_frame_vepv.Bonobo_Unknown_epv  = bonobo_object_get_epv ();
	bonobo_zoomable_frame_vepv.Bonobo_ZoomableFrame_epv = bonobo_zoomable_frame_get_epv ();
}

static void
marshal_NONE__FLOAT (GtkObject *object,
		      GtkSignalFunc func,
		      gpointer func_data,
		      GtkArg *args)
{
	(* (void (*)(GtkObject *, float, gpointer)) func)
		(object,
		 GTK_VALUE_FLOAT (args[0]),
		 func_data);
}

static void
bonobo_zoomable_frame_class_init (BonoboZoomableFrameClass *klass)
{
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass*) klass;
	
	bonobo_zoomable_frame_parent_class = gtk_type_class (bonobo_object_get_type ());
	bonobo_zoomable_frame_class = klass;

	signals[ZOOM_LEVEL_CHANGED] =
		gtk_signal_new ("zoom_level_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableFrameClass, zoom_level_changed),
				marshal_NONE__FLOAT,
				GTK_TYPE_NONE, 1, GTK_TYPE_FLOAT);
	signals[ZOOM_PARAMETERS_CHANGED] =
		gtk_signal_new ("zoom_parameters_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableFrameClass, zoom_parameters_changed),
				marshal_NONE__FLOAT,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	init_zoomable_corba_class ();
}

static void
bonobo_zoomable_frame_init (BonoboZoomableFrame *zoomable)
{
	zoomable->priv = g_new0 (BonoboZoomableFramePrivate, 1);

}

/**
 * bonobo_zoomable_frame_get_type:
 *
 * Returns: the GtkType for a BonoboZoomableFrame object.
 */
GtkType
bonobo_zoomable_frame_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboZoomableFrame",
			sizeof (BonoboZoomableFrame),
			sizeof (BonoboZoomableFrameClass),
			(GtkClassInitFunc) bonobo_zoomable_frame_class_init,
			(GtkObjectInitFunc) bonobo_zoomable_frame_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

Bonobo_ZoomableFrame
bonobo_zoomable_frame_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_ZoomableFrame *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_ZoomableFrame *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_zoomable_frame_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_ZoomableFrame__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
		CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

	CORBA_exception_free (&ev);
	return (Bonobo_ZoomableFrame) bonobo_object_activate_servant (object, servant);
}

BonoboZoomableFrame *
bonobo_zoomable_frame_construct (BonoboZoomableFrame	*p,
				 Bonobo_ZoomableFrame	 corba_p)
{
	g_return_val_if_fail (p != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_ZOOMABLE_FRAME (p), NULL);
	g_return_val_if_fail (corba_p != NULL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (p), corba_p);

	return p;
}

/**
 * bonobo_zoomable_frame_new:
 * 
 * Create a new bonobo-zoomable implementing BonoboObject
 * interface.
 * 
 * Return value: 
 **/
BonoboZoomableFrame *
bonobo_zoomable_frame_new (void)
{
	BonoboZoomableFrame	*p;
	Bonobo_ZoomableFrame	 corba_p;

	p = gtk_type_new (bonobo_zoomable_frame_get_type ());
	g_return_val_if_fail (p != NULL, NULL);

	corba_p = bonobo_zoomable_frame_corba_object_create (BONOBO_OBJECT (p));
	if (corba_p == CORBA_OBJECT_NIL){
		bonobo_object_unref (BONOBO_OBJECT (p));
		return NULL;
	}

	return bonobo_zoomable_frame_construct (p, corba_p);
}

/**
 * bonobo_zoomable_frame_bind_to_zoomable:
 * @zoomable_frame: A BonoboZoomableFrame object.
 * @zoomable: The CORBA object for the BonoboZoomable embedded
 * in this BonoboZoomableFrame.
 *
 * Associates @zoomable with this @zoomable_frame.
 */
void
bonobo_zoomable_frame_bind_to_zoomable (BonoboZoomableFrame *zoomable_frame, Bonobo_Zoomable zoomable)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable != CORBA_OBJECT_NIL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame));

	/*
	 * Keep a local handle to the Zoomable.
	 */
	if (zoomable_frame->priv->zoomable != CORBA_OBJECT_NIL)
		g_warning ("FIXME: leaking zoomable reference");

	zoomable_frame->priv->zoomable = bonobo_object_dup_ref (zoomable, NULL);

	/*
	 * Introduce ourselves to the Zoomable.
	 */
	CORBA_exception_init (&ev);
	Bonobo_Zoomable_set_frame (zoomable,
				   bonobo_object_corba_objref (BONOBO_OBJECT (zoomable_frame)),
				   &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		bonobo_object_check_env (BONOBO_OBJECT (zoomable_frame), zoomable, &ev);
	CORBA_exception_free (&ev);
}

/**
 * bonobo_zoomable_frame_get_zoomable:
 * @zoomable_frame: A BonoboZoomableFrame which is bound to a remote
 * BonoboZoomable.
 *
 * Returns: The Bonobo_Zoomable CORBA interface for the remote Zoomable
 * which is bound to @frame.  See also
 * bonobo_zoomable_frame_bind_to_zoomable().
 */
Bonobo_Zoomable
bonobo_zoomable_frame_get_zoomable (BonoboZoomableFrame *zoomable_frame)
{
	g_return_val_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame), CORBA_OBJECT_NIL);

	return zoomable_frame->priv->zoomable;
}

void
bonobo_zoomable_frame_zoom_in (BonoboZoomableFrame *zoomable_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable_frame != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame));
	g_return_if_fail (zoomable_frame->priv->zoomable != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	Bonobo_Zoomable_zoom_in (zoomable_frame->priv->zoomable, &ev);
	bonobo_object_check_env (BONOBO_OBJECT (zoomable_frame),
				 zoomable_frame->priv->zoomable, &ev);
	CORBA_exception_free (&ev);
}

void
bonobo_zoomable_frame_zoom_out (BonoboZoomableFrame *zoomable_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable_frame != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame));
	g_return_if_fail (zoomable_frame->priv->zoomable != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	Bonobo_Zoomable_zoom_out (zoomable_frame->priv->zoomable, &ev);
	bonobo_object_check_env (BONOBO_OBJECT (zoomable_frame),
				 zoomable_frame->priv->zoomable, &ev);
	CORBA_exception_free (&ev);
}

void
bonobo_zoomable_frame_zoom_to_fit (BonoboZoomableFrame *zoomable_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable_frame != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame));
	g_return_if_fail (zoomable_frame->priv->zoomable != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	Bonobo_Zoomable_zoom_to_fit (zoomable_frame->priv->zoomable, &ev);
	bonobo_object_check_env (BONOBO_OBJECT (zoomable_frame),
				 zoomable_frame->priv->zoomable, &ev);
	CORBA_exception_free (&ev);
}

void
bonobo_zoomable_frame_zoom_to_default (BonoboZoomableFrame *zoomable_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable_frame != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE_FRAME (zoomable_frame));
	g_return_if_fail (zoomable_frame->priv->zoomable != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	Bonobo_Zoomable_zoom_to_default (zoomable_frame->priv->zoomable, &ev);
	bonobo_object_check_env (BONOBO_OBJECT (zoomable_frame),
				 zoomable_frame->priv->zoomable, &ev);
	CORBA_exception_free (&ev);
}
