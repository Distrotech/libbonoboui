/*
 * Test to check if removing controls is borked
 *
 * Author:  Iain <iain@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 */

#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

GtkWidget *window, *vbox, *button, *placeholder;
GtkWidget *bonobo_widget = NULL;

#define TEST_OAFIID "OAFIID:Bonobo_Sample_Mines"

static void
remove_and_add (GtkWidget *click, gpointer useless_user_data)
{
	gtk_object_ref (GTK_OBJECT (bonobo_widget));
	gtk_container_remove (GTK_CONTAINER (placeholder), bonobo_widget);
	g_print ("Removed............\n");
	gtk_container_add (GTK_CONTAINER (placeholder), bonobo_widget);
	g_print ("Added..............\n");
	gtk_widget_show (bonobo_widget);
}
			
static int
make_bonobo_widget (gpointer data)
{
	if (bonobo_widget != NULL)
		return FALSE;

	bonobo_widget = bonobo_widget_new_control (TEST_OAFIID, CORBA_OBJECT_NIL);
	gtk_widget_show (bonobo_widget);
	gtk_container_add (GTK_CONTAINER (placeholder), bonobo_widget);

	return FALSE;
}

int
main (int argc, char **argv)
{
	CORBA_ORB orb;

	gnome_init_with_popt_table ("Fun test", "1.0", argc, argv, 
				    oaf_popt_options, 0, NULL);
	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL) == FALSE) {
		g_error ("Bigger problems than controls not working");
		exit (0);
	}

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	placeholder = gtk_frame_new ("Place holder");
	gtk_box_pack_start (GTK_BOX (vbox), placeholder, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Remove and add");
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (remove_and_add), NULL);
  
	gtk_widget_show_all (window);
	g_idle_add (make_bonobo_widget, NULL);
	bonobo_main ();
  
	return 0;
}
