/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME BonoboWidget object.
 *
 * Bonobo component embedding for hydrocephalic imbeciles.
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

#define CHILD_SPACING     1
#define DEFAULT_LEFT_POS  4
#define DEFAULT_TOP_POS   4
#define DEFAULT_SPACING   7

static GnomeWrapperClass *gnome_bonobo_widget_parent_class;

static GnomeObjectClient *
gnome_bonobo_widget_launch_component (char *object_desc)
{
	GnomeObjectClient *server;

	server = gnome_object_activate (object_desc, (GoadActivationFlags) 0);

	return server;
}

static void
gnome_bonobo_widget_create_object (GnomeBonoboWidget *bw, char *object_desc)
{
	GtkWidget *view_widget;

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
	bw->client_site = gnome_client_site_new (bw->container);

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
	gnome_container_add (bw->container, GNOME_OBJECT (bw->client_site));

	/*
	 * Now create a new view for the remote object.
	 */
	bw->view_frame = gnome_client_site_new_view (bw->client_site);

	/*
	 * Add the view frame.
	 */
	view_widget = gnome_view_frame_get_wrapper (bw->view_frame);
	gtk_container_add (GTK_CONTAINER (bw), view_widget);
	gtk_widget_show (view_widget);
}

GnomeBonoboWidget *
gnome_bonobo_widget_new (char *object_desc,
			 GnomeUIHandler *uih)
{
	GnomeBonoboWidget *bw;
	GtkWidget *view_widget;
	int width, height;

	g_return_val_if_fail (object_desc != NULL, NULL);

	bw = gtk_type_new (GNOME_BONOBO_WIDGET_TYPE);

	gnome_bonobo_widget_create_object (bw, object_desc);

	gtk_widget_show (GTK_WIDGET (bw));

	gnome_view_frame_size_request (bw->view_frame, &width, &height);

	view_widget = gnome_view_frame_get_wrapper (bw->view_frame);

	gtk_widget_set_usize (view_widget, width, height);

	return bw;
}

static void
gnome_bonobo_widget_destroy (GtkObject *object)
{
}

static void
gnome_bonobo_widget_realize (GtkWidget *widget)
{
	GnomeBonoboWidget *bw;
	GdkWindowAttr attributes;
	gint attributes_mask;
	gint border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_WIDGET (widget));

	bw = GNOME_BONOBO_WIDGET (widget);
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	border_width = GTK_CONTAINER (widget)->border_width;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x + border_width;
	attributes.y = widget->allocation.y + border_width;
	attributes.width = widget->allocation.width - border_width * 2;
	attributes.height = widget->allocation.height - border_width * 2;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_EXPOSURE_MASK |
				  GDK_BUTTON_PRESS_MASK |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_ENTER_NOTIFY_MASK |
				  GDK_LEAVE_NOTIFY_MASK);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, bw);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
gnome_bonobo_widget_size_request (GtkWidget *widget,
				  GtkRequisition *requisition)
{
	GnomeBonoboWidget *bw;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_WIDGET (widget));
	g_return_if_fail (requisition != NULL);

	bw = GNOME_BONOBO_WIDGET (widget);

	requisition->width = (GTK_CONTAINER (widget)->border_width + CHILD_SPACING +
			      GTK_WIDGET (widget)->style->klass->xthickness) * 2;
	requisition->height = (GTK_CONTAINER (widget)->border_width + CHILD_SPACING +
			       GTK_WIDGET (widget)->style->klass->ythickness) * 2;

	if (GTK_WIDGET_CAN_DEFAULT (widget))
	{
		requisition->width += (GTK_WIDGET (widget)->style->klass->xthickness * 2 +
				       DEFAULT_SPACING);
		requisition->height += (GTK_WIDGET (widget)->style->klass->ythickness * 2 +
					DEFAULT_SPACING);
	}

	if (GTK_BIN (bw)->child && GTK_WIDGET_VISIBLE (GTK_BIN (bw)->child))
	{
		GtkRequisition child_requisition;

		gtk_widget_size_request (GTK_BIN (bw)->child, &child_requisition);

		requisition->width += child_requisition.width;
		requisition->height += child_requisition.height;
	}
}

static void
gnome_bonobo_widget_size_allocate (GtkWidget *widget,
				   GtkAllocation *allocation)
{
	GnomeBonoboWidget *bw;
	GtkAllocation child_allocation;
	gint border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_WIDGET (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	border_width = GTK_CONTAINER (widget)->border_width;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move_resize (widget->window,
					widget->allocation.x + border_width,
					widget->allocation.y + border_width,
					widget->allocation.width - border_width * 2,
					widget->allocation.height - border_width * 2);

	bw = GNOME_BONOBO_WIDGET (widget);

	if (GTK_BIN (bw)->child && GTK_WIDGET_VISIBLE (GTK_BIN (bw)->child))
	{
		child_allocation.x = (CHILD_SPACING + GTK_WIDGET (widget)->style->klass->xthickness);
		child_allocation.y = (CHILD_SPACING + GTK_WIDGET (widget)->style->klass->ythickness);

		child_allocation.width = MAX (1, (gint)widget->allocation.width - child_allocation.x * 2 -
					      border_width * 2);
		child_allocation.height = MAX (1, (gint)widget->allocation.height - child_allocation.y * 2 -
					       border_width * 2);

		if (GTK_WIDGET_CAN_DEFAULT (bw))
		{
			child_allocation.x += (GTK_WIDGET (widget)->style->klass->xthickness +
					       DEFAULT_LEFT_POS);
			child_allocation.y += (GTK_WIDGET (widget)->style->klass->ythickness +
					       DEFAULT_TOP_POS);
			child_allocation.width =  MAX (1, (gint)child_allocation.width -
			       (gint)(GTK_WIDGET (widget)->style->klass->xthickness * 2 + DEFAULT_SPACING));
			child_allocation.height = MAX (1, (gint)child_allocation.height -
			       (gint)(GTK_WIDGET (widget)->style->klass->xthickness * 2 + DEFAULT_SPACING));
		}

		gtk_widget_size_allocate (GTK_BIN (bw)->child, &child_allocation);
	}
}

static void
gnome_bonobo_widget_class_init (GnomeBonoboWidgetClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

	gnome_bonobo_widget_parent_class = gtk_type_class (GTK_TYPE_BIN);

/*	widget_class->realize = gnome_bonobo_widget_realize; */
	widget_class->size_request = gnome_bonobo_widget_size_request;
	widget_class->size_allocate = gnome_bonobo_widget_size_allocate;

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

		type = gtk_type_unique (gtk_bin_get_type (), &info);
	}

	return type;
}
