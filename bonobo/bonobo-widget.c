/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME BonoboWidget object.
 *
 * Embedding Bonobo components for hydrocephalic imbeciles.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-bonobo-widget.h>

static GnomeWrapperClass *gnome_bonobo_widget_parent_class;

static GnomeObjectClient *
gnome_bonobo_widget_launch_component (char *object_desc)
{
	GnomeObjectClient *server;

	server = gnome_object_activate (object_desc, (GoadActivationFlags) 0);
}

static void
gnome_bonobo_widget_create_object (GnomeBonoboWidget *bw, char *object_desc)
{
	/*
	 * Create the GnomeContainer.  This will contain
	 * just one GnomeClientSite.
	 */
	bw->container = gnome_container_new ();

	bw->server = gnome_bonobo_widget_launch_component (object_desc);

	/*
	 * Create the client site.  This is the container-side point
	 * of contact for the remote component.
	 */
	bw->client_site = gnome_client_site_new ();

	/*
	 * Bind the local ClientSite object to the remote Embeddable
	 * component.
	 */
	gnome_client_site_bind_embeddable (bw->client_site, bw->server);

	/*
	 * Add the client site to the container.  This container is
	 * basically just there as a place holder; this is the only
	 * client site that will ever be added to it.
	 */
	gnome_container_add (bw->container, bw->client_site);

	/*
	 * Now create a new view for the remote object.
	 */
	bw->view_frame = gnome_client_site_new_view (bw->client_site);
}

GnomeBonoboWidget *
gnome_bonobo_widget_new (char *object_desc,
			 GnomeUIHandler *uih)
{
	GnomeBonoboWidget *bw;

	g_return_val_if_fail (object_desc != NULL, NULL);

	bw = gtk_type_new (GNOME_BONOBO_WIDGET_TYPE);

	gnome_bonobo_widget_create_object (bw, object_desc);

	if (bw->view_frame == NULL)
		return NULL;

	gnome_container_add (GTK_CONTAINER (
	return gnome_view_frame_get_wrapper (bw->view_frame);
}

static void
gnome_bonobo_widget_class_init (GnomeBonoboWidgetClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (GTK_TYPE_BIN);

	object_class->destroy = gnome_bonobo_widget_destroy;
}

static void
gnome_bonobo_widget_init (GnomeBonoboWidget *bw)
{
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

		type = gtk_type_unique (GNOME_WRAPPER_TYPE, &info);
	}

	return type;
}
