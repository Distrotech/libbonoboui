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

static GtkWidget *
table_create (GtkWidget *center)
{
	GtkWidget *table;
	GtkWidget *text;
	int i, j;

	table = gtk_table_new (3, 3, FALSE);

	for (i = 0; i < 3; i ++)
		for (j = 0; j < 3; j ++) {

			if (i == 1 && j == 1)
				continue;

			if (i == 0 && j == 0)
				continue;

			text = gtk_text_new (NULL, NULL);
			gtk_table_attach (GTK_TABLE (table), text,
					  i, i + 1,
					  j, j + 1,
					  0, 0,
					  0, 0);
		}

	gtk_table_attach (GTK_TABLE (table), center,
			  1, 2, 1, 2,
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
