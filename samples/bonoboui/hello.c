/*
 * hello.c
 *
 * A hello world application using the Bonobo UI handler
 *
 * Authors:
 *	Michael Meeks    <michael@ximian.com>
 *	Murray Cumming   <murrayc@usa.net>
 *      Havoc Pennington <hp@redhat.com>
 *
 * Copyright (C) 1999 Red Hat, Inc.
 *               2001 Murray Cumming,
 *               2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "config.h"

#include <stdlib.h>

#include <libbonoboui.h>

/* Keep a list of all open application windows */
static GSList *app_list = NULL;

#define HELLO_UI_XML "Bonobo_Sample_Hello.xml"

/*   A single forward prototype - try and keep these
 * to a minumum by ordering your code nicely */
static GtkWidget *hello_new (const gchar *message,
			     const gchar *geometry);

static void
hello_on_menu_file_new (BonoboUIComponent *uic,
			   gpointer           user_data,
			   const gchar       *verbname)
{
	GtkWidget *app;

	app = hello_new (_("Hello, World!"), NULL);
	
	gtk_widget_show_all(app);
}

static void
show_nothing_dialog(GtkWidget* parent)
{
	GtkWidget* dialog;

	dialog = gtk_message_dialog_new (
		GTK_WINDOW (parent),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		_("This does nothing; it is only a demonstration."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
hello_on_menu_file_open (BonoboUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_save (BonoboUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_saveas (BonoboUIComponent *uic,
			      gpointer           user_data,
			      const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_exit (BonoboUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	/* FIXME: quit the mainloop nicely */
	exit (0);
}	

static void
hello_on_menu_file_close (BonoboUIComponent *uic,
			     gpointer           user_data,
			     const gchar       *verbname)
{
	GtkWidget *app = user_data;

	/* Remove instance: */
	app_list = g_slist_remove (app_list, app);

	gtk_widget_destroy (app);

	if (!app_list)
		hello_on_menu_file_exit(uic, user_data, verbname);
}

static void
hello_on_menu_edit_undo (BonoboUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}	

static void
hello_on_menu_edit_redo (BonoboUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}	

static void
hello_on_menu_help_about (BonoboUIComponent *uic,
			     gpointer           user_data,
			     const gchar       *verbname)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (
		GTK_WINDOW (user_data),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		_("BonoboUI-Hello."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
hello_on_button_click (GtkWidget* w, gpointer user_data)
{
	gchar    *text;
	GtkLabel *label = GTK_LABEL (user_data);

	text = g_strdup (gtk_label_get_text (label));

	g_strreverse (text);

	gtk_label_set_text (label, text);

	g_free (text);
}

/*
 *   These verb names are standard, see libonobobui/doc/std-ui.xml
 * to find a list of standard verb names.
 *   The menu items are specified in Bonobo_Sample_Hello.xml and
 * given names which map to these verbs here.
 */
static const BonoboUIVerb hello_verbs [] = {
	BONOBO_UI_VERB ("FileNew",    hello_on_menu_file_new),
	BONOBO_UI_VERB ("FileOpen",   hello_on_menu_file_open),
	BONOBO_UI_VERB ("FileSave",   hello_on_menu_file_save),
	BONOBO_UI_VERB ("FileSaveAs", hello_on_menu_file_saveas),
	BONOBO_UI_VERB ("FileClose",  hello_on_menu_file_close),
	BONOBO_UI_VERB ("FileExit",   hello_on_menu_file_exit),

	BONOBO_UI_VERB ("EditUndo",   hello_on_menu_edit_undo),
	BONOBO_UI_VERB ("EditRedo",   hello_on_menu_edit_redo),

	BONOBO_UI_VERB ("HelpAbout",  hello_on_menu_help_about),

	BONOBO_UI_VERB_END
};

static BonoboWindow *
hello_create_main_window (void)
{
	BonoboWindow      *win;

	win = BONOBO_WINDOW (bonobo_window_new (PACKAGE, _("Gnome Hello")));

#if 0
	BonoboUIContainer *ui_container;
	BonoboUIComponent *ui_component;

	/* Create Container: */
	ui_container = bonobo_window_get_ui_container (win);

	/* TODO: What does this do? */
	bonobo_ui_engine_config_set_path ( bonobo_window_get_ui_engine (win),
					   "/my-application-name/UIConfig/kvps");

	/* Set up Window's UI: */
	ui_component = bonobo_ui_component_new ("my-app");

	/* Associate the BonoboUIComponent with the container */
	bonobo_ui_component_set_container (
		ui_component, BONOBO_OBJREF (ui_container), NULL);

	/* Tell the BonoboUIComponent to use the UI XML file, in <prefix>/share/gnome/ */
	hello_check_for_ui_xml_file(ui_xml_filename); /*Tell user to symlink it if necessary. */
	bonobo_ui_util_set_ui (
		ui_component, HELLO_DATADIR, HELLO_UI_XML,
		 "my-app"); /* XML file describing the menus, toolbars, etc */

	/* TODO: */
	/* - How can I add stock menu items without specifying all the details? */
	/* - There are stock pixmaps, but how can I find a list of all the possible pixmap names? */
	/* - Toolbar items tend to share details and callbacks with menu items, but the cmd can't */
	/*     just be reused because toolbar items don't parse the navigation underscore.        */

	/* Associate our verb -> callback mapping with the BonoboWindow */
	/* All the callback's user_data pointers will be set to 'win' */
	bonobo_ui_component_add_verb_list_with_data (ui_component, hello_verbs, win);
#endif

	return win;
}

static gint 
delete_event_cb (GtkWidget *window, GdkEventAny *e, gpointer user_data)
{
	hello_on_menu_file_close (NULL, window, NULL);

	/* Prevent the window's destruction, since we destroyed it 
	 * ourselves with hello_app_close() */
	return TRUE;
}

static GtkWidget *
hello_new (const gchar *message,
	   const gchar *geometry)
{
	GtkWidget    *label;
	GtkWidget    *frame;
	GtkWidget    *button;
	BonoboWindow *win;

	win = hello_create_main_window();

	/* Add some widgets to the main BonoboWindow: */

	/* Create Button: */
	button = gtk_button_new ();
	gtk_container_set_border_width (GTK_CONTAINER (button), 10);

	/* Create Label and put it in the Button: */
	label = gtk_label_new (message ? message : _("Hello, World!"));
	gtk_container_add (GTK_CONTAINER (button), label);

	/* Connect the Button's 'clicked' signal to the signal handler:
	 * pass label as the data, so that the signal handler can use it. */
	gtk_signal_connect (
		GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC(hello_on_button_click), label);

	gtk_window_set_policy (GTK_WINDOW (win), FALSE, TRUE, FALSE);
	gtk_window_set_default_size (GTK_WINDOW (win), 250, 350);

	/* Create Frame and add it to the main BonoboWindow: */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), button);
	bonobo_window_set_contents (win, frame);

	/* Connect to the delete_event: a close from the window manager */
	gtk_signal_connect (GTK_OBJECT (win),
			    "delete_event",
			    GTK_SIGNAL_FUNC (delete_event_cb),
			    NULL);


	/*** gnomehello-geometry */
	/*
	  if (geometry != NULL) 
	  {
	  gint x, y, w, h;
	  if ( gnome_parse_geometry( geometry, 
	  &x, &y, &w, &h ) ) 
	  {
          if (x != -1)
	  {
	  gtk_widget_set_uposition(app, x, y);
	  }

          if (w != -1) 
	  {
	  gtk_window_set_default_size(GTK_WINDOW(app), w, h);
	  }
	  }
	  else 
	  {
          g_error(_("Could not parse geometry string `%s'"), geometry);
	  }
	  }
	*/

	/* Append ourself to the list of windows */
	app_list = g_slist_prepend (app_list, win);

	return GTK_WIDGET(win);
}

#if 0
/*** gnomehello-popttable */
static char* message  = NULL;
static char* geometry = NULL;

struct poptOption options[] = {
  { 
    "message",
    'm',
    POPT_ARG_STRING,
    &message,
    0,
    N_("Specify a message other than \"Hello, World!\""),
    N_("MESSAGE")
  },
  { 
    "geometry",
    '\0',
    POPT_ARG_STRING,
    &geometry,
    0,
    N_("Specify the geometry of the main window"),
    N_("GEOMETRY")
  },
  {
    NULL,
    '\0',
    0,
    NULL,
    0,
    NULL,
    NULL
  }
};
/* gnomehello-popttable ***/
#endif

int 
main(int argc, char* argv[])
{
	/*** gnomehello-parsing */
	GtkWidget* app;

	//GnomeClient* client;
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);  
	textdomain(PACKAGE);

	if (!bonobo_ui_init_full ("BonoboUI-Hello", VERSION, &argc, argv, NULL, NULL, NULL, TRUE))
		g_error (_("Cannot init libbonoboui code"));

#if 0
	/* Argument parsing */
	//args = poptGetArgs(pctx);
	//poptFreeContext(pctx);

	/* gnomehello-parsing ***/

	/* Session Management */
  
	/*** gnomehello-client */
/*
  client = gnome_master_client ();
  gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
  GTK_SIGNAL_FUNC (save_session), argv[0]);
  gtk_signal_connect (GTK_OBJECT (client), "die",
  GTK_SIGNAL_FUNC (session_die), NULL);
*/
	/* gnomehello-client ***/

  
	/* Main app */
#endif

	app = hello_new (NULL, NULL); /* message, geometry);*/

	gtk_widget_show_all (GTK_WIDGET(app));

	bonobo_main ();

	return 0;
}

#if 0
/*** gnomehello-save-session */

static gint
save_session (GnomeClient *client, gint phase, GnomeSaveStyle save_style,
              gint is_shutdown, GnomeInteractStyle interact_style,
              gint is_fast, gpointer client_data)
{
  gchar** argv;
  guint argc;

  /* allocate 0-filled, so it will be NULL-terminated */
  argv = g_malloc0(sizeof(gchar*)*4);
  argc = 1;

  argv[0] = client_data;

  if (message)
    {
      argv[1] = "--message";
      argv[2] = message;
      argc = 3;
    }
  
  gnome_client_set_clone_command (client, argc, argv);
  gnome_client_set_restart_command (client, argc, argv);

  return TRUE;
}

/* gnomehello-save-session ***/

/*** gnomehello-session-die */

static void
session_die(GnomeClient* client, gpointer client_data)
{
  gtk_main_quit ();
}

/* gnomehello-session-die ***/
#endif
