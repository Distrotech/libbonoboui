/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  Bonobo::Zoomable - zoomable interface for Controls.
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
#include <bonobo/bonobo-zoomable.h>
#include <bonobo/bonobo-property-bag.h>
#include <gtk/gtksignal.h>

#undef ZOOMABLE_DEBUG

static BonoboObjectClass   *bonobo_zoomable_parent_class;
static BonoboZoomableClass *bonobo_zoomable_class;

struct _BonoboZoomablePrivate {
	CORBA_float		 zoom_level;

	CORBA_float		 min_zoom_level;
	CORBA_float		 max_zoom_level;
	CORBA_boolean		 has_min_zoom_level;
	CORBA_boolean		 has_max_zoom_level;
	CORBA_boolean		 is_continuous;

	Bonobo_ZoomLevel	*preferred_zoom_levels;
	int			 num_preferred_zoom_levels;

	Bonobo_ZoomableFrame	 zoomable_frame;
};

enum {
	SET_ZOOM_LEVEL,
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_TO_FIT,
	ZOOM_TO_DEFAULT,
	LAST_SIGNAL
};

enum {
	ARG_0,
	ARG_ZOOM_LEVEL,
	ARG_MIN_ZOOM_LEVEL,
	ARG_MAX_ZOOM_LEVEL,
	ARG_HAS_MIN_ZOOM_LEVEL,
	ARG_HAS_MAX_ZOOM_LEVEL,
	ARG_IS_CONTINUOUS
};

static guint signals[LAST_SIGNAL];

typedef struct {
	POA_Bonobo_Zoomable	servant;
	
	BonoboZoomable		*gtk_object;
} impl_POA_Bonobo_Zoomable;

POA_Bonobo_Zoomable__vepv bonobo_zoomable_vepv;

#define CLASS(o) BONOBO_ZOOMABLE_CLASS(GTK_OBJECT(o)->klass)

static inline BonoboZoomable *
bonobo_zoomable_from_servant (PortableServer_Servant _servant)
{
	if (!BONOBO_IS_ZOOMABLE (bonobo_object_from_servant (_servant)))
		return NULL;
	else
		return BONOBO_ZOOMABLE (bonobo_object_from_servant (_servant));
}

static CORBA_float
impl_Bonobo_Zoomable__get_zoom_level (PortableServer_Servant  _servant,
				      CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->zoom_level;
}

static CORBA_float
impl_Bonobo_Zoomable__get_min_zoom_level (PortableServer_Servant  _servant,
					  CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->min_zoom_level;
}

static CORBA_float
impl_Bonobo_Zoomable__get_max_zoom_level (PortableServer_Servant  _servant,
					  CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->max_zoom_level;
}

static CORBA_boolean
impl_Bonobo_Zoomable__get_has_min_zoom_level (PortableServer_Servant  _servant,
					      CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->has_min_zoom_level;
}

static CORBA_boolean
impl_Bonobo_Zoomable__get_has_max_zoom_level (PortableServer_Servant  _servant,
					      CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->has_max_zoom_level;
}

static CORBA_boolean
impl_Bonobo_Zoomable__get_is_continuous (PortableServer_Servant  _servant,
					 CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	return zoomable->priv->is_continuous;
}

static Bonobo_ZoomLevelList *
impl_Bonobo_Zoomable__get_preferred_zoom_levels (PortableServer_Servant  _servant,
						 CORBA_Environment      *ev)
{
	Bonobo_ZoomLevelList *list;
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);

	list = Bonobo_ZoomLevelList__alloc ();
	list->_maximum = zoomable->priv->num_preferred_zoom_levels;
	list->_length = zoomable->priv->num_preferred_zoom_levels;
	list->_buffer = zoomable->priv->preferred_zoom_levels;
	
	/*  set_release defaults to FALSE - CORBA_sequence_set_release (list, FALSE) */ 

	return list;
}

static void 
impl_Bonobo_Zoomable_set_zoom_level (PortableServer_Servant  _servant,
				      const CORBA_float      zoom_level,
				      CORBA_Environment      *ev)
{
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	gtk_signal_emit (GTK_OBJECT (zoomable), signals[SET_ZOOM_LEVEL], zoom_level);
}

static void
impl_Bonobo_Zoomable_zoom_in (PortableServer_Servant  _servant,
			      CORBA_Environment      *ev)
{	
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	gtk_signal_emit (GTK_OBJECT (zoomable), signals[ZOOM_IN]);
}

static void
impl_Bonobo_Zoomable_zoom_out (PortableServer_Servant  _servant,
			       CORBA_Environment      *ev)
{	
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	gtk_signal_emit (GTK_OBJECT (zoomable), signals[ZOOM_OUT]);
}

static void
impl_Bonobo_Zoomable_zoom_to_fit (PortableServer_Servant  _servant,
				  CORBA_Environment      *ev)
{	
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	gtk_signal_emit (GTK_OBJECT (zoomable), signals[ZOOM_TO_FIT]);
}

static void
impl_Bonobo_Zoomable_zoom_to_default (PortableServer_Servant  _servant,
				      CORBA_Environment      *ev)
{	
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);
	gtk_signal_emit (GTK_OBJECT (zoomable), signals[ZOOM_TO_DEFAULT]);
}

static void
impl_Bonobo_Zoomable_set_frame (PortableServer_Servant  _servant,
				Bonobo_ZoomableFrame	zoomable_frame,
				CORBA_Environment      *ev)
{	
	BonoboZoomable *zoomable;

	zoomable = bonobo_zoomable_from_servant (_servant);

	g_assert (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL);
	zoomable->priv->zoomable_frame = CORBA_Object_duplicate (zoomable_frame, ev);
}


/**
 * bonobo_zoomable_get_epv:
 */
POA_Bonobo_Zoomable__epv *
bonobo_zoomable_get_epv (void)
{
	POA_Bonobo_Zoomable__epv *epv;

	epv = g_new0 (POA_Bonobo_Zoomable__epv, 1);

	epv->_get_zoom_level = impl_Bonobo_Zoomable__get_zoom_level;
	epv->_get_min_zoom_level = impl_Bonobo_Zoomable__get_min_zoom_level;
	epv->_get_max_zoom_level = impl_Bonobo_Zoomable__get_max_zoom_level;
	epv->_get_has_min_zoom_level = impl_Bonobo_Zoomable__get_has_min_zoom_level;
	epv->_get_has_max_zoom_level = impl_Bonobo_Zoomable__get_has_max_zoom_level;
	epv->_get_is_continuous = impl_Bonobo_Zoomable__get_is_continuous;
	epv->_get_preferred_zoom_levels = impl_Bonobo_Zoomable__get_preferred_zoom_levels;

	epv->zoom_in = impl_Bonobo_Zoomable_zoom_in;
	epv->zoom_out = impl_Bonobo_Zoomable_zoom_out;
	epv->zoom_to_fit = impl_Bonobo_Zoomable_zoom_to_fit;
	epv->zoom_to_default = impl_Bonobo_Zoomable_zoom_to_default;

	epv->set_zoom_level = impl_Bonobo_Zoomable_set_zoom_level;
	epv->set_frame = impl_Bonobo_Zoomable_set_frame;
	
	return epv;
}

static void
init_zoomable_corba_class (void)
{
	/* The VEPV */
	bonobo_zoomable_vepv.Bonobo_Unknown_epv  = bonobo_object_get_epv ();
	bonobo_zoomable_vepv.Bonobo_Zoomable_epv = bonobo_zoomable_get_epv ();
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
impl_get_arg (GtkObject* obj, GtkArg* arg, guint arg_id)
{
	BonoboZoomable *zoomable = BONOBO_ZOOMABLE (obj);
	BonoboZoomablePrivate *priv = zoomable->priv;

	switch (arg_id) {
	case ARG_ZOOM_LEVEL:
		GTK_VALUE_FLOAT(*arg) = priv->zoom_level;
		break;
	case ARG_MIN_ZOOM_LEVEL:
		GTK_VALUE_FLOAT(*arg) = priv->min_zoom_level;
		break;
	case ARG_MAX_ZOOM_LEVEL:
		GTK_VALUE_FLOAT(*arg) = priv->max_zoom_level;
		break;
	case ARG_HAS_MIN_ZOOM_LEVEL:
		GTK_VALUE_BOOL(*arg) = priv->has_min_zoom_level;
		break;
	case ARG_HAS_MAX_ZOOM_LEVEL:
		GTK_VALUE_BOOL(*arg) = priv->has_max_zoom_level;
		break;
	case ARG_IS_CONTINUOUS:
		GTK_VALUE_BOOL(*arg) = priv->is_continuous;
		break;
	default:
		g_message ("Unknown arg_id `%d'", arg_id);
		break;
	};
}

static void
bonobo_zoomable_class_init (BonoboZoomableClass *klass)
{
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass*) klass;
	
	bonobo_zoomable_parent_class = gtk_type_class (bonobo_object_get_type ());
	bonobo_zoomable_class = klass;

	gtk_object_add_arg_type("BonoboZoomable::zoom_level",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_ZOOM_LEVEL);
	gtk_object_add_arg_type("BonoboZoomable::min_zoom_level",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_MIN_ZOOM_LEVEL);
	gtk_object_add_arg_type("BonoboZoomable::max_zoom_level",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_MAX_ZOOM_LEVEL);
	gtk_object_add_arg_type("BonoboZoomable::has_min_zoom_level",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_HAS_MIN_ZOOM_LEVEL);
	gtk_object_add_arg_type("BonoboZoomable::has_max_zoom_level",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_HAS_MAX_ZOOM_LEVEL);
	gtk_object_add_arg_type("BonoboZoomable::is_continuous",
				GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG_IS_CONTINUOUS);

	signals[SET_ZOOM_LEVEL] =
		gtk_signal_new ("set_zoom_level",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableClass, set_zoom_level),
				marshal_NONE__FLOAT,
				GTK_TYPE_NONE, 1, GTK_TYPE_FLOAT);
	signals[ZOOM_IN] = 
		gtk_signal_new ("zoom_in",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableClass, zoom_in),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	signals[ZOOM_OUT] = 
		gtk_signal_new ("zoom_out",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableClass, zoom_out),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	signals[ZOOM_TO_FIT] = 
		gtk_signal_new ("zoom_to_fit",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableClass, zoom_to_fit),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	signals[ZOOM_TO_DEFAULT] = 
		gtk_signal_new ("zoom_to_default",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboZoomableClass, zoom_to_default),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->get_arg = impl_get_arg;

	init_zoomable_corba_class ();
}

static void
bonobo_zoomable_init (BonoboZoomable *zoomable)
{
	zoomable->priv = g_new0 (BonoboZoomablePrivate, 1);

	zoomable->priv->zoom_level = 0.0;
	zoomable->priv->min_zoom_level = 0.0;
	zoomable->priv->max_zoom_level = 0.0;
	zoomable->priv->has_min_zoom_level = FALSE;
	zoomable->priv->has_max_zoom_level = FALSE;
	zoomable->priv->is_continuous = FALSE;
}

/**
 * bonobo_zoomable_get_type:
 *
 * Returns: the GtkType for a BonoboZoomable object.
 */
GtkType
bonobo_zoomable_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboZoomable",
			sizeof (BonoboZoomable),
			sizeof (BonoboZoomableClass),
			(GtkClassInitFunc) bonobo_zoomable_class_init,
			(GtkObjectInitFunc) bonobo_zoomable_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

Bonobo_Zoomable
bonobo_zoomable_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_Zoomable *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_Zoomable *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_zoomable_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_Zoomable__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
		CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

	CORBA_exception_free (&ev);
	return (Bonobo_Zoomable) bonobo_object_activate_servant (object, servant);
}

void
bonobo_zoomable_set_parameters (BonoboZoomable	*p,
				float		 min_zoom_level,
				float		 max_zoom_level,
				gboolean	 has_min_zoom_level,
				gboolean	 has_max_zoom_level,
				gboolean	 is_continuous,
				float		*preferred_zoom_levels,
				int		 num_preferred_zoom_levels)
{
	g_return_if_fail (p != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE (p));

	p->priv->min_zoom_level = min_zoom_level;
	p->priv->max_zoom_level = max_zoom_level;
	p->priv->has_min_zoom_level = has_min_zoom_level;
	p->priv->has_max_zoom_level = has_max_zoom_level;
	p->priv->is_continuous = is_continuous;

	if (p->priv->preferred_zoom_levels) {
		g_free (p->priv->preferred_zoom_levels);
		p->priv->preferred_zoom_levels = NULL;
		p->priv->num_preferred_zoom_levels = 0;
	}

	if (preferred_zoom_levels) {
		p->priv->preferred_zoom_levels = g_memdup
			(preferred_zoom_levels,
			 sizeof (float) * num_preferred_zoom_levels);
		p->priv->num_preferred_zoom_levels = num_preferred_zoom_levels;
	}
}

BonoboZoomable *
bonobo_zoomable_construct (BonoboZoomable	*p,
			   Bonobo_Zoomable	 corba_p)
{
	g_return_val_if_fail (p != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_ZOOMABLE (p), NULL);
	g_return_val_if_fail (corba_p != NULL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (p), corba_p);

	return p;
}

/**
 * bonobo_zoomable_new:
 * 
 * Create a new bonobo-zoomable implementing BonoboObject
 * interface.
 * 
 * Return value: 
 **/
BonoboZoomable *
bonobo_zoomable_new (void)
{
	BonoboZoomable	*p;
	Bonobo_Zoomable	 corba_p;

	p = gtk_type_new (bonobo_zoomable_get_type ());
	g_return_val_if_fail (p != NULL, NULL);

	corba_p = bonobo_zoomable_corba_object_create (BONOBO_OBJECT (p));
	if (corba_p == CORBA_OBJECT_NIL){
		bonobo_object_unref (BONOBO_OBJECT (p));
		return NULL;
	}

	return bonobo_zoomable_construct (p, corba_p);
}

void
bonobo_zoomable_report_zoom_level_changed (BonoboZoomable *zoomable,
					   float           new_zoom_level)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE (zoomable));

	zoomable->priv->zoom_level = new_zoom_level;

	if (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL)
		return;

	CORBA_exception_init (&ev);
	Bonobo_ZoomableFrame_report_zoom_level_changed (zoomable->priv->zoomable_frame,
							zoomable->priv->zoom_level,
							&ev);
	CORBA_exception_free (&ev);
}

void
bonobo_zoomable_report_zoom_parameters_changed (BonoboZoomable *zoomable)
{
	CORBA_Environment ev;

	g_return_if_fail (zoomable != NULL);
	g_return_if_fail (BONOBO_IS_ZOOMABLE (zoomable));

	if (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL)
		return;

	CORBA_exception_init (&ev);
	Bonobo_ZoomableFrame_report_zoom_parameters_changed (zoomable->priv->zoomable_frame,
							     &ev);
	CORBA_exception_free (&ev);
}
