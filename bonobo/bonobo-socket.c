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
#include <bonobo/bonobo-control-frame.h>

struct _BonoboSocketPrivate {
	/* The control on the other side which we use to gdk_flush() */
	BonoboControlFrame *frame;
};

/* Local data */

static GtkSocketClass *parent_class = NULL;

/* Destroy handler for the socket */
static void
bonobo_socket_finalize (GObject *object)
{
	BonoboSocket *socket;
	BonoboSocketPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BONOBO_IS_SOCKET (object));

	socket = BONOBO_SOCKET (object);
	priv = socket->priv;

	g_free (priv);
	socket->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
bonobo_socket_realize (GtkWidget *widget)
{
	BonoboSocket *socket;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_SOCKET (widget));

	socket = BONOBO_SOCKET (widget);

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	bonobo_control_frame_sync_realize (socket->priv->frame);
}

static void
bonobo_socket_unrealize (GtkWidget *widget)
{
	BonoboSocket *socket;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_SOCKET (widget));

	socket = BONOBO_SOCKET (widget);

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);

	bonobo_control_frame_sync_unrealize (socket->priv->frame);
}

static void
bonobo_socket_class_init (GObjectClass *klass)
{
	GtkWidgetClass *widget_class;

	widget_class = (GtkWidgetClass*) klass;

	parent_class = gtk_type_class (GTK_TYPE_SOCKET);

	klass->finalize = bonobo_socket_finalize;

	widget_class->realize = bonobo_socket_realize;
	widget_class->unrealize = bonobo_socket_unrealize;
}

static void
bonobo_socket_init (BonoboSocket *socket)
{
	BonoboSocketPrivate *priv;

	priv = g_new (BonoboSocketPrivate, 1);
	socket->priv = priv;
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
			(GtkObjectInitFunc) bonobo_socket_init,
			NULL,
			NULL
		};

		socket_type = gtk_type_unique (gtk_socket_get_type (), &socket_info);
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

void
bonobo_socket_set_control_frame (BonoboSocket       *socket,
				 BonoboControlFrame *frame)
{
	g_return_if_fail (BONOBO_IS_SOCKET (socket));

	if (socket->priv)
		socket->priv->frame = frame;
}

