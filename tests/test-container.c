#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/gnome-bonobo.h>
#include <bonobo/gnome-stream-fs.h>

CORBA_Environment ev;
CORBA_ORB orb;

GnomeObjectClient *text_obj;

char *server_goadid = "Test_server_component";

typedef struct {
	GtkWidget *app;
	GnomeContainer *container;
	GtkWidget *box;
	GNOME_View view;
} Application;

static GnomeObjectClient *
launch_server (GnomeContainer *container, char *goadid)
{
	GnomeClientSite *client_site;
	GnomeObjectClient *object_server;
	
	client_site = gnome_client_site_new (container);
	gnome_container_add (container, GNOME_OBJECT (client_site));

	printf ("Launching...\n");
	object_server = gnome_object_activate_with_goad_id (NULL, goadid, 0, NULL);
	printf ("Return: %p\n", object_server);
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

static GnomeObjectClient *
add_cmd (GtkWidget *widget, Application *app, char *server_goadid)
{
	GtkWidget *frame, *w;
	GnomeObjectClient *server;
	GNOME_View view;
	GNOME_View_windowid id;
	
	server = launch_server (app->container, server_goadid);
	if (server == NULL)
		return NULL;

	w = gnome_component_new_view (server);

	frame = gtk_frame_new ("Component");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (app->box), frame, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), w);

	gtk_widget_show_all (frame);

	gnome_component_client_activate (server);

	return server;
}

static void
add_demo_cmd (GtkWidget *widget, Application *app)
{
	add_cmd (widget, app, server_goadid);
}

static void
add_image_cmd (GtkWidget *widget, Application *app)
{
	GnomeObjectClient *object;
	GnomeStream *stream;
	GNOME_PersistStream persist;

	object = add_cmd (widget, app, "component:image-x-png");
	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch component."));
	    return;
	  }
	persist = GNOME_obj_query_interface (
		GNOME_OBJECT (object)->object,
		"IDL:GNOME/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Component supports PersistStream");
	
	stream = gnome_stream_fs_open (NULL, "/tmp/a.png", GNOME_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /tmp/a.png!\n");
		return;
	}
	
	GNOME_PersistStream_load (persist, (GNOME_Stream) GNOME_OBJECT (stream)->object, &ev);
}

static void
add_text_cmd (GtkWidget *widget, Application *app)
{
	GnomeObjectClient *object;
	GnomeStream *stream;
	GNOME_PersistStream persist;

	object = add_cmd (widget, app, "component:text-plain");
	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch component."));
	    return;
	  }

	text_obj = object;

	persist = GNOME_obj_query_interface (
		GNOME_OBJECT (object)->object,
		"IDL:GNOME/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Component supports PersistStream");
	
	stream = gnome_stream_fs_open (NULL, "/usr/dict/words",
				       GNOME_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /usr/dict/words!\n");
		return;
	}
	
	GNOME_PersistStream_load (persist, (GNOME_Stream) GNOME_OBJECT (stream)->object, &ev);

}

static void
send_text_cmd (GtkWidget *widget, Application *app)
{
	GnomeObjectClient *object = text_obj;
	GnomeStream *stream;
	GNOME_PersistStream persist;

	persist = GNOME_obj_query_interface (
		GNOME_OBJECT (object)->object,
		"IDL:GNOME/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Component supports PersistStream");
	
	stream = gnome_stream_fs_open (NULL, "/tmp/pipe",
				       GNOME_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /tmp/pipe!\n");
		return;
	}
	
	GNOME_PersistStream_load (persist, (GNOME_Stream) GNOME_OBJECT (stream)->object, &ev);

}

static void
exit_cmd (void)
{
	gtk_main_quit ();
}

static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE(N_("_Add a new object"), NULL, add_demo_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("_Add a new image/x-png handler"), NULL, add_image_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("_Add a new text/plain handler"), NULL, add_text_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("Send data from /tmp/pipe to an existing text component (to show off progressive loading)"), NULL, send_text_cmd),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cmd, NULL),
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

int
main (int argc, char *argv [])
{
	Application *app;

	if (argc != 1){
		server_goadid = argv [1];
	}
	
	CORBA_exception_init (&ev);
	
	gnome_CORBA_init ("MyShell", "1.0", &argc, argv, 0, &ev);
	orb = gnome_CORBA_ORB ();
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init\n"));

	app = application_new ();
	
	gtk_main ();

	CORBA_exception_free (&ev);
	
	return 0;
}
