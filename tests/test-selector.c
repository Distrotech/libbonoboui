/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <libgnome.h>
#include <gtk/gtk.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-selector.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-ui-main.h>
#include <bonobo/bonobo-win.h>

#include <liboaf/liboaf.h>

CORBA_Environment ev;
CORBA_ORB orb;

static void
noact_callback (GtkWidget *widget, gpointer data)
{
	gchar *text;

	text = bonobo_selector_select_id (_("Select an object"), NULL);
	g_print ("%s\n", text);

	g_free (text);
}

static void
quit_callback (GtkWidget *widget, gpointer data)
{
	bonobo_main_quit ();
}

static void
panel_callback (GtkWidget *widget, gpointer data)
{
/* it filters! */
	g_warning ("You can't get an id of a panel applet since the panel"
		   "is using GOAD at the moment");
}

int
main (int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button;

	CORBA_exception_init (&ev);

/*        gnome_program_init ("BonoboSel Test", "1.0", &libgnome_module_info,
	  argc, argv, NULL);*/

	bonobo_ui_init ("BonoboSelector Test", VERSION, &argc, argv);

	orb = bonobo_orb ();

	window = bonobo_window_new ("selector_test", "Bonobo Selection Test");
	gtk_signal_connect (GTK_OBJECT (window), "delete_event", 
		GTK_SIGNAL_FUNC (quit_callback), NULL);
	
	vbox = gtk_vbox_new (TRUE, 0);
	bonobo_window_set_contents (BONOBO_WINDOW (window), vbox);
	
	button = gtk_button_new_with_label ("Get id");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (noact_callback), NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 5);

	button = gtk_button_new_with_label ("Get id of panel applet");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (panel_callback), NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 5);
	
	gtk_widget_show_all (window);

	bonobo_main ();
	
	return 0;
}
