/*
 * sample-control-container.c
 * 
 * Authors:
 *   Nat Friedman  (nat@helixcode.com)
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#include <config.h>
#include <gnome.h>

#if USING_OAF
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif

#include <bonobo.h>

BonoboPropertyBagClient *pbc = NULL;

static void
populate_property_list (GtkWidget *bw, GtkCList *clist)
{
	GList *property_list, *l;

	property_list = bonobo_property_bag_client_get_property_names (pbc);
	for (l = property_list; l != NULL; l = l->next) {
		char *row_array[2];
		CORBA_TypeCode tc;
		gchar *name = l->data;

		row_array [0] = name;

		tc = bonobo_property_bag_client_get_property_type (pbc, name);
		switch (tc->kind) {

		case CORBA_tk_boolean:
			row_array [1] = g_strdup (
				bonobo_property_bag_client_get_value_gboolean (pbc, name) ? "TRUE" : "FALSE");
			break;

		case CORBA_tk_string:
			row_array [1] = g_strdup (bonobo_property_bag_client_get_value_string (pbc, name));
			break;

		case CORBA_tk_long:
			row_array [1] = g_strdup_printf ("%ld", bonobo_property_bag_client_get_value_glong (pbc, name));
			break;

		case CORBA_tk_float:
			row_array [1] = g_strdup_printf ("%f", bonobo_property_bag_client_get_value_gfloat (pbc, name));
			break;

		case CORBA_tk_double:
			row_array [1] = g_strdup_printf ("%g", bonobo_property_bag_client_get_value_gdouble (pbc, name));
			break;

		default:
			row_array [1] = g_strdup ("Unhandled Property Type");
			break;
		}

		gtk_clist_append (clist, row_array);
	}
	g_list_free (property_list);
}

static void
edit_property (GtkCList *clist, GdkEventButton *event, BonoboWidget *bw)
{
	gchar *prop;
	gint row, col;
	GList *l;
	CORBA_TypeCode tc;

	if (event->button == 3) {
		gtk_clist_get_selection_info (clist, event->x, event->y,
		                              &row, &col);
		if (row < 0) return;
		l = bonobo_property_bag_client_get_property_names (pbc);
		if (row > g_list_length (l) - 1) return;

		/* Get the value of the property they clicked on. */
		prop = g_list_nth_data (l, row);
		/* Change it appropriately. */
		tc = bonobo_property_bag_client_get_property_type (pbc, prop);

		switch (tc->kind) {

		case CORBA_tk_boolean:
			bonobo_property_bag_client_set_value_gboolean (
				pbc, prop, !bonobo_property_bag_client_get_value_gboolean (pbc, prop));
			break;

		default:
			g_warning ("Cannot set_value this type of property yet, sorry.");
			break;
			
		}

		g_list_free (l);

		/* Redraw the property list. */
		gtk_clist_clear (clist);
		populate_property_list (GTK_WIDGET (bw), clist);
	}

}

static GtkWidget *
create_proplist (GtkWidget *bw)
{
	gchar *clist_titles[] = {"Property Name", "Value"};
	GtkWidget *clist;

	/* Put the property CList on the bottom. */
	clist = gtk_clist_new_with_titles (2, clist_titles);
	gtk_signal_connect (GTK_OBJECT (clist), "button_press_event",
		GTK_SIGNAL_FUNC (edit_property), bw);
 
	populate_property_list (bw, GTK_CLIST (clist));

	return clist;
}

static void
incr_calc (GtkButton *button, BonoboWidget *control)
{
	CORBA_double i;

	bonobo_widget_get_property (control, "value", &i, NULL);
	i+= 0.37;
	bonobo_widget_set_property (control, "value", i, NULL);
}

static void
app_destroy_cb (GtkWidget *app, BonoboUIHandler *uih)
{
	bonobo_object_unref (BONOBO_OBJECT (uih));
	if (pbc)
		bonobo_object_unref (BONOBO_OBJECT (pbc));
	pbc = NULL;

	gtk_main_quit ();
}

static int
app_delete_cb (GtkWidget *widget, GdkEvent *event, gpointer dummy)
{
	gtk_widget_destroy (GTK_WIDGET (widget));
	return FALSE;
}

static guint
container_create (void)
{
	GtkWidget       *app;
	GtkWidget       *control;
	GtkWidget       *proplist;
	GtkWidget       *box;
	GtkWidget       *button;
	BonoboUIHandler *uih;
	BonoboControlFrame *cf;

	app = gnome_app_new ("sample-control-container",
			     "Sample Bonobo Control Container");
	gtk_window_set_default_size (GTK_WINDOW (app), 500, 440);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	uih = bonobo_ui_handler_new ();

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    GTK_SIGNAL_FUNC (app_delete_cb), NULL);

	gtk_signal_connect (GTK_OBJECT (app), "destroy",
			    GTK_SIGNAL_FUNC (app_destroy_cb), uih);

	bonobo_ui_handler_set_app (uih, GNOME_APP (app));
	bonobo_ui_handler_create_menubar (uih);

	box = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), box);

#if USING_OAF
	control = bonobo_widget_new_control (
		"OAFIID:bonobo_calculator:fab8c2a7-9576-437c-aa3a-a8617408970f",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#else
	control = bonobo_widget_new_control (
  	        "control:calculator",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#endif

	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Increment result");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc)incr_calc, control);

#if USING_OAF
	control = bonobo_widget_new_control (
		"OAFIID:bonobo_clock:d42cc651-44ae-4f69-a10d-a0b6b2cc6ecc",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#else
	control = bonobo_widget_new_control (
		"control:clock",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#endif

	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);

	cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (control));
	pbc = bonobo_control_frame_get_control_property_bag (cf);

	proplist = create_proplist (control);

#if USING_OAF
	control = bonobo_widget_new_control (
		"OAFIID:bonobo_entry_factory:ef3e3c33-43e2-4f7c-9ca9-9479104608d6",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#else
	control = bonobo_widget_new_control (
		"control:entry",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
#endif

	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (box), proplist, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	gtk_widget_show_all (app);

	return FALSE;
}

int
main (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_ORB orb;
	CORBA_exception_init (&ev);

	/* Encorage -lefence to play ball */
	{ char *tmp = malloc (4); if (tmp) free (tmp); }

#if USING_OAF
        gnome_init_with_popt_table("sample-control-container", "0.0",
				   argc, argv,
				   oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);
#else
	gnome_CORBA_init_with_popt_table (
      	"sample-control-container", "0.0",
	&argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);
	orb = gnome_CORBA_ORB ();
#endif

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error ("Could not initialize Bonobo");

	/*
	 * We can't make any CORBA calls unless we're in the main
	 * loop.  So we delay creating the container here.
	 */
	gtk_idle_add ((GtkFunction) container_create, NULL);

	bonobo_main ();

	return 0;
}
