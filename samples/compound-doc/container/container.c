/* $Id */
/*
  Bonobo-Sample Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

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


static void sample_app_create ();
static void delete_cb (GtkWidget *caller, SampleApp *inst);

static void create_component_frame (SampleApp *inst, Component *component, gchar *name);
static BonoboObjectClient* launch_component (BonoboClientSite *client_site,
					     BonoboContainer *container,
					     gchar *component_id);

static void
sample_app_create ()
{   
    SampleApp *inst = g_new0 (SampleApp, 1);
    GtkWidget *app, *box;
    
    /* Create widgets */
    app = inst->app = gnome_app_new ("container",
				     "Sample Bonobo container");
    box = inst->box = gtk_vbox_new (FALSE, 10);
    
    gtk_signal_connect (GTK_OBJECT (app), "destroy",
			delete_cb, inst);
    
    /* Do the packing stuff */
    gnome_app_set_contents (GNOME_APP (app), box);
    gtk_widget_set_usize (app, 400, 400);
    
    inst->container = bonobo_container_new ();
    inst->ui_handler = bonobo_ui_handler_new ();
    bonobo_ui_handler_set_app (inst->ui_handler, GNOME_APP (app));
    /* Create menu bar */
    bonobo_ui_handler_create_menubar (inst->ui_handler);
    sample_app_fill_menu (inst);

    gtk_widget_show_all (app);
}

Component*
sample_app_add_component (SampleApp *inst, gchar *obj_id)
{
    Component *component;
    BonoboClientSite *client_site;
    BonoboObjectClient *server;
    /*
     * The ClientSite is the container-side point of contact for
     * the Embeddable.  So there is a one-to-one correspondence
     * between BonoboClientSites and BonoboEmbeddables.
     */
    client_site = bonobo_client_site_new (inst->container);
    
    /*
     * A BonoboObjectClient is a simple wrapper for a remote
     * BonoboObject (a server supporting Bonobo::Unknown).
     */
    server = launch_component (client_site, inst->container,
			       obj_id);

    if (!server)
    {
	gchar *error_msg;
	
	error_msg = g_strdup_printf (_("Could not launch Embeddable %s!"),
				     obj_id);
	gnome_warning_dialog (error_msg);
	g_free (error_msg);
	
	return NULL;
    }
    
    /*
     * Create the internal data structure which we will use to
     * keep track of this component.
     */
    component = g_new0 (Component, 1);
    component->container = inst;
    component->client_site = client_site;
    component->server = server;
    inst->components = g_list_append (inst->components, component);

    /*
     * Now we have a BonoboEmbeddable bound to our local
     * ClientSite.  Here we create a little on-screen box to store
     * the embeddable in, when the user adds views for it.
     */
    create_component_frame (inst, component, obj_id);
    return component;
}

void
sample_app_remove_component (SampleApp *inst,
			    Component *component)
{
    inst->components = g_list_remove (inst->components, component);
    
    /* Destroy the container widget */
    gtk_container_remove (GTK_CONTAINER (component->container->box),
			  component->widget);
}

static BonoboObjectClient *
launch_component (BonoboClientSite *client_site,
		  BonoboContainer *container,
		  gchar *component_id)
{
    BonoboObjectClient *object_server;
    
    /*
     * Launch the component.
     */
    object_server = bonobo_object_activate_with_oaf_id (
	component_id, 0);
    
    if (!object_server)
	return NULL;
    
    /*
     * Bind it to the local ClientSite.  Every embedded component
     * has a local BonoboClientSite object which serves as a
     * container-side point of contact for the embeddable.  The
     * container talks to the embeddable through its ClientSite
     */
    if (!bonobo_client_site_bind_embeddable (client_site, object_server))
    {
	bonobo_object_unref (BONOBO_OBJECT (object_server));
	return NULL;
    }
    
    /*
     * The BonoboContainer object maintains a list of the
     * ClientSites which it manages.  Here we add the new
     * ClientSite to that list.
     */
    bonobo_container_add (container, BONOBO_OBJECT (client_site));
    
    return object_server;
}

static void
create_component_frame (SampleApp *inst,
			Component *component,
			gchar *name)
{
    GtkWidget *widget = component_create_frame (component, name);

    gtk_widget_show_all (widget);
    gtk_box_pack_start (GTK_BOX (inst->box), widget, FALSE, FALSE, 0);
}

void
sample_app_exit (SampleApp *app)
{
    bonobo_object_destroy (BONOBO_OBJECT (app->container));
    gtk_main_quit ();
}

static void
delete_cb (GtkWidget *caller, SampleApp *inst)
{
    sample_app_exit (inst);
}

int
main (int argc, char **argv)
{
    CORBA_Environment ev;
    CORBA_ORB orb;
    
    CORBA_exception_init (&ev);
#if USING_OAF
    gnome_init_with_popt_table ("container", VERSION,
				argc, argv,
				oaf_popt_options, 0, NULL);
    
    orb = oaf_init (argc, argv);
#else
    gnome_CORBA_init ("container", VERSION, &argc, argv, 0, &ev);

    CORBA_exception_free (&ev);
    orb = gnome_CORBA_ORB ();
#endif
    
    
    if (bonobo_init (orb, NULL, NULL) == FALSE)
	g_error (_("Could not initialize Bonobo!\n"));
    
    sample_app_create ();
    
    bonobo_main ();
    
    return 0;
}
