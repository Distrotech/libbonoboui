#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-container.h>
#include <bonobo/gnome-client-site.h>

CORBA_Environment ev;
CORBA_ORB orb;

char *server_repoid = "IDL:Sample/server:1.0";

typedef struct {
	GtkWidget *app;
	GnomeContainer *container;
	GtkWidget *box;
	GNOME_View view;
} Application;

static GnomeObject *
launch_server (GnomeContainer *container, char *repoid)
{
	GnomeClientSite *client_site;
	GnomeObject     *object_server;
	
	client_site = gnome_client_site_new (container);
	gnome_container_add (container, GNOME_OBJECT (client_site));

	object_server = gnome_object_activate_with_repo_id (NULL, repoid, 0, NULL);
	if (!object_server){
		g_warning (_("Can not activate object_server\n"));
		return NULL;
	}

	if (!gnome_client_site_bind_component (client_site, object_server)){
		g_warning (_("Can not bind object server to client_site\n"));
		return NULL;
	}

	return object_server;
}

static void
add_cmd (GtkWidget *widget, Application *app)
{
	GtkWidget *frame, *socket, *w;
	GnomeObject *server;
	GNOME_View view;
	GNOME_View_windowid id;
	
	server = launch_server (app->container, server_repoid);
	if (server == NULL)
		return;

	w = gnome_component_new_view (server);

	frame = gtk_frame_new ("Component");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (app->box), frame, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), w);

	gtk_widget_show_all (frame);
}

static void
exit_cmd (void)
{
	gtk_main_quit ();
}

static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE(N_("_Add a new Sample::server:1.0..."), NULL, add_cmd),
	GNOMEUIINFO_ITEM_STOCK (N_("Exit"), NULL, exit_cmd, GNOME_STOCK_PIXMAP_QUIT),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_main_menu [] = {
	GNOMEUIINFO_MENU_FILE_TREE (container_file_menu),
	GNOMEUIINFO_END
};

static Application *
application_new (void)
{
	Application *app;

	app = g_new0 (Application, 1);
	app->app = gnome_app_new ("test-container", "Sample Container Application");
	app->container = GNOME_CONTAINER (gnome_container_new ());

	app->box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (app->box);
	gnome_app_set_contents (GNOME_APP (app->app), app->box);
	gnome_app_create_menus_with_data (GNOME_APP (app->app), container_main_menu, app);
	gtk_widget_show (app->app);

	return app;
}

void
main (int argc, char *argv [])
{
	Application *app;


	CORBA_exception_init (&ev);
	
	gnome_CORBA_init ("MyShell", "1.0", &argc, argv, 0, &ev);
	orb = gnome_CORBA_ORB ();
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init\n"));

	app = application_new ();
	
	gtk_main ();

	CORBA_exception_free (&ev);
}
