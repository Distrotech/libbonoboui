/*
 * sample-control-container.c
 *
 * 
 * Author:
 *   Nat Friedman (nat@nat.org)
 */
#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo.h>

BonoboPropertyBagClient *pbc;

#define CORBA_boolean__alloc() (CORBA_boolean *) CORBA_octet_allocbuf (sizeof (CORBA_boolean))

static void populate_property_list (GtkWidget *bw, GtkCList *clist);

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
			bonobo_property_bag_client_set_value_boolean (pbc, prop,
				!bonobo_property_bag_client_get_value_boolean (pbc, prop));
			break;
		default:
			g_warning ("Cannot set_value this type of property yet, sorry.");
			break;
			
		}

		g_list_free (l);
		/* Redraw the property list. */
		gtk_clist_clear (clist);
		populate_property_list (GTK_WIDGET(bw), clist);
	}

}


static void
populate_property_list (GtkWidget *bw, GtkCList *clist)
{
	BonoboControlFrame *cf;
	GList *property_list, *l;

	/* Get the list of properties. */
	if (pbc == NULL) {
		cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (bw));
		pbc = bonobo_control_frame_get_control_property_bag (cf);
	}

	property_list = bonobo_property_bag_client_get_property_names (pbc);
	for (l = property_list; l != NULL; l = l->next) {
		char *row_array[2];
		CORBA_TypeCode tc;
		gchar *name = l->data;

		row_array [0] = name;

		tc = bonobo_property_bag_client_get_property_type (pbc, name);
		switch (tc->kind) {
		case CORBA_tk_boolean:
			row_array [1] = g_strdup (bonobo_property_bag_client_get_value_boolean (pbc, name) ? "TRUE" : "FALSE");
			break;
		case CORBA_tk_string:
			row_array [1] = g_strdup (bonobo_property_bag_client_get_value_string (pbc, name));
			break;
		case CORBA_tk_short:
			row_array [1] = g_strdup_printf ("%d", bonobo_property_bag_client_get_value_short (pbc, name));
			break;
		case CORBA_tk_ushort:
			row_array [1] = g_strdup_printf ("%d", bonobo_property_bag_client_get_value_ushort (pbc, name));
			break;
		case CORBA_tk_long:
			row_array [1] = g_strdup_printf ("%ld", bonobo_property_bag_client_get_value_long (pbc, name));
			break;
		case CORBA_tk_ulong:
			row_array [1] = g_strdup_printf ("%ld", bonobo_property_bag_client_get_value_ulong (pbc, name));
			break;
		case CORBA_tk_float:
			row_array [1] = g_strdup_printf ("%f", bonobo_property_bag_client_get_value_float (pbc, name));
			break;
		case CORBA_tk_double:
			row_array [1] = g_strdup_printf ("%g", bonobo_property_bag_client_get_value_double (pbc, name));
			break;
		default:
			row_array [1] = g_strdup ("Unhandled Property Type");
		}

		gtk_clist_append (clist, row_array);
	}
	g_list_free (property_list);
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

static guint
container_create (void)
{
	GtkWidget       *app;
	GtkWidget       *control;
	GtkWidget       *proplist;
	GtkWidget       *box;
	GtkWidget       *button;
	BonoboUIHandler *uih;

	app = gnome_app_new ("sample-control-container",
			     "Sample Bonobo Control Container");
	gtk_window_set_default_size (GTK_WINDOW (app), 500, 440);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	uih = bonobo_ui_handler_new ();

	bonobo_ui_handler_set_app (uih, GNOME_APP (app));
	bonobo_ui_handler_create_menubar (uih);

	control = bonobo_widget_new_control (
		"control:calculator",
		bonobo_object_corba_objref (BONOBO_OBJECT (uih)));

	proplist = create_proplist (control);

	box = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), box);
	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), proplist, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Increment Calc");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc)incr_calc, control);
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

	gnome_CORBA_init ("sample-control-container", "1.0", &argc, argv, 0, &ev);

	CORBA_exception_free (&ev);

	orb = gnome_CORBA_ORB ();

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error ("Could not initialize Bonobo\n");

	/*
	 * We can't make any CORBA calls unless we're in the main
	 * loop.  So we delay creating the container here.
	 */
	gtk_idle_add ((GtkFunction) container_create, NULL);

	bonobo_main ();

	return 0;
}
