/*
 * Test to check if removing controls is borked
 *
 * Authors:
 *    Iain Holmes   <iain@ximian.com>
 *    Michael Meeks <michael@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 */

#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

GtkWidget *window, *vbox, *button, *placeholder1, *placeholder2;
GtkWidget *remote_widget = NULL, *inproc_widget = NULL;

#define TEST_OAFIID "OAFIID:Bonobo_Sample_Entry"

static void
remove_and_add (GtkWidget *click, gpointer useless_user_data)
{
	gtk_object_ref (GTK_OBJECT (remote_widget));
	gtk_container_remove (GTK_CONTAINER (placeholder1), remote_widget);
	gtk_object_ref (GTK_OBJECT (inproc_widget));
	gtk_container_remove (GTK_CONTAINER (placeholder2), inproc_widget);
	g_print ("Removed............\n");
	gtk_container_add (GTK_CONTAINER (placeholder1), remote_widget);
	gtk_container_add (GTK_CONTAINER (placeholder2), inproc_widget);
	g_print ("Added..............\n");
}

static void
make_remote_widget (void)
{
	g_assert (remote_widget == NULL);

	remote_widget = bonobo_widget_new_control (TEST_OAFIID, CORBA_OBJECT_NIL);
	gtk_widget_show (remote_widget);
	gtk_container_add (GTK_CONTAINER (placeholder1), remote_widget);
}

static void
make_inproc_widget (void)
{
	BonoboControl *control;
	GtkWidget     *entry;

	g_assert (inproc_widget == NULL);

	entry = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry), "In-proc");
	gtk_widget_show (entry);

	control = bonobo_control_new (entry);

	inproc_widget = bonobo_widget_new_control_from_objref (
		BONOBO_OBJREF (control), CORBA_OBJECT_NIL);

	gtk_widget_show (inproc_widget);
	gtk_container_add (GTK_CONTAINER (placeholder2), inproc_widget);
}

			
static gboolean
idle_init (gpointer data)
{
	make_remote_widget ();
	make_inproc_widget ();

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

	placeholder1 = gtk_frame_new ("Out of proc");
	gtk_box_pack_start (GTK_BOX (vbox), placeholder1, TRUE, TRUE, 0);

	placeholder2 = gtk_frame_new ("In proc");
	gtk_box_pack_start (GTK_BOX (vbox), placeholder2, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Remove and add");
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (remove_and_add), NULL);
  
	gtk_widget_show_all (window);
	g_timeout_add (0, idle_init, NULL);
	bonobo_main ();
  
	return 0;
}

