/* Bonobo-Plug.c: A private version of GtkPlug
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

#include "gdk/gdkx.h"
#include "gdk/gdkkeysyms.h"
#include "bonobo/bonobo-plug.h"

static GtkPlugClass *parent_class = NULL;

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

static void
bonobo_plug_init (BonoboPlug *plug)
{
}

static void
bonobo_plug_class_init (BonoboPlugClass *klass)
{
	parent_class = gtk_type_class (GTK_TYPE_PLUG);
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

