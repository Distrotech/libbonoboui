#include <unistd.h>
#include <bonobo.h>
#include <glib.h>

#define APPNAME "TestContainer"
#define APPVERSION "1.0"

/*
 * FIXME: TODO:
 *
 *  + Add a Menu item 'activate' to do a control_frame_activate /
 *  trigger a UI merge / de-merge.
 *  + Add gtk-only mode
 */
static void
window_destroyed (GtkWindow *window, gpointer user_data)
{
	g_warning ("FIXME: should count toplevels");
        bonobo_main_quit ();
}

static void
window_title (GtkWindow *window, const char *moniker, gboolean use_gtk)
{
	char *title;

	title = g_strdup_printf ("%s - in a %s", moniker,
				 g_type_name_from_instance ((gpointer) window));
	gtk_window_set_title (window, title);
	g_free (title);
}

static void
window_create (const char *moniker, gboolean use_gtk)
{
	GtkWidget         *window;
	GtkWidget         *control;
	BonoboUIContainer *ui_container;

	if (use_gtk) {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		ui_container = CORBA_OBJECT_NIL;
	} else {
		window = bonobo_window_new (APPNAME, APPNAME);
		ui_container = bonobo_window_get_ui_container (BONOBO_WINDOW (window));
	}

	window_title (GTK_WINDOW (window), moniker, use_gtk);

	control = bonobo_widget_new_control (moniker, BONOBO_OBJREF (ui_container));

	if (!control) {
		g_warning ("Couldn't create a control for '%s'", moniker);
		return;
	}

	if (use_gtk)
		gtk_container_add (GTK_CONTAINER (window), control);
	else
		bonobo_window_set_contents (BONOBO_WINDOW (window), control);

	g_signal_connect (window, "destroy",
			  G_CALLBACK (window_destroyed), NULL);

	gtk_widget_show_all (window);
}

int
main ( int argc, char *argv[] )
{
	int           i;
	const char  **args;
	poptContext   popt_context;
	GnomeProgram *program;

	static gboolean use_gtk_window = FALSE;
	static struct poptOption options[] = {
		{ "gtk", 'g', POPT_ARG_NONE, &use_gtk_window, 0,
		  "Use GtkWindow instead of BonoboWindow (default)", NULL },
		POPT_AUTOHELP POPT_TABLEEND
	};

	program = gnome_program_init (APPNAME, APPVERSION,
				      LIBBONOBOUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_NONE);

        g_object_get (program, GNOME_PARAM_POPT_CONTEXT, &popt_context, NULL);

	/* Check for argument consistency. */
	if (!popt_context || !(args = poptGetArgs (popt_context))) {
		g_message ("Must specify a filename");
		return 1;
	}

	for (i = 0; args[i]; i++) {
		char *moniker;

		/* FIXME: we should do some auto-detection here */
		moniker = g_strdup_printf ("file:%s", args[i]);

		window_create (moniker, use_gtk_window);
	}

	bonobo_main ();

	poptFreeContext (popt_context);

        return 0;
}
