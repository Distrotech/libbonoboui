#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/gnome-bonobo.h>
#include <bonobo/gnome-stream-fs.h>

CORBA_Environment ev;
CORBA_ORB orb;

/* A handle to the last text/plain widget created. */
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

/*
 * This function uses GNOME::PersistStream to load a set of data into
 * the text/plain component.
 */
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

/*
 * These functions handle the progressive transmission of data
 * to the text/plain component.
 */
struct progressive_timeout {
	GNOME_ProgressiveDataSink psink;
	FILE *f;
};

/*
 * Send a new line to the text/plain component.
 */
static gboolean
timeout_next_line (gpointer data)
{
	struct progressive_timeout *tmt = (struct progressive_timeout *) data;
  
	GNOME_ProgressiveDataSink_iobuf *buffer;
	char line[1024];
	int line_len;
	
	if (fgets (line, sizeof (line), tmt->f) == NULL)
	  return FALSE;

	line_len = strlen (line);

	buffer = GNOME_ProgressiveDataSink_iobuf__alloc ();
	CORBA_sequence_set_release (buffer, TRUE);

	buffer->_length = line_len;
	buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (line_len);
	memcpy (buffer->_buffer, line, line_len);

	GNOME_ProgressiveDataSink_add_data (tmt->psink, buffer, &ev);

	return TRUE;
} /* timeout_add_more_data */

/*
 * Setup a timer to send a new line to the text/plain component using
 * ProgressiveDataSink.
 */
static void
send_text_cmd (GtkWidget *widget, Application *app)
{
	GNOME_ProgressiveDataSink psink;
	struct progressive_timeout *tmt;
	FILE *f;

	if (text_obj == NULL)
		return;

	psink = GNOME_obj_query_interface (
		GNOME_OBJECT (text_obj)->object,
		"IDL:GNOME/ProgressiveDataSink:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (psink == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Component supports ProgressiveDataSink");

	tmt = g_new0 (struct progressive_timeout, 1);

	GNOME_ProgressiveDataSink_start (psink, &ev);

	CORBA_exception_free (&ev);

	f = fopen ("/usr/dict/words", "r");
	if (f == NULL) {
		printf ("I could not open /usr/dict/words!\n");
		return;
	}
	
	tmt->psink = psink;
	tmt->f = f;
	
	g_timeout_add (500, timeout_next_line, (gpointer) tmt);
} /* send_text_cmd */

static void
exit_cmd (void)
{
	gtk_main_quit ();
}

static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE(N_("Add a new _object"), NULL, add_demo_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("Add a new _image/x-png handler"), NULL, add_image_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("Add a new _text/plain handler"), NULL, add_text_cmd),
	GNOMEUIINFO_ITEM_NONE(N_("_Send progressive data to an existing text component"), NULL, send_text_cmd),
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
