/* app.c
 *
 * Copyright (C) 1999 Havoc Pennington, 2001 Murray Cumming
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

/*** gnomehello-app */

#include "config.h"

#include <stdlib.h>

#include "helloapp.h"

/* Keep a list of all open application windows */
static GSList* app_list = NULL;

static gint delete_event_cb(GtkWidget* w, GdkEventAny* e, gpointer data);

/* Declarations of private functions: */
void helloapp_on_menu_file_new(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void show_nothing_dialog(GtkWidget* parent);
void helloapp_on_menu_file_open(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_file_save(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_file_saveas(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_file_exit(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
static void helloapp_on_menu_file_close(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_edit_undo(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_edit_redo(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_on_menu_help_about(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void helloapp_check_for_ui_xml_file(const gchar* filename);
BonoboWindow* helloapp_create_main_window(void);



void helloapp_on_menu_file_new(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GtkWidget* app;

  app = helloapp_new(_("Hello, World!"), NULL);

  gtk_widget_show_all(app);
}

void show_nothing_dialog(GtkWidget* parent)
{
  GtkWidget* dialog;

  dialog = gtk_message_dialog_new( GTK_WINDOW(parent),
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
             _("This does nothing; it is only a demonstration.") );

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void helloapp_on_menu_file_open(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	show_nothing_dialog(GTK_WIDGET(user_data));
}

void helloapp_on_menu_file_save(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	show_nothing_dialog(GTK_WIDGET(user_data));
}

void helloapp_on_menu_file_saveas(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	show_nothing_dialog(GTK_WIDGET(user_data));
}

void helloapp_on_menu_file_exit(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	exit (0);
}	

static void helloapp_on_menu_file_close(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	BonoboWindow* app;

  app = BONOBO_WINDOW(user_data);

  /* Remove instance: */
  app_list = g_slist_remove(app_list, app);

  /* TODO: Dont' I have to do some bonobo destroy stuff here? */
  gtk_widget_destroy(GTK_WIDGET(app));

  if (app_list == NULL)
  {
    /* No windows remaining */
    helloapp_on_menu_file_exit(uic, user_data, verbname);
  }
}

void helloapp_on_menu_edit_undo(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	show_nothing_dialog(GTK_WIDGET(user_data));
}	

void helloapp_on_menu_edit_redo(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	show_nothing_dialog(GTK_WIDGET(user_data));
}	

void helloapp_on_menu_help_about(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GtkWidget* dialog = 0;

#if 0
  /* We can't use gnome_about_new here because we don't want libbonoboui to depend on libgnomeui. */
  /* We'll use it in the standalone version of this example though. */

  gchar* logo = 0;

  const gchar *authors[] = {
      "Murray Cumming <murrayc@usa.net>",
      NULL
  };

  logo = gnome_program_locate_file(gnome_program_get (), GNOME_FILE_DOMAIN_PIXMAP,
                    "bonoboui-hello-logo.png", TRUE, NULL);



  dialog = gnome_about_new (_("BonoboUI-Hello"), VERSION,
                              "(C) 2001 Murray Cumming, 1999 Havoc Pennington",
                              authors,
                              _("A sample GNOME application, using the Bonobo UI API.."),
                              NULL); //logo);
  g_free(logo);

  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(user_data));

#else
  /* Show a regular dialog instead of a libgnomeui GnomeAbout */

  dialog = gtk_message_dialog_new( GTK_WINDOW(user_data),
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
             _("BonoboUI-Hello.") );
#endif

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
helloapp_on_button_click(GtkWidget* w, gpointer data)
{
  GtkWidget* label;
  gchar* text;
  gchar* tmp;

  label = GTK_WIDGET(data);

  gtk_label_get(GTK_LABEL(label), &text);

  tmp = g_strdup(text);

  g_strreverse(tmp);

  gtk_label_set_text(GTK_LABEL(label), tmp);

  g_free(tmp);
}


void helloapp_check_for_ui_xml_file(const gchar* filename)
{
  gchar* filepath;

  /* We don't actually need to specify HELLO_DATADIR if we use the standard top-level directory
. *  If we use NULL then it looks in <prefix>/share. The tests use NULL. */

  filepath = bonobo_ui_util_get_ui_fname (HELLO_DATADIR, filename);
	if (!filepath || !(g_file_test (filepath, G_FILE_TEST_EXISTS))) //If the file isn't there.
  {
		g_warning ("\nCan't find <prefix>/share/gnome/ui/bonoboui-hello-ui.xml\n"
			   "You need to symlink it like so:\n"
         "e.g. ln -s <downloadpath>/libbonoboui/samples/bonoboui-hello/%s %s/%s/%s\n"
         "A normal application would install the xml file as part of 'make install'.\n",
         filename, HELLO_DATADIR, "gnome/ui", filename);
	}
	g_free (filepath);
}

/* Creates the BonoboWindow, which is then filled with more stuff in helloapp_new(). */
BonoboWindow* helloapp_create_main_window()
{
  BonoboUIContainer* ui_container = 0;
  BonoboUIComponent* ui_component = 0;
  BonoboWindow* win = 0;
  gchar* ui_xml_filename = "bonoboui-hello-ui.xml";

  //Verbs: These verb names (e.g. FileExit) are specified in the UI XML file.
  //If the verb is "", then the verb name is the cmd name.
  BonoboUIVerb verbs [] = { BONOBO_UI_VERB ("FileNew", helloapp_on_menu_file_new),
                            BONOBO_UI_VERB ("FileOpen", helloapp_on_menu_file_open),
                            BONOBO_UI_VERB ("FileSave", helloapp_on_menu_file_save),
                            BONOBO_UI_VERB ("FileSaveAs", helloapp_on_menu_file_saveas),
                            BONOBO_UI_VERB ("FileClose", helloapp_on_menu_file_close),
                            BONOBO_UI_VERB ("FileExit", helloapp_on_menu_file_exit),

                            BONOBO_UI_VERB ("EditUndo", helloapp_on_menu_edit_undo),
                            BONOBO_UI_VERB ("EditRedo", helloapp_on_menu_edit_redo),
                            /* I Haven't bothered with the rest of the Edit menu */

                            BONOBO_UI_VERB ("HelpAbout", helloapp_on_menu_help_about),

	                          BONOBO_UI_VERB_END
                          };


  /* Create Window: */
  win = BONOBO_WINDOW ( bonobo_window_new (PACKAGE, _("Gnome Hello")) );

  /* Create Container: */
  ui_container = bonobo_window_get_ui_container(win);

  /* TODO: What does this do? */
  bonobo_ui_engine_config_set_path ( bonobo_window_get_ui_engine (win),
                                     "/my-application-name/UIConfig/kvps");

  /* Set up Window's UI: */
  ui_component = bonobo_ui_component_new ("my-app");

  /* Associate the BonoboUIComponent with the container */
  bonobo_ui_component_set_container (
          ui_component, BONOBO_OBJREF (ui_container), NULL);

  /* Tell the BonoboUIComponent to use the UI XML file, in <prefix>/share/gnome/ */
  helloapp_check_for_ui_xml_file(ui_xml_filename); /*Tell user to symlink it if necessary. */
  bonobo_ui_util_set_ui (
          ui_component, HELLO_DATADIR,
          ui_xml_filename, "my-app"); /* XML file describing the menus, toolbars, etc */
  /* TODO: I hate that the newbie user has to do this symlinking business just to try out an example. */
  /*       Bonobo should look in the current directory too. */

  /* TODO: */
  /* - How can I add stock menu items without specifying all the details? */
  /* - There are stock pixmaps, but how can I find a list of all the possible pixmap names? */
  /* - Toolbar items tend to share details and callbacks with menu items, but the cmd can't */
  /*     just be reused because toolbar items don't parse the navigation underscore.        */

  /* Link the Verb names (e.g. FileExit) with the callbacks(e.g. helloapp_on_menu_file_exit()). */
  /* Uses the _with_data version, so that the callbacks know which instance (win) they are acting on. */
  bonobo_ui_component_add_verb_list_with_data(ui_component, verbs, win);


  return win;
}

GtkWidget*
helloapp_new(const gchar* message,
              const gchar* geometry)
{
  BonoboWindow* win = 0;
  GtkWidget* button = 0;
  GtkWidget* label = 0;
  GtkWidget* frame = 0;

  /* Create the BonoboWindow, with the menus and toolbars from the UI XML file: */
  win = helloapp_create_main_window();

  /* Add some widgets to the main BonoboWindow: */

  /* Create Button: */
  button = gtk_button_new();
  gtk_container_set_border_width(GTK_CONTAINER(button), 10);

  /* Create Label and put it in the Button: */
  label  = gtk_label_new(message ? message : _("Hello, World!"));
  gtk_container_add(GTK_CONTAINER(button), label);

  /* Connect the Button's 'clicked' signal to the signal handler: */
  gtk_signal_connect(GTK_OBJECT(button),
                     "clicked",
                     GTK_SIGNAL_FUNC(helloapp_on_button_click),
                     label); /* pass label as the data, so that the signal handler can use it. */


  /* TODO: Are there bonobo_window_*() ways to do this? */
  gtk_window_set_policy(GTK_WINDOW(win), FALSE, TRUE, FALSE);
  gtk_window_set_default_size(GTK_WINDOW(win), 250, 350);
  //gtk_window_set_wmclass(GTK_WINDOW(win), "hello", "GnomeHello");

  /* Create Frame and add it to the main BonoboWindow: */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame), button); /* Put the Button in the frame */
  bonobo_window_set_contents(win, frame);

  /* gnomehello-widgets ***/
  
  /*** gnomehello-signals */

  gtk_signal_connect(GTK_OBJECT(win),
                     "delete_event",
                     GTK_SIGNAL_FUNC(delete_event_cb),
                     NULL);


  /* gnomehello-signals ***/

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

  /* gnomehello-geometry ***/

  app_list = g_slist_prepend(app_list, win);

  return GTK_WIDGET(win);
}



/*** gnomehello-quit */
static gint 
delete_event_cb(GtkWidget* window, GdkEventAny* e, gpointer data)
{
  helloapp_on_menu_file_close(NULL, window, NULL);

  /* Prevent the window's destruction, since we destroyed it 
   * ourselves with hello_app_close()
   */
  return TRUE;
}


/* gnomehello-app ***/


/* hello.c
 *
 * Copyright (C) 1999 Havoc Pennington, 2001 Murray Cumming
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

/*** gnomehello */

#include <config.h>
#include <bonobo/bonobo-ui-main.h>


#include "helloapp.h"

/*
static void session_die(GnomeClient* client, gpointer client_data);

static gint save_session(GnomeClient *client, gint phase, 
                         GnomeSaveStyle save_style,
                         gint is_shutdown, GnomeInteractStyle interact_style,
                         gint is_fast, gpointer client_data);
*/

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


	if (bonobo_init (&argc, argv) == FALSE) {
		g_error ("Couldn't initialize Bonobo");
	}


  app = helloapp_new(message, geometry);

  gtk_widget_show_all(GTK_WIDGET(app));



  bonobo_main();

  return 0;
  /* gnomehello-main ***/
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
/* gnomehello ***/
