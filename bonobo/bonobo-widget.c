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
 * This purpose of GnomeBonoboWidget is to make container-side use of
 * Bonobo as easy as pie.  This widget has two functions:
 *
 *   1. Provide a simple wrapper for embedding a single-view
 *      subdocument.  In this case, GnomeBonoboWidget handles creating
 *      the embeddable, binding it to a local GnomeClientSite,
 *      creating a view for it, and displaying the view.  You can use
 *      the accessor functions (gnome_bonobo_widget_get_view_frame,
 *      etc) to get at the actual Bonobo objects which underlie the
 *      whole process.
 *
 *      In order to do this, just call:
 * 
 *        bw = gnome_bonobo_widget_new_subdoc ("goad id of subdoc embddable",
 *                                             top_level_uihandler);
 * 
 *      And then insert the 'bw' widget into the widget tree of your
 *      application like so:
 *
 *        gtk_container_add (some_container, bw);
 *
 *      You are free to make the UIHandler argument to
 *      gnome_bonobo_widget_new_subdoc() be NULL.
 *
 *   2. Provide a simple wrapper for embedding Controls.  Embedding
 *      controls is already really easy, but GnomeBonoboWidget reduces
 *      the work from about 5 lines to 1.  To embed a given control,
 *      just do:
 *
 *        bw = gnome_bonobo_widget_new_control ("goad id for control");
 *        gtk_container_add (some_container, bw);
 *
 *      Ta da!
 *
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-bonobo-widget.h>

struct _GnomeBonoboWidgetPrivate {

	GnomeObjectClient *server;

	/*
	 * Control stuff.
	 */
	GnomeControlFrame *control_frame;
	
	/*
	 * Subdocument (Embeddable/View) things.
	 */
	GnomeContainer	  *container;
	GnomeClientSite   *client_site;
	GnomeViewFrame    *view_frame;
	GnomeUIHandler	  *uih;

};

static GnomeWrapperClass *gnome_bonobo_widget_parent_class;

static GnomeObjectClient *
gnome_bonobo_widget_launch_component (char *object_desc)
{
	GnomeObjectClient *server;

	server = gnome_object_activate (object_desc, (GoadActivationFlags) 0);

	return server;
}


/*
 *
 * Control support for GnomeBonoboWidget.
 *
 */
static GnomeBonoboWidget *
gnome_bonobo_widget_construct_control_from_objref (GnomeBonoboWidget *bw,
						   GNOME_Control control)
{
	GtkWidget    *control_frame_widget;

	/*
	 * Create a local ControlFrame for it.
	 */
	bw->priv->control_frame = gnome_control_frame_new ();
	gnome_control_frame_bind_to_control (bw->priv->control_frame,
					     control);

	/*
	 * Grab the actual widget which visually contains the remote
	 * Control.  This is a GtkSocket, in reality.
	 */
	control_frame_widget = gnome_control_frame_get_widget (bw->priv->control_frame);

	/*
	 * Now stick it into this GnomeBonoboWidget.
	 */
	gtk_container_add (GTK_CONTAINER (bw),
			   control_frame_widget);
	gtk_widget_show (control_frame_widget);
	
	return bw;
}

static GnomeBonoboWidget *
gnome_bonobo_widget_construct_control (GnomeBonoboWidget *bw,
				       char *goad_id)
{
	GNOME_Control control;

	/*
	 * Create the remote Control object.
	 */
	bw->priv->server = gnome_bonobo_widget_launch_component (goad_id);
	if (bw->priv->server == NULL) {
		gtk_object_unref (GTK_OBJECT (bw));
		return NULL;
	}

	control = gnome_object_corba_objref (GNOME_OBJECT (bw->priv->server));

	return gnome_bonobo_widget_construct_control_from_objref (bw, control);
}

GtkWidget *
gnome_bonobo_widget_new_control_from_objref (GNOME_Control control)
{
	GnomeBonoboWidget *bw;

	g_return_val_if_fail (control != CORBA_OBJECT_NIL, NULL);

	bw = gtk_type_new (GNOME_BONOBO_WIDGET_TYPE);

	bw = gnome_bonobo_widget_construct_control_from_objref (bw, control);

	if (bw == NULL)
		return NULL;

	return GTK_WIDGET (bw);
}

GtkWidget *
gnome_bonobo_widget_new_control (char *goad_id)
{
	GnomeBonoboWidget *bw;

	g_return_val_if_fail (goad_id != NULL, NULL);

	bw = gtk_type_new (GNOME_BONOBO_WIDGET_TYPE);

	bw = gnome_bonobo_widget_construct_control (bw, goad_id);

	if (bw == NULL)
		return NULL;
	else
		return GTK_WIDGET (bw);
}

GnomeControlFrame *
gnome_bonobo_widget_get_control_frame (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->control_frame;
}




/*
 *
 * Subdocument support for GnomeBonoboWidget.
 *
 */
static void
gnome_bonobo_widget_create_subdoc_object (GnomeBonoboWidget *bw, char *object_desc)
{
	GtkWidget *view_widget;

	/*
	 * Create the GnomeContainer.  This will contain
	 * just one GnomeClientSite.
	 */
	bw->priv->container = gnome_container_new ();

	bw->priv->server = gnome_bonobo_widget_launch_component (object_desc);

	/*
	 * Create the client site.  This is the container-side point
	 * of contact for the remote component.
	 */
	bw->priv->client_site = gnome_client_site_new (bw->priv->container);

	/*
	 * Bind the local ClientSite object to the remote Embeddable
	 * component.
	 */
	gnome_client_site_bind_embeddable (bw->priv->client_site, bw->priv->server);

	/*
	 * Add the client site to the container.  This container is
	 * basically just there as a place holder; this is the only
	 * client site that will ever be added to it.
	 */
	gnome_container_add (bw->priv->container, GNOME_OBJECT (bw->priv->client_site));

	/*
	 * Now create a new view for the remote object.
	 */
	bw->priv->view_frame = gnome_client_site_new_view (bw->priv->client_site);

	/*
	 * Add the view frame.
	 */
	view_widget = gnome_view_frame_get_wrapper (bw->priv->view_frame);
	gtk_container_add (GTK_CONTAINER (bw), view_widget);
	gtk_widget_show (view_widget);
}

GtkWidget *
gnome_bonobo_widget_new_subdoc (char *object_desc,
				GnomeUIHandler *uih)
{
	GnomeBonoboWidget *bw;

	g_return_val_if_fail (object_desc != NULL, NULL);

	bw = gtk_type_new (GNOME_BONOBO_WIDGET_TYPE);

	if (bw == NULL)
		return NULL;

	gnome_bonobo_widget_create_subdoc_object (bw, object_desc);

	if (bw == NULL)
		return NULL;
 
	gnome_view_frame_set_covered (bw->priv->view_frame, FALSE);

	return GTK_WIDGET (bw);
}


GnomeContainer *
gnome_bonobo_widget_get_container (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->container;
}

GnomeClientSite *
gnome_bonobo_widget_get_client_site (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->client_site;
}

GnomeViewFrame *
gnome_bonobo_widget_get_view_frame (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->view_frame;
}

GnomeUIHandler *
gnome_bonobo_widget_get_uih (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->uih;
}



/*
 *
 * Generic (non-control/subdoc specific) GnomeBonoboWidget stuff.
 *
 */
GnomeObjectClient *
gnome_bonobo_widget_get_server (GnomeBonoboWidget *bw)
{
	g_return_val_if_fail (bw != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_WIDGET (bw), NULL);

	return bw->priv->server;
}

static void
gnome_bonobo_widget_destroy (GtkObject *object)
{
}

static void
gnome_bonobo_widget_size_request (GtkWidget *widget,
				  GtkRequisition *requisition)
{
	GtkBin *bin;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_WIDGET (widget));
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
gnome_bonobo_widget_size_allocate (GtkWidget *widget,
				   GtkAllocation *allocation)
{
	GtkBin *bin;
	GtkAllocation child_allocation;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_WIDGET (widget));
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
gnome_bonobo_widget_class_init (GnomeBonoboWidgetClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	gnome_bonobo_widget_parent_class = gtk_type_class (GTK_TYPE_BIN);

	widget_class->size_request = gnome_bonobo_widget_size_request;
	widget_class->size_allocate = gnome_bonobo_widget_size_allocate;

	object_class->destroy = gnome_bonobo_widget_destroy;
}

static void
gnome_bonobo_widget_init (GnomeBonoboWidget *bw)
{
	bw->priv = g_new0 (GnomeBonoboWidgetPrivate, 1);
}

GtkType
gnome_bonobo_widget_get_type (void)
{
	static GtkType type = 0;

	if (! type) {
		static const GtkTypeInfo info = {
			"GnomeBonoboWidget",
			sizeof (GnomeBonoboWidget),
			sizeof (GnomeBonoboWidgetClass),
			(GtkClassInitFunc) gnome_bonobo_widget_class_init,
			(GtkObjectInitFunc) gnome_bonobo_widget_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_bin_get_type (), &info);
	}

	return type;
}
