/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <gtk/gtk.h>
#include <bonobo/gnome-object.h>
#include <libgnorba/gnorba.h>

CORBA_Environment ev;
CORBA_ORB orb;

void panel_callback (GtkWidget *widget, gpointer data);
void noact_callback (GtkWidget *widget, gpointer data);
void quit_callback (GtkWidget *widget, gpointer data);

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button;

	CORBA_exception_init (&ev);
	gnome_CORBA_init ("BonoboSel Test", "1.0", &argc, argv, 0, &ev);
	orb = gnome_CORBA_ORB ();

	window = gnome_app_new ("selector_test", "Bonobo Selection Test");
	gtk_signal_connect (GTK_OBJECT (window), "delete_event", 
		GTK_SIGNAL_FUNC (quit_callback), NULL);
	
	vbox = gtk_vbox_new (TRUE, 0);
	gnome_app_set_contents (GNOME_APP (window), vbox);
	
	button = gtk_button_new_with_label ("Get goad id");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (noact_callback), NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 5);

	button = gtk_button_new_with_label ("Get goad id of panel applet");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (panel_callback), NULL);
	gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 5);
	
	gtk_widget_show_all (window);

	gtk_main ();
	
	return 0;
}

void panel_callback (GtkWidget *widget, gpointer data)
{
/* it filters! */
	const gchar *ints [] = { "IDL:GNOME/Applet:1.0", NULL };
	gchar *text;

	text = gnome_bonobo_select_goad_id (_("Select an object"), ints);

	g_print("%s\n", text);
	if (text != NULL)
		g_free(text);
}

void noact_callback (GtkWidget *widget, gpointer data)
{
	/* This is also a demonstration of default being what we just did above */ 
	gchar *text;

	text = gnome_bonobo_select_goad_id (_("Select an object"), NULL);
	g_print("%s\n", text);
	if (text != NULL)
		g_free(text);
}

void quit_callback (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}


