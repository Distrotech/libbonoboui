/*
 * test-focus.c: A test application to sort focus issues.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Ximian, Inc.
 */

#include "config.h"

#include <stdlib.h>
#include <libbonoboui.h>
#include <bonobo/bonobo-i18n.h>

int
main (int argc, char **argv)
{
/*	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *tmp, *control;*/
	CORBA_ORB  orb;

	free (malloc (8));

	textdomain (GETTEXT_PACKAGE);

	if (!bonobo_ui_init ("test-focus", VERSION, &argc, argv))
		g_error (_("Can not bonobo_ui_init"));

	orb = bonobo_orb ();

	bonobo_activate ();

/*	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Focus test");

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	tmp = gtk_entry_new ();
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	tmp = gtk_button_new_with_label ("In Container A");
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	control = bonobo_widget_new_control ("OAFIID:Bonobo_Sample_Entry", NULL);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), control);

	tmp = gtk_button_new_with_label ("Destroy remote control");
	gtk_box_pack_start_defaults (GTK_BOX (vbox), tmp);

	gtk_widget_show_all (window);

	gtk_main ();*/

	/* FIXME: we will iterate the gtk mainloop and show windows,
	   but we will be fully automated */

	return bonobo_shutdown ();
}
