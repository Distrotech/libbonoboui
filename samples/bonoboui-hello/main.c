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

  printf("before gnome_init_with_popt_table\n");

  if (!bonobo_ui_init_full ("BonoboUI-Hello", VERSION, &argc, argv, NULL, NULL, NULL, TRUE))
	  g_error (_("Cannot init libbonoboui code"));

  printf("after gnome_init_with_popt_table\n");
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
