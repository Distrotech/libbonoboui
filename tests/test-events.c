/*
 * test-events.c: A test application to sort X events issues.
 *
 * Author:
 *	Mark McLoughlin <mark@skynet.ie>
 *
 * Copyright 2001 Sun Microsytems, Inc.
 */

#include "config.h"
#include <string.h>

#include <libbonoboui.h>

static gboolean
event_cb (GtkWidget *widget, GdkEventButton *event)
{
	switch (event->type) {
	case GDK_BUTTON_PRESS:
		g_message (_("button press event %d"), event->button);
		break;
	case GDK_BUTTON_RELEASE:
		g_message (_("button release event %d"), event->button);
		break;
	default:
		g_message (_("event type: %d"), event->type);
		break;
	}

	return FALSE;
}

static int
exit_cb (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
	return FALSE;
}

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *control;
	gchar     *iid;

	if (argc != 2 || strncmp (argv [1], "OAFIID", 6))
		g_error (_("usage: test-events <oaf-iid>"));

	iid = argv [1];

	textdomain (GETTEXT_PACKAGE);

	if (!bonobo_ui_init ("test-events", VERSION, &argc, argv))
		g_error (_("Can not bonobo_ui_init"));

	bonobo_activate ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Events test");

	g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (exit_cb), NULL);
	g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (event_cb), NULL);
	g_signal_connect (G_OBJECT (window), "button-release-event", G_CALLBACK (event_cb), NULL);

	control = bonobo_widget_new_control (iid, NULL);

	gtk_container_add (GTK_CONTAINER (window), control);

	gtk_widget_show_all (window);

	gtk_main ();

	return bonobo_debug_shutdown ();
}