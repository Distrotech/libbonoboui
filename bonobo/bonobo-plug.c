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
	g_return_if_fail (BONOBO_IS_PLUG (plug));

	if (plug->control == control)
		return;

	if (plug->control) {
		bonobo_control_set_plug (plug->control, NULL);
		g_object_unref (G_OBJECT (plug->control));
	}

	if (control)
		plug->control = g_object_ref (G_OBJECT (control));
}

static void
bonobo_plug_dispose (GObject *object)
{
	BonoboPlug *plug = (BonoboPlug *) object;

	if (plug->control)
		bonobo_plug_set_control (plug, NULL);

	parent_class->dispose (object);
}

static void
bonobo_plug_init (BonoboPlug *plug)
{
}

static void
bonobo_plug_class_init (GObjectClass *klass)
{
	parent_class = gtk_type_class (GTK_TYPE_PLUG);

	klass->dispose = bonobo_plug_dispose;
}

GtkType
bonobo_plug_get_type ()
{
	static guint plug_type = 0;

	if (!plug_type)
	{
		static const GtkTypeInfo plug_info =
		{
			"BonoboPlug",
			sizeof (BonoboPlug),
			sizeof (BonoboPlugClass),
			(GtkClassInitFunc) bonobo_plug_class_init,
			(GtkObjectInitFunc) bonobo_plug_init,
			NULL,
			NULL
		};

		plug_type = gtk_type_unique (gtk_plug_get_type (), &plug_info);
	}

	return plug_type;
}
