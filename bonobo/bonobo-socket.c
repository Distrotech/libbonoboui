/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* By Owen Taylor <otaylor@gtk.org>              98/4/4 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#undef SOCKET_DEBUG

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

/* Forward declararations */

static void bonobo_socket_class_init               (BonoboSocketClass    *klass);
static void bonobo_socket_init                     (BonoboSocket         *socket);
static void bonobo_socket_destroy                  (GtkObject            *object);
static void bonobo_socket_realize                  (GtkWidget            *widget);
static void bonobo_socket_unrealize                (GtkWidget            *widget);

/* Local data */

static GtkSocketClass *parent_class = NULL;

guint
bonobo_socket_get_type ()
{
	static guint socket_type = 0;

	if (!socket_type)
	{
		static const GtkTypeInfo socket_info =
		{
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

static void
bonobo_socket_class_init (BonoboSocketClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	parent_class = gtk_type_class (GTK_TYPE_SOCKET);

	object_class->destroy = bonobo_socket_destroy;

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

/* Destroy handler for the socket */
static void
bonobo_socket_destroy (GtkObject *object)
{
	BonoboSocket *socket;
	BonoboSocketPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BONOBO_IS_SOCKET (object));

	socket = BONOBO_SOCKET (object);
	priv = socket->priv;

	g_free (priv);
	socket->priv = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

void
bonobo_socket_set_control_frame (BonoboSocket       *socket,
				 BonoboControlFrame *frame)
{
	g_return_if_fail (BONOBO_IS_SOCKET (socket));

	if (socket->priv)
		socket->priv->frame = frame;
}

