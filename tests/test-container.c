/*
 * test-container.c
 *
 * A simple program to act as a test container for Embeddables.
 *
 * Authors:
 *    Nat Friedman (nat@gnome-support.com)
 *    Miguel de Icaza (miguel@gnu.org)
 */
 
#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/gnome-bonobo.h>
#include <sys/stat.h>
#include <unistd.h>

CORBA_Environment ev;
CORBA_ORB orb;

/*
 * A handle to some Embeddables and their ClientSites so we can add
 * views to existing components.
 */

GnomeObjectClient *text_obj;
GnomeClientSite *text_client_site;

GnomeObjectClient *image_png_obj;
GnomeClientSite   *image_client_site;

char *server_goadid = "Test_server_bonobo_object";

typedef struct {
	GtkWidget *app;
	GnomeContainer *container;
	GtkWidget *box;
	GNOME_View view;
} Application;

static GnomeObjectClient *
launch_server (GnomeClientSite *client_site, GnomeContainer *container, char *goadid)
{
	GnomeObjectClient *object_server;
	
	gnome_container_add (container, GNOME_OBJECT (client_site));

	printf ("Launching...\n");
	object_server = gnome_object_activate_with_goad_id (NULL, goadid, 0, NULL);
	printf ("Return: %p\n", object_server);
	if (!object_server){
		g_warning (_("Can not activate object_server\n"));
		return NULL;
	}

	if (!gnome_client_site_bind_bonobo_object (client_site, object_server)){
		g_warning (_("Can not bind object server to client_site\n"));
		return NULL;
	}

	return object_server;
}

static gboolean
view_frame_activated_cb (GnomeViewFrame *view_frame, gboolean state,
			 GnomeObjectClient *server_object)
{

	if (state) {
		GNOME_Embeddable_verb_list *verbs;
		int i;

		CORBA_exception_init (&ev);
		verbs = GNOME_Embeddable_get_verb_list (
			gnome_object_corba_objref (GNOME_OBJECT (server_object)),
			&ev);
		if (ev._major != CORBA_NO_EXCEPTION) {
			g_warning ("Could not get verb list!\n");
		}

		for (i = 0; i < verbs->_length; i ++) {
			printf ("Got Verb: %s\n", verbs->_buffer[i]);
		}			

	}

	return FALSE;
} /* view_frame_activated_cb */

static GnomeViewFrame *
add_view (GtkWidget *widget, Application *app,
	  GnomeClientSite *client_site, GnomeObjectClient *server) 
{
	GnomeViewFrame *view_frame;
	GtkWidget *view_widget;
	GtkWidget *frame;
	
	view_frame = gnome_embeddable_client_new_view_simple (server,
								 client_site);
	gtk_signal_connect (GTK_OBJECT (view_frame), "view_activated",
			    GTK_SIGNAL_FUNC (view_frame_activated_cb),
			    server);

	view_widget = gnome_view_frame_get_wrapper (view_frame);

	frame = gtk_frame_new ("Embeddable");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (app->box), frame, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), view_widget);

	gtk_widget_show_all (frame);

	return view_frame;
} /* add_view */


static GnomeObjectClient *
add_cmd (GtkWidget *widget, Application *app, char *server_goadid,
	 GnomeClientSite **client_site)
{
	GnomeObjectClient *server;
	
	*client_site = gnome_client_site_new (app->container);

	server = launch_server (*client_site, app->container, server_goadid);
	if (server == NULL)
		return NULL;

	add_view (widget, app, *client_site, server);
	return server;
}

static void
add_demo_cmd (GtkWidget *widget, Application *app)
{
	GnomeClientSite *client_site;
	add_cmd (widget, app, server_goadid, &client_site);
}

static void
add_image_cmd (GtkWidget *widget, Application *app)
{
	GnomeObjectClient *object;
	GnomeStream *stream;
	GNOME_PersistStream persist;

	object = add_cmd (widget, app, "bonobo-object:image-x-png", &image_client_site);
	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch bonobo object."));
	    return;
	  }

	image_png_obj = object;

	persist = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (GNOME_OBJECT (object)),
		"IDL:GNOME/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports PersistStream\n");
	
	stream = gnome_stream_fs_open (NULL, "/tmp/a.png", GNOME_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /tmp/a.png!\n");
		return;
	}
	
	GNOME_PersistStream_load (
		persist,
		(GNOME_Stream) gnome_object_corba_objref (GNOME_OBJECT (stream)), &ev);

	GNOME_Unknown_unref (persist, &ev);
	CORBA_Object_release (persist, &ev);
}

/*
 * Add a new view for the existing image/x-png Embeddable.
 */
static void
add_image_view (GtkWidget *widget, Application *app)
{
	if (image_png_obj == NULL)
		return;

	add_view (NULL, app, image_client_site, image_png_obj);
} /* add_image_view */

/*
 * This function uses GNOME::PersistStream to load a set of data into
 * the text/plain Embeddable.
 */
static void
add_text_cmd (GtkWidget *widget, Application *app)
{
	GnomeObjectClient *object;
	GnomeStream *stream;
	GNOME_PersistStream persist;

	object = add_cmd (widget, app, "bonobo-object:text-plain", &text_client_site);
	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch Embeddable."));
	    return;
	  }

	text_obj = object;

	persist = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (GNOME_OBJECT (object)),
		"IDL:GNOME/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports PersistStream\n");
	
	stream = gnome_stream_fs_open (NULL, "/usr/dict/words",
				       GNOME_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /usr/dict/words!\n");
		return;
	}
	
	GNOME_PersistStream_load (
	     persist, (GNOME_Stream) gnome_object_corba_objref (GNOME_OBJECT (stream)), &ev);

	GNOME_Unknown_unref (persist, &ev);
	CORBA_Object_release (persist, &ev);
} /* add_text_cmd */

/*
 * These functions handle the progressive transmission of data
 * to the text/plain Embeddable.
 */
struct progressive_timeout {
	GNOME_ProgressiveDataSink psink;
	FILE *f;
};

/*
 * Send a new line to the text/plain Embeddable.
 */
static gboolean
timeout_next_line (gpointer data)
{
	struct progressive_timeout *tmt = (struct progressive_timeout *) data;
  
	GNOME_ProgressiveDataSink_iobuf *buffer;
	char line[1024];
	int line_len;
	
	if (fgets (line, sizeof (line), tmt->f) == NULL)
	{
		GNOME_ProgressiveDataSink_end (tmt->psink, &ev);

		fclose (tmt->f);

		GNOME_Unknown_unref (tmt->psink, &ev);
		CORBA_Object_release (tmt->psink, &ev);

		g_free (tmt);
		return FALSE;
	}

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
 * Add a new view for the existing text Embeddable.
 */
static void
add_text_view (GtkWidget *widget, Application *app)
{
	if (text_obj == NULL)
		return;

	add_view (NULL, app, text_client_site, text_obj);
} /* add_text_view */

/*
 * Setup a timer to send a new line to the text/plain Embeddable using
 * ProgressiveDataSink.
 */
static void
send_text_cmd (GtkWidget *widget, Application *app)
{
	GNOME_ProgressiveDataSink psink;
	struct progressive_timeout *tmt;
	struct stat statbuf;
	FILE *f;

	if (text_obj == NULL)
		return;

	psink = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (GNOME_OBJECT (text_obj)),
		"IDL:GNOME/ProgressiveDataSink:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (psink == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports ProgressiveDataSink\n");

	tmt = g_new0 (struct progressive_timeout, 1);

	GNOME_ProgressiveDataSink_start (psink, &ev);

	f = fopen ("/etc/passwd", "r");
	if (f == NULL) {
		printf ("I could not open /etc/passwd!\n");
		return;
	}
	
	fstat (fileno (f), &statbuf);
	GNOME_ProgressiveDataSink_set_size (psink,
					    (CORBA_long) statbuf.st_size,
					    &ev);

	tmt->psink = psink;
	tmt->f = f;

	g_timeout_add (500, timeout_next_line, (gpointer) tmt);
} /* send_text_cmd */

static void
exit_cmd (void)
{
	gtk_main_quit ();
}

static GnomeUIInfo container_text_plain_menu [] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Add a new text/plain component"), NULL,
			       add_text_cmd),
	GNOMEUIINFO_ITEM_NONE (
	N_("_Send progressive data to an existing text/plain component"),
			       NULL, send_text_cmd),
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new _view to an existing text/plain component"),
			       NULL, add_text_view),
	
	GNOMEUIINFO_END
};

static GnomeUIInfo container_image_png_menu [] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Add a new image/x-png component"), NULL,
			       add_image_cmd),
 	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new _view to an existing image/x-png component"),
			       NULL, add_image_view),
	
	GNOMEUIINFO_END
};

static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE(N_("Add a new _object"), NULL, add_demo_cmd),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cmd, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_main_menu [] = {
	GNOMEUIINFO_MENU_FILE_TREE (container_file_menu),
	GNOMEUIINFO_SUBTREE (N_("_text/plain"), container_text_plain_menu),
	GNOMEUIINFO_SUBTREE (N_("_image/x-png"), container_image_png_menu),
	GNOMEUIINFO_END
};

static Application *
application_new (void)
{
	Application *app;

	app = g_new0 (Application, 1);
	app->app = gnome_app_new ("test-container",
				  "Sample Container Application");
	app->container = GNOME_CONTAINER (gnome_container_new ());

	app->box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (app->box);
	gnome_app_set_contents (GNOME_APP (app->app), app->box);
	gnome_app_create_menus_with_data (GNOME_APP (app->app),
					  container_main_menu, app);
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
