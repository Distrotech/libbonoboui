/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-socket.c: a Gtk+ socket wrapper
 *
 * Authors:
 *   Martin Baulig     (martin@home-of-linux.org)
 *   Michael Meeks     (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 *                 Martin Baulig.
 */
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkdnd.h>
#include <bonobo/bonobo-socket.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-control-frame.h>
#include <bonobo/bonobo-control-internal.h>

static GtkSocketClass *parent_class = NULL;

static void
bonobo_socket_finalize (GObject *object)
{
	dprintf ("bonobo_socket_finalize %p\n", object);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bonobo_socket_dispose (GObject *object)
{
	BonoboSocket *socket = (BonoboSocket *) object;

	dprintf ("bonobo_socket_dispose %p\n", object);

	if (socket->frame) {
		BonoboObject *object = BONOBO_OBJECT (socket->frame);

		bonobo_socket_set_control_frame (socket, NULL);
		bonobo_object_unref (object);
		g_assert (socket->frame == NULL);
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bonobo_socket_realize (GtkWidget *widget)
{
	BonoboSocket *socket;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_SOCKET (widget));

	socket = BONOBO_SOCKET (widget);

	dprintf ("bonobo_socket_realize %p\n", widget);

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	if (socket->frame) {
		g_object_ref (G_OBJECT (socket->frame));
		bonobo_control_frame_get_remote_window (socket->frame, NULL);
		g_object_unref (G_OBJECT (socket->frame));
	}

	g_assert (GTK_WIDGET_REALIZED (widget));
}

static void
bonobo_socket_unrealize (GtkWidget *widget)
{
	dprintf ("unrealize %p\n", widget);

	g_assert (GTK_WIDGET_REALIZED (widget));
	g_assert (GTK_WIDGET (widget)->window);

	/* To stop evilness inside Gtk+ */
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gboolean
bonobo_socket_expose_event (GtkWidget      *widget,
			    GdkEventExpose *event)
{
	dprintf ("bonobo_socket_expose_event %p (%d, %d), (%d, %d)\n",
		 widget,
		 event->area.x, event->area.y,
		 event->area.width, event->area.height);

#ifdef DEBUG_CONTROL
	gdk_draw_line (widget->window,
		       widget->style->black_gc,
		       event->area.x, event->area.y,
		       event->area.x + event->area.width,
		       event->area.y + event->area.height);
#endif

	return FALSE;
}

static void
bonobo_socket_state_changed (GtkWidget   *widget,
			     GtkStateType previous_state)
{
	BonoboSocket *socket = BONOBO_SOCKET (widget);

	if (!socket->frame)
		return;

	if (!bonobo_control_frame_get_autostate (socket->frame))
		return;

	bonobo_control_frame_control_set_state (
		socket->frame, GTK_WIDGET_STATE (widget));
}

static gint
bonobo_socket_focus_in (GtkWidget     *widget,
			GdkEventFocus *focus)
{
	BonoboSocket *socket = BONOBO_SOCKET (widget);

	if (socket->frame &&
	    bonobo_control_frame_get_autoactivate (socket->frame))
		bonobo_control_frame_control_activate (socket->frame);
	else
		dprintf ("No activate on focus in");

	return GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, focus);
}

static gint
bonobo_socket_focus_out (GtkWidget     *widget,
			 GdkEventFocus *focus)
{
	BonoboSocket *socket = BONOBO_SOCKET (widget);

	if (socket->frame &&
	    bonobo_control_frame_get_autoactivate (socket->frame))
		bonobo_control_frame_control_deactivate (socket->frame);
	else
		dprintf ("No de-activate on focus out");

	return GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, focus);
}

static void
bonobo_socket_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
	dprintf ("bonobo_socket_size_allocate: (%d, %d), (%d, %d)\n",
		 allocation->x, allocation->y,
		 allocation->width, allocation->height);
	
	return GTK_WIDGET_CLASS (parent_class)->size_allocate (
		widget, allocation);
}

static void
bonobo_socket_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
	BonoboSocket *socket = (BonoboSocket *) widget;
	GtkSocket    *gtk_socket = (GtkSocket *) widget;

	if (GTK_WIDGET_REALIZED (widget) ||
	    !socket->frame ||
	    (gtk_socket->is_mapped && gtk_socket->have_size))

		GTK_WIDGET_CLASS (parent_class)->size_request (
			widget, requisition);

	else if (gtk_socket->have_size &&
		 GTK_WIDGET_VISIBLE (gtk_socket)) {

		requisition->width = gtk_socket->request_width;
		requisition->width = gtk_socket->request_height;

	} else {
		CORBA_Environment tmp_ev, *ev;

		CORBA_exception_init ((ev = &tmp_ev));

		bonobo_control_frame_size_request (
			socket->frame, requisition, ev);

		if (!BONOBO_EX (ev)) {
			gtk_socket->have_size = TRUE;
			gtk_socket->request_width = requisition->width;
			gtk_socket->request_height = requisition->width;
		}

		CORBA_exception_free (ev);
	}

	dprintf ("bonobo_socket_size_request: %d, %d\n",
		 requisition->width, requisition->height);
}

static gboolean
bonobo_socket_plug_removed (GtkSocket *socket)
{
	dprintf ("bonobo_socket_plug_removed %p\n", socket);

	return TRUE;
}

static void
bonobo_socket_class_init (GObjectClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkSocketClass *socket_class;

	widget_class = (GtkWidgetClass*) klass;
	socket_class = (GtkSocketClass*) klass;

	parent_class = g_type_class_peek_parent (klass);

	klass->finalize = bonobo_socket_finalize;
	klass->dispose  = bonobo_socket_dispose;

	widget_class->realize         = bonobo_socket_realize;
	widget_class->unrealize       = bonobo_socket_unrealize;
	widget_class->state_changed   = bonobo_socket_state_changed;
	widget_class->focus_in_event  = bonobo_socket_focus_in;
	widget_class->focus_out_event = bonobo_socket_focus_out;
	widget_class->size_request    = bonobo_socket_size_request;
	widget_class->size_allocate   = bonobo_socket_size_allocate;
	widget_class->expose_event    = bonobo_socket_expose_event;

	socket_class->plug_removed    = bonobo_socket_plug_removed;
}

guint
bonobo_socket_get_type ()
{
	static guint socket_type = 0;

	if (!socket_type) {
		static const GtkTypeInfo socket_info = {
			"BonoboSocket",
			sizeof (BonoboSocket),
			sizeof (BonoboSocketClass),
			(GtkClassInitFunc) bonobo_socket_class_init,
			(GtkObjectInitFunc) NULL,
			NULL,
			NULL
		};

		socket_type = gtk_type_unique (
			gtk_socket_get_type (), &socket_info);
	}

	return socket_type;
}

/**
 * bonobo_socket_new:
 *
 * Create a new empty #BonoboSocket.
 *
 * Returns: A new #BonoboSocket.
 */
GtkWidget*
bonobo_socket_new (void)
{
	BonoboSocket *socket;

	socket = gtk_type_new (bonobo_socket_get_type ());

	return GTK_WIDGET (socket);
}

BonoboControlFrame *
bonobo_socket_get_control_frame (BonoboSocket *socket)
{
	g_return_val_if_fail (BONOBO_IS_SOCKET (socket), NULL);

	return socket->frame;
}

void
bonobo_socket_set_control_frame (BonoboSocket       *socket,
				 BonoboControlFrame *frame)
{
	BonoboControlFrame *old_frame;

	g_return_if_fail (BONOBO_IS_SOCKET (socket));

	if (socket->frame == frame)
		return;

	old_frame = socket->frame;

	if (frame) {
		socket->frame = g_object_ref (G_OBJECT (frame));
		bonobo_control_frame_set_socket (frame, socket);
	} else
		socket->frame = NULL;

	if (old_frame) {
		bonobo_control_frame_set_socket (old_frame, NULL);
		g_object_unref (G_OBJECT (old_frame));
	}
}
