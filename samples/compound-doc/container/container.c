#include <gnome.h>
#include <bonobo.h>
#include "config.h"
#include <liboaf/liboaf.h>

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

#ifdef USE_UI_HANDLER	

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
#else
	/* Create widgets */
	app->app = bonobo_app_new ("sample-container",
				   _("Sample Bonobo container"));
	app_widget = bonobo_app_get_window (app->app);

	app->box = gtk_vbox_new (FALSE, 10);

	gtk_signal_connect (GTK_OBJECT (app_widget),
			    "destroy", delete_cb, app);

	/* Do the packing stuff */
	bonobo_app_set_contents (app->app, app->box);
	gtk_widget_set_usize (app_widget, 400, 600);

	app->container = bonobo_container_new ();
	app->ui_handler = bonobo_ui_handler_new_for_app (app->app);

	/* Create menu bar */
	bonobo_ui_handler_create_menubar (app->ui_handler);
	sample_app_fill_menu (app);

	gtk_widget_show_all (app_widget);
#endif

	return app;
}

static SampleClientSite *
sample_app_add_embeddable (SampleApp          *app,
			   BonoboObjectClient *server,
			   char               *object_id)
{
	SampleClientSite *site;

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

	sample_client_site_add_frame (site);

	gtk_widget_show_all (GTK_WIDGET (app->box));
	
	return site;
}

SampleClientSite *
sample_app_add_component (SampleApp *app,
			  gchar     *object_id)
{
	BonoboObjectClient *server;

	server = bonobo_object_activate (object_id, 0);

	if (!server) {
		gchar *error_msg;

		error_msg =
		    g_strdup_printf (_("Could not launch Embeddable %s!"),
				     object_id);
		gnome_warning_dialog (error_msg);
		g_free (error_msg);

		return NULL;
	}

	return sample_app_add_embeddable (app, server, object_id);
}

typedef struct {
	SampleApp *app;
	const char **startup_files;
} setup_data_t;

static Bonobo_Moniker
make_moniker (const char *name)
{
	Bonobo_Moniker      moniker;
	CORBA_Environment   ev;

	CORBA_exception_init (&ev);

	moniker = bonobo_moniker_client_new_from_name (name, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("Moniker new exception '%s'\n",
			   bonobo_exception_get_text (&ev));
		return CORBA_OBJECT_NIL;
	}
	
	return moniker;
}

static void
resolve_and_add (SampleApp *app, Bonobo_Moniker moniker, const char *interface)
{
	Bonobo_Embeddable embeddable;
	SampleClientSite *site;
	CORBA_Environment ev;
	char             *name;

	CORBA_exception_init (&ev);

	embeddable = bonobo_moniker_client_resolve_default (
		moniker, interface, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("Moniker resolve exception '%s'\n",
			 bonobo_exception_get_text (&ev));
		return;
	}

	name = bonobo_moniker_client_get_name (moniker, &ev);
	g_print ("My moniker looks like '%s'\n", name);
	
	if (ev._major != CORBA_NO_EXCEPTION)
		g_warning ("Moniker get name exception '%s'\n",
			   bonobo_exception_get_text (&ev));

	site = sample_app_add_embeddable (
		app, bonobo_object_client_from_corba (embeddable),
		name);

	if (!site)
		g_warning ("Failed to add embeddable to app");

	bonobo_object_release_unref (moniker, &ev);
}

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

	if (g_getenv ("BONOBO_MONIKER_TEST")) {
		Bonobo_Moniker moniker;

		moniker = make_moniker ("file:/demo/a.jpeg");
		resolve_and_add (sd->app, moniker, "IDL:Bonobo/Embeddable:1.0");
		bonobo_object_release_unref (moniker, NULL);

		moniker = make_moniker ("OAFIID:bonobo_application-x-mines:804d34a8-57dd-428b-9c94-7aa3a8365230");
		resolve_and_add (sd->app, moniker, "IDL:Bonobo/Embeddable:1.0");
		bonobo_object_release_unref (moniker, NULL);

		moniker = make_moniker ("query:(bonobo:supported_mime_types.has ('image/x-png'))");
		resolve_and_add (sd->app, moniker, "IDL:Bonobo/Embeddable:1.0");
		bonobo_object_release_unref (moniker, NULL);
	}

	return FALSE;
}

int
main (int argc, char **argv)
{
	setup_data_t init_data;
	CORBA_Environment ev;
	CORBA_ORB orb;

	free (malloc (8));

 	CORBA_exception_init (&ev);
	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);

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
