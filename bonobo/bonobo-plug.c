/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-plug.c: a Gtk plug wrapper.
 *
 * Author:
 *   Martin Baulig     (martin@home-of-linux.org)
 *   Michael Meeks     (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 *                 Martin Baulig.
 */

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <bonobo/bonobo-plug.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-control-internal.h>

static GObjectClass *parent_class = NULL;

/**
 * bonobo_plug_construct:
 * @plug: The #BonoboPlug.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the creation of a #BonoboPlug widget. This function
 * will generally only be used by classes deriving
 * from #BonoboPlug.
 */
void
bonobo_plug_construct (BonoboPlug *plug, guint32 socket_id)
{
	gtk_plug_construct (GTK_PLUG (plug), socket_id);
}

/**
 * bonobo_plug_new:
 * @socket_id: the XID of the socket's window.
 *
 * Create a new plug widget inside the #GtkSocket identified
 * by @socket_id.
 *
 * Returns: the new #BonoboPlug widget.
 */
GtkWidget*
bonobo_plug_new (guint32 socket_id)
{
	BonoboPlug *plug;

	plug = BONOBO_PLUG (g_object_new (bonobo_plug_get_type (), NULL));

	bonobo_plug_construct (plug, socket_id);

	dprintf ("bonobo_plug_new => %p\n", plug);

	return GTK_WIDGET (plug);
}

BonoboControl *
bonobo_plug_get_control (BonoboPlug *plug)
{
	g_return_val_if_fail (BONOBO_IS_PLUG (plug), NULL);

	return plug->control;
}

void
bonobo_plug_set_control (BonoboPlug    *plug,
			 BonoboControl *control)
{
	BonoboControl *old_control;

	g_return_if_fail (BONOBO_IS_PLUG (plug));

	if (plug->control == control)
		return;

	old_control = plug->control;

	if (control) {
		plug->control = g_object_ref (G_OBJECT (control));
		bonobo_control_set_plug (control, plug);
	} else
		plug->control = NULL;

	if (old_control) {
		bonobo_control_set_plug (old_control, NULL);
		g_object_unref (G_OBJECT (old_control));
	}

}

static gboolean
bonobo_plug_delete_event (GtkWidget   *widget,
			  GdkEventAny *event)
{
	dprintf ("bonobo_plug_delete_event %p\n", widget);

	return FALSE;
}

static void
bonobo_plug_realize (GtkWidget *widget)
{
	dprintf ("bonobo_plug_realize %p\n", widget);

	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

static void
bonobo_plug_dispose (GObject *object)
{
	BonoboPlug *plug = (BonoboPlug *) object;
	GtkBin *bin_plug = (GtkBin *) object;

	dprintf ("bonobo_plug_dispose %p\n", object);

	if (bin_plug->child) {
		g_object_ref (G_OBJECT (bin_plug->child));
		gtk_container_remove (
			&bin_plug->container, bin_plug->child);
		g_warning ("Removing child ...");
	}

	if (plug->control) {
		BonoboControl *control = plug->control;
		gboolean       inproc_parent_died = FALSE;

		if (BONOBO_IS_SOCKET (GTK_WIDGET (plug)->parent))
			inproc_parent_died = bonobo_socket_disposed (
				BONOBO_SOCKET (GTK_WIDGET (plug)->parent));
		else
			inproc_parent_died = FALSE;

		bonobo_plug_set_control (plug, NULL);

		bonobo_control_notify_plug_died (
			control, inproc_parent_died);
	}

	parent_class->dispose (object);
}

static void
bonobo_plug_size_allocate (GtkWidget     *widget,
			   GtkAllocation *allocation)
{
	dprintf ("bonobo_plug_size_allocate: (%d, %d), (%d, %d) %d! %s\n",
		 allocation->x, allocation->y,
		 allocation->width, allocation->height,
		 GTK_WIDGET_TOPLEVEL (widget),
		 GTK_BIN (widget)->child ?
		 g_type_name_from_instance ((gpointer)GTK_BIN (widget)->child):
		 "No child!");

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
}

static void
bonobo_plug_size_request (GtkWidget      *widget,
			  GtkRequisition *requisition)
{
	dprintf ("bonobo_plug_size_request: %d, %d\n",
		 requisition->width, requisition->height);

	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}

static gboolean
bonobo_plug_expose_event (GtkWidget      *widget,
			  GdkEventExpose *event)
{
	gboolean retval;

	retval = GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

	dprintf ("bonobo_plug_expose_event %p (%d, %d), (%d, %d)\n",
		 widget,
		 event->area.x, event->area.y,
		 event->area.width, event->area.height);

#ifdef DEBUG_CONTROL
	gdk_draw_line (widget->window,
		       widget->style->black_gc,
		       event->area.x + event->area.width,
		       event->area.y,
		       event->area.x, 
		       event->area.y + event->area.height);
#endif

	return retval;
}

static void
bonobo_plug_init (BonoboPlug *plug)
{
}

static void
bonobo_plug_class_init (GObjectClass *klass)
{
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	klass->dispose = bonobo_plug_dispose;

	widget_class->realize = bonobo_plug_realize;
	widget_class->delete_event  = bonobo_plug_delete_event;
	widget_class->size_request  = bonobo_plug_size_request;
	widget_class->size_allocate = bonobo_plug_size_allocate;
	widget_class->expose_event  = bonobo_plug_expose_event;
}

GtkType
bonobo_plug_get_type ()
{
	static guint plug_type = 0;

	if (!plug_type) {
		static const GtkTypeInfo plug_info = {
			"BonoboPlug",
			sizeof (BonoboPlug),
			sizeof (BonoboPlugClass),
			(GtkClassInitFunc) bonobo_plug_class_init,
			(GtkObjectInitFunc) bonobo_plug_init,
			NULL,
			NULL
		};

		plug_type = gtk_type_unique (
			gtk_plug_get_type (), &plug_info);
	}

	return plug_type;
}
