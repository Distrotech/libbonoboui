#include <gnome.h>
#include <bonobo.h>
#include "config.h"
#if USING_OAF
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif

#include "container.h"
#include "component.h"
#include "container-io.h"
#include "container-menu.h"

poptContext ctx;

void
sample_app_exit (SampleApp *app)
{
	while (app->components)
		bonobo_object_unref (
			BONOBO_OBJECT (app->components->data));

	bonobo_object_unref (BONOBO_OBJECT (app->container));
	bonobo_object_unref (BONOBO_OBJECT (app->ui_handler));

	gtk_main_quit ();
}

static void
delete_cb (GtkWidget *caller, SampleApp *app)
{
	sample_app_exit (app);
}

static SampleApp *
sample_app_create (void)
{
	SampleApp *app = g_new0 (SampleApp, 1);
	GtkWidget *app_widget;
	
	/* Create widgets */
	app_widget = app->app = gnome_app_new ("sample-container",
					       _("Sample Bonobo container"));
	app->box = gtk_vbox_new (FALSE, 10);

	gtk_signal_connect (GTK_OBJECT (app_widget), "destroy", delete_cb, app);

	/* Do the packing stuff */
	gnome_app_set_contents (GNOME_APP (app_widget), app->box);
	gtk_widget_set_usize (app_widget, 400, 600);

	app->container = bonobo_container_new ();
	app->ui_handler = bonobo_ui_handler_new ();
	bonobo_ui_handler_set_app (app->ui_handler, GNOME_APP (app_widget));

	/* Create menu bar */
	bonobo_ui_handler_create_menubar (app->ui_handler);
	sample_app_fill_menu (app);

	gtk_widget_show_all (app_widget);

	return app;
}

SampleClientSite *
sample_app_add_component (SampleApp *app,
			  gchar     *object_id)
{
	SampleClientSite   *site;
	BonoboObjectClient *server;

	server = bonobo_object_activate_with_oaf_id (object_id, 0);

	if (!server) {
		gchar *error_msg;

		error_msg =
		    g_strdup_printf (_("Could not launch Embeddable %s!"),
				     object_id);
		gnome_warning_dialog (error_msg);
		g_free (error_msg);

		return NULL;
	}

	/*
	 * The ClientSite is the container-side point of contact for
	 * the Embeddable.  So there is a one-to-one correspondence
	 * between BonoboClientSites and BonoboEmbeddables.
	 */
	site = sample_client_site_new (app->container, app,
				       server, object_id);
	if (!site) {
		g_warning ("Can't create client site");
		return NULL;
	}

	app->components = g_list_append (app->components, site);

	gtk_box_pack_start (GTK_BOX (app->box),
			    sample_client_site_get_widget (site),
			    FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET (app->box));
	
	return site;
}

typedef struct {
	SampleApp *app;
	const char **startup_files;
} setup_data_t;

/*
 *  This is placed after the Bonobo main loop has started
 * and we can handle CORBA properly.
 */
static guint
final_setup (setup_data_t *sd)
{
	const gchar **filenames = sd->startup_files;

	while (filenames && *filenames) {
		sample_container_load (sd->app, *filenames);
		filenames++;
	}

	return FALSE;
}

int
main (int argc, char **argv)
{
	setup_data_t init_data;
	CORBA_Environment ev;
	CORBA_ORB orb;

	CORBA_exception_init (&ev);
#if USING_OAF
	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);
#else
	gnome_CORBA_init_with_popt_table ("container", VERSION,
					  &argc, argv,
					  NULL, 0, &ctx, 0, &ev);

	CORBA_exception_free (&ev);
	orb = gnome_CORBA_ORB ();
#endif

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	init_data.app = sample_app_create ();

	if (ctx)
		init_data.startup_files = poptGetArgs (ctx);
	else
		init_data.startup_files = NULL;

	gtk_idle_add ((GtkFunction) final_setup, &init_data);

	bonobo_main ();

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
