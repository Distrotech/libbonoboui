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
#include <bonobo/gnome-bonobo.h>

GList *property_list;

#define CORBA_boolean__alloc() (CORBA_boolean *) CORBA_octet_allocbuf (sizeof (CORBA_boolean))

static void populate_property_list (GtkWidget *bw, GtkCList *clist);

static void
edit_property (GtkCList *clist, GdkEventButton *event, GnomeBonoboWidget *bw)
{
	GNOME_Property prop;
	CORBA_Environment ev;
	gint row, col;
	CORBA_any *any, *newany;
	CORBA_boolean *b;

	if (event->button == 3) {
		CORBA_exception_init (&ev);
		gtk_clist_get_selection_info (clist, event->x, event->y,
		                              &row, &col);

		/* Get the value of the property they clicked on. */
		prop = g_list_nth_data (property_list, row);
		any = GNOME_Property_get_value (prop, &ev);

		/* Change it appropriately. */
		switch (any->_type->kind) {
		case CORBA_tk_boolean:
			b = CORBA_boolean__alloc();
			*b = ! *((CORBA_boolean *) any->_value);
			newany = CORBA_any__alloc();
			newany->_type = (CORBA_TypeCode) TC_boolean;
			newany->_value = b;
			CORBA_any_set_release (newany, TRUE);
			GNOME_Property_set_value (prop, newany, &ev);
			CORBA_free (newany);
			break;
		default:
			g_warning ("Cannot set_value this type of property yet, sorry.");
			break;
			
		}


		CORBA_exception_free (&ev);

		/* Redraw the property list. */
		gtk_clist_clear (clist);
		populate_property_list (GTK_WIDGET(bw), clist);
	}

}


static void
populate_property_list (GtkWidget *bw, GtkCList *clist)
{
	GnomePropertyBagClient *pbc;
	GnomeControlFrame *cf;
	GList *l;
	CORBA_Environment ev;

	/* Get the list of properties. */
	if (property_list == NULL) {
		cf = gnome_bonobo_widget_get_control_frame (GNOME_BONOBO_WIDGET (bw));
		pbc = gnome_control_frame_get_control_property_bag (cf);
		property_list = gnome_property_bag_client_get_properties (pbc);
	}

	CORBA_exception_init (&ev);
	for (l = property_list; l != NULL; l = l->next) {
		GNOME_Property prop;
		CORBA_any *any;
		char *property_name;
		char *row_array[2];

		prop = l->data;
		property_name = GNOME_Property_get_name (prop, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			g_error ("populate_property_list: Exception trying to get property name");
		}
		
		any = GNOME_Property_get_value (prop, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			g_error ("populate_property_list: Exception trying to get property value");
		}

		row_array [0] = property_name;

		switch (any->_type->kind) {
		case CORBA_tk_boolean:
			row_array [1] = g_strdup (*((CORBA_boolean *) any->_value) ? "TRUE" : "FALSE");
			break;
		case CORBA_tk_string:
			row_array [1] = g_strdup (*((CORBA_char **) any->_value));
			break;
		case CORBA_tk_short:
			row_array [1] = g_strdup_printf ("%d", *(CORBA_short *) any->_value);
			break;
		case CORBA_tk_ushort:
			row_array [1] = g_strdup_printf ("%d", *(CORBA_unsigned_short *) any->_value);
			break;
		case CORBA_tk_long:
			row_array [1] = g_strdup_printf ("%d", *(CORBA_long *) any->_value);
			break;
		case CORBA_tk_ulong:
			row_array [1] = g_strdup_printf ("%d", *(CORBA_unsigned_long *) any->_value);
			break;
		case CORBA_tk_float:
			row_array [1] = g_strdup_printf ("%f", *(CORBA_float *) any->_value);
			break;
		case CORBA_tk_double:
			row_array [1] = g_strdup_printf ("%g", *(CORBA_double *) any->_value);
			break;
		default:
			row_array [1] = g_strdup ("Unhandled Property Type");
		}

		gtk_clist_append (clist, row_array);
	}
	CORBA_exception_free (&ev);
}

static GtkWidget *
table_create (GtkWidget *control)
{
	gchar *clist_titles[] = {"Property Name", "Value"};
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *clist;
	int i, j;

	table = gtk_table_new (3, 3, FALSE);

	/* Put some dead space around the sides. */
	for (i = 0; i < 3; i ++)
		for (j = 0; j < 2; j ++) {

			if (i == 1 && j == 1)
				continue;

			label = gtk_label_new ("Dead space");
			gtk_widget_set_usize (label, 100, 100);
			gtk_table_attach (GTK_TABLE (table), label,
					  i, i + 1,
					  j, j + 1,
					  0, 0,
					  0, 0);
		}

	/* Put the control in the center. */
	gtk_widget_show (control);
	gtk_table_attach (GTK_TABLE (table), control,
			  1, 2, 1, 2,
			  0, 0,
			  0, 0);

	/* Put the property CList on the bottom. */
	clist = gtk_clist_new_with_titles (2, clist_titles);
	gtk_signal_connect (GTK_OBJECT (clist), "button_press_event",
		GTK_SIGNAL_FUNC (edit_property), control);
 
	gtk_table_attach (GTK_TABLE (table), clist,
			  0, 3, 2, 3,
			  GTK_EXPAND | GTK_FILL, 
			  GTK_EXPAND | GTK_FILL, 
			  0, 0);

	populate_property_list (control, GTK_CLIST (clist));

	control = gnome_bonobo_widget_new_control ("control:calculator");
	gtk_widget_show (control);
	gtk_table_attach (GTK_TABLE (table), control,
			  0, 1, 0, 1,
			  0, 0,
			  0, 0);

	return table;
}

static guint
container_create (void)
{
	GtkWidget *app;
	GtkWidget *control;

	app = gnome_app_new ("sample-control-container",
			     "Sample Bonobo Control Container");
	gtk_window_set_default_size (GTK_WINDOW (app), 500, 440);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	control = gnome_bonobo_widget_new_control ("control:clock");
	gnome_app_set_contents (GNOME_APP (app), table_create (control));

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
