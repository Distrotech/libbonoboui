/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME BonoboWidget object.
 *
 * Authors:
 *   Nat Friedman    (nat@helixcode.com)
 *
 * Copyright 1999 Helix Code, Inc.
 * 
 * Bonobo component embedding for hydrocephalic imbeciles.
 *
 * Pure cane sugar.
 *
 * This purpose of BonoboWidget is to make container-side use of
 * Bonobo as easy as pie.  This widget has two functions:
 *
 *   1. Provide a simple wrapper for embedding a single-view
 *      subdocument.  In this case, BonoboWidget handles creating
 *      the embeddable, binding it to a local BonoboClientSite,
 *      creating a view for it, and displaying the view.  You can use
 *      the accessor functions (bonobo_widget_get_view_frame,
 *      etc) to get at the actual Bonobo objects which underlie the
 *      whole process.
 *
 *      In order to do this, just call:
 * 
 *        bw = bonobo_widget_new_subdoc ("goad id of subdoc embddable",
 *                                             top_level_uihandler);
 * 
 *      And then insert the 'bw' widget into the widget tree of your
 *      application like so:
 *
 *        gtk_container_add (some_container, bw);
 *
 *      You are free to make the UIHandler argument to
 *      bonobo_widget_new_subdoc() be NULL.
 *
 *   2. Provide a simple wrapper for embedding Controls.  Embedding
 *      controls is already really easy, but BonoboWidget reduces
 *      the work from about 5 lines to 1.  To embed a given control,
 *      just do:
 *
 *        bw = bonobo_widget_new_control ("goad id for control");
 *        gtk_container_add (some_container, bw);
 *
 *      Ta da!
 *
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-widget.h>

struct _BonoboWidgetPrivate {

	BonoboObjectClient *server;

	/*
	 * Control stuff.
	 */
	BonoboControlFrame *control_frame;
	
	/*
	 * Subdocument (Embeddable/View) things.
	 */
	BonoboContainer	   *container;
	BonoboClientSite   *client_site;
	BonoboViewFrame    *view_frame;
	Bonobo_UIHandler    uih;
};

static BonoboWrapperClass *bonobo_widget_parent_class;

static BonoboObjectClient *
bonobo_widget_launch_component (const char *object_desc)
{
	BonoboObjectClient *server;

	server = bonobo_object_activate (object_desc, 0);

	return server;
}


/*
 *
 * Control support for BonoboWidget.
 *
 */
static BonoboWidget *
bonobo_widget_construct_control_from_objref (BonoboWidget     *bw,
					     Bonobo_Control    control,
					     Bonobo_UIHandler  uih)
{
	GtkWidget    *control_frame_widget;

	/*
	 * Create a local ControlFrame for it.
	 */
	bw->priv->control_frame = bonobo_control_frame_new (uih);

	bonobo_control_frame_bind_to_control (bw->priv->control_frame, control);

	bonobo_control_frame_set_autoactivate (bw->priv->control_frame, TRUE);

	/*
	 * Grab the actual widget which visually contains the remote
	 * Control.  This is a GtkSocket, in reality.
	 */
	control_frame_widget = bonobo_control_frame_get_widget (bw->priv->control_frame);

	/*
	 * Now stick it into this BonoboWidget.
	 */
	gtk_container_add (GTK_CONTAINER (bw),
			   control_frame_widget);
	gtk_widget_show (control_frame_widget);

	if (uih != CORBA_OBJECT_NIL)
		bw->priv->uih = bonobo_object_dup_ref (uih, NULL);

	return bw;
}

static BonoboWidget *
bonobo_widget_construct_control (BonoboWidget     *bw,
				 const char       *goad_id,
				 Bonobo_UIHandler  uih)
{
	Bonobo_Control control;

	/*
	 * Create the remote Control object.
	 */
	bw->priv->server = bonobo_widget_launch_component (goad_id);
	if (bw->priv->server == NULL) {
		gtk_object_unref (GTK_OBJECT (bw));
		return NULL;
	}

	control = bonobo_object_corba_objref (BONOBO_OBJECT (bw->priv->server));

	return bonobo_widget_construct_control_from_objref (bw, control, uih);
}

GtkWidget *
bonobo_widget_new_control_from_objref (Bonobo_Control   control,
				       Bonobo_UIHandler uih)
{
	BonoboWidget *bw;

	g_return_val_if_fail (control != CORBA_OBJECT_NIL, NULL);

	bw = gtk_type_new (BONOBO_WIDGET_TYPE);

	bw = bonobo_widget_construct_control_from_objref (bw, control, uih);

	if (bw == NULL)
		return NULL;

	return GTK_WIDGET (bw);
}

GtkWidget *
bonobo_widget_new_control (const char       *goad_id,
			   Bonobo_UIHandler  uih)
{
	BonoboWidget *bw;

	g_return_val_if_fail (goad_id != NULL, NULL);

	bw = gtk_type_new (BONOBO_WIDGET_TYPE);

	bw = bonobo_widget_construct_control (bw, goad_id, uih);

	if (bw == NULL)
		return NULL;
	else
		return GTK_WIDGET (bw);
}

BonoboControlFrame *
bonobo_widget_get_control_frame (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->control_frame;
}




/*
 *
 * Subdocument support for BonoboWidget.
 *
 */
static BonoboWidget *
bonobo_widget_create_subdoc_object (BonoboWidget     *bw,
				    const char       *object_desc,
				    Bonobo_UIHandler  uih)
{
	GtkWidget *view_widget;
	
	/*
	 * Create the BonoboContainer.  This will contain
	 * just one BonoboClientSite.
	 */
	bw->priv->container = bonobo_container_new ();

	bw->priv->server = bonobo_widget_launch_component (object_desc);
	if (bw->priv->server == NULL)
		return NULL;
	
	/*
	 * Create the client site.  This is the container-side point
	 * of contact for the remote component.
	 */
	bw->priv->client_site = bonobo_client_site_new (bw->priv->container);

	/*
	 * Bind the local ClientSite object to the remote Embeddable
	 * component.
	 */
	if (!bonobo_client_site_bind_embeddable (bw->priv->client_site, bw->priv->server))
		return NULL;

	/*
	 * Add the client site to the container.  This container is
	 * basically just there as a place holder; this is the only
	 * client site that will ever be added to it.
	 */
	bonobo_container_add (bw->priv->container, BONOBO_OBJECT (bw->priv->client_site));

	/*
	 * Now create a new view for the remote object.
	 */
	bw->priv->view_frame = bonobo_client_site_new_view (bw->priv->client_site, uih);

	/*
	 * Add the view frame.
	 */
	view_widget = bonobo_view_frame_get_wrapper (bw->priv->view_frame);
	gtk_container_add (GTK_CONTAINER (bw), view_widget);
	gtk_widget_show (view_widget);

	if (uih != CORBA_OBJECT_NIL){
		CORBA_Environment ev;
		
		CORBA_exception_init (&ev);

		Bonobo_UIHandler_ref (uih, &ev);
		if (ev._major == CORBA_NO_EXCEPTION)
			bw->priv->uih = CORBA_Object_duplicate (uih, &ev);
		CORBA_exception_free (&ev);
	}
	
	return bw;
}

GtkWidget *
bonobo_widget_new_subdoc (const char       *object_desc,
			  Bonobo_UIHandler  uih)
{
	BonoboWidget *bw;

	g_return_val_if_fail (object_desc != NULL, NULL);

	bw = gtk_type_new (BONOBO_WIDGET_TYPE);

	if (bw == NULL)
		return NULL;

	if (!bonobo_widget_create_subdoc_object (bw, object_desc, uih)){
		gtk_object_destroy (GTK_OBJECT (bw));
		return NULL;
	}
 
	bonobo_view_frame_set_covered (bw->priv->view_frame, FALSE);

	return GTK_WIDGET (bw);
}


BonoboContainer *
bonobo_widget_get_container (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->container;
}

BonoboClientSite *
bonobo_widget_get_client_site (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->client_site;
}

BonoboViewFrame *
bonobo_widget_get_view_frame (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->view_frame;
}

Bonobo_UIHandler
bonobo_widget_get_uih (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->uih;
}



/*
 *
 * Generic (non-control/subdoc specific) BonoboWidget stuff.
 *
 */
BonoboObjectClient *
bonobo_widget_get_server (BonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WIDGET (bw), NULL);

	return bw->priv->server;
}

static void
bonobo_widget_destroy (GtkObject *object)
{
	BonoboWidget *bw = BONOBO_WIDGET (object);
	BonoboWidgetPrivate *priv = bw->priv;
	
	if (priv->control_frame)
		bonobo_object_unref (BONOBO_OBJECT (priv->control_frame));
	if (priv->container)
		bonobo_object_unref (BONOBO_OBJECT (priv->container));
	if (priv->client_site)
		bonobo_object_unref (BONOBO_OBJECT (priv->client_site));
	if (priv->view_frame)
		bonobo_object_unref (BONOBO_OBJECT (priv->view_frame));
	if (priv->uih != CORBA_OBJECT_NIL)
		bonobo_object_release_unref (priv->uih, NULL);

	g_free (priv);
	GTK_OBJECT_CLASS (bonobo_widget_parent_class)->destroy (object);
}

static void
bonobo_widget_size_request (GtkWidget *widget,
			    GtkRequisition *requisition)
{
	GtkBin *bin;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_WIDGET (widget));
	g_return_if_fail (requisition != NULL);

	bin = GTK_BIN (widget);

	if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
		GtkRequisition child_requisition;
      
		gtk_widget_size_request (bin->child, &child_requisition);

		requisition->width = child_requisition.width;
		requisition->height = child_requisition.height;
	}
}

static void
bonobo_widget_size_allocate (GtkWidget *widget,
			     GtkAllocation *allocation)
{
	GtkBin *bin;
	GtkAllocation child_allocation;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_WIDGET (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	bin = GTK_BIN (widget);

	child_allocation.x = allocation->x;
	child_allocation.y = allocation->y;
	child_allocation.width = allocation->width;
	child_allocation.height = allocation->height;

	if (bin->child) {
		gtk_widget_size_allocate (bin->child, &child_allocation);
	}
}

static void
bonobo_widget_class_init (BonoboWidgetClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	bonobo_widget_parent_class = gtk_type_class (GTK_TYPE_BIN);

	widget_class->size_request = bonobo_widget_size_request;
	widget_class->size_allocate = bonobo_widget_size_allocate;

	object_class->destroy = bonobo_widget_destroy;
}

static void
bonobo_widget_init (BonoboWidget *bw)
{
	bw->priv = g_new0 (BonoboWidgetPrivate, 1);
}

GtkType
bonobo_widget_get_type (void)
{
	static GtkType type = 0;

	if (! type) {
		static const GtkTypeInfo info = {
			"BonoboWidget",
			sizeof (BonoboWidget),
			sizeof (BonoboWidgetClass),
			(GtkClassInitFunc) bonobo_widget_class_init,
			(GtkObjectInitFunc) bonobo_widget_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_bin_get_type (), &info);
	}

	return type;
}

void
bonobo_widget_set_property (BonoboWidget      *control,
			    const char        *first_prop, ...)
{
	Bonobo_PropertyBag pb;
	CORBA_Environment  ev;

	va_list args;
	va_start (args, first_prop);

	g_return_if_fail (control != NULL);
	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (control->priv != NULL);
	g_return_if_fail (BONOBO_IS_WIDGET (control));

	CORBA_exception_init (&ev);
	
	pb = bonobo_control_frame_get_control_property_bag (
		control->priv->control_frame, &ev);

	if (ev._major != CORBA_NO_EXCEPTION)
		g_warning ("Error getting property bag from control");
	else {
		/* FIXME: this should use ev */
		char *err = bonobo_property_bag_client_setv (pb, &ev, first_prop, args);

		if (err)
			g_warning ("Error '%s'", err);
	}

	bonobo_object_release_unref (pb, &ev);

	CORBA_exception_free (&ev);

	va_end (args);
}


void
bonobo_widget_get_property (BonoboWidget      *control,
			    const char        *first_prop, ...)
{
	Bonobo_PropertyBag pb;
	CORBA_Environment  ev;

	va_list args;
	va_start (args, first_prop);

	g_return_if_fail (control != NULL);
	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (control->priv != NULL);
	g_return_if_fail (BONOBO_IS_WIDGET (control));

	CORBA_exception_init (&ev);
	
	pb = bonobo_control_frame_get_control_property_bag (
		control->priv->control_frame, &ev);

	if (ev._major != CORBA_NO_EXCEPTION)
		g_warning ("Error getting property bag from control");
	else {
		/* FIXME: this should use ev */
		char *err = bonobo_property_bag_client_getv (pb, &ev, first_prop, args);

		if (err)
			g_warning ("Error '%s'", err);
	}

	bonobo_object_release_unref (pb, &ev);

	CORBA_exception_free (&ev);

	va_end (args);
}
