/* $Id */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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

#include "hello-view.h"
#include <libgnomeui/libgnomeui.h>

static GtkWidget *view_new (HelloView * view_data);
static void view_delete (BonoboView * view, HelloView * view_data);
static void view_activate_cb (BonoboView * view, gboolean activate,
			      gpointer data);
static void view_clicked_cb (GtkWidget * caller, gpointer data);
static void view_change_string_cb (gchar * string, gpointer data);


/*
 * Creates the GTK part of a new View for the data passed in its
 * argument. Does not include the Bonobo part of view creation
 */
static GtkWidget *
view_new (HelloView * view_data)
{
	GtkWidget *w;
	GtkWidget *button, *label, *vbox;

	view_data->label = label = gtk_label_new ("");

	button = gtk_button_new_with_label ("Change text");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (view_clicked_cb), view_data);

	vbox = gtk_vbox_new (FALSE, 10);

	gtk_container_add (GTK_CONTAINER (vbox), label);
	gtk_container_add (GTK_CONTAINER (vbox), button);

	w = vbox;

	hello_view_refresh (view_data);

	gtk_widget_show_all (w);
	return w;
}

/*
 * Refresh a view. Call this whenever the object changes (see
 * hello_model_refresh_views for an example)
 */
void
hello_view_refresh (HelloView * view_data)
{
	Hello *obj = view_data->obj;

	gtk_label_set (GTK_LABEL (view_data->label), obj->text);
}


/*
 * Create the Bonobo part of a new view. The actualy widget creation
 * is done in hello_view_new.
 */
BonoboView *
hello_view_factory (BonoboEmbeddable * bonobo_object,
		    const Bonobo_ViewFrame view_frame, void *data)
{
	BonoboView *view;
	Hello *obj = (Hello *) data;
	HelloView *view_data = g_new (HelloView, 1);

	view_data->obj = obj;
	view_data->widget = view_new (view_data);

	view_data->view = view = bonobo_view_new (view_data->widget);
	bonobo_view_set_view_frame (view, view_frame);

	gtk_object_set_data (GTK_OBJECT (view), "view_data", view_data);

	gtk_signal_connect (GTK_OBJECT (view), "destroy",
			    GTK_SIGNAL_FUNC (view_delete), view_data);

	gtk_signal_connect (GTK_OBJECT (view), "activate",
			    GTK_SIGNAL_FUNC (view_activate_cb), view_data);

	return view;
}

/*
 * View destructor
 */
static void
view_delete (BonoboView * view, HelloView * view_data)
{
	g_free (view_data);
}

/*
 * Signal handler for the `activate' signal
 */
static void
view_activate_cb (BonoboView * view, gboolean activate, gpointer data)
{
	bonobo_view_activate_notify (view, activate);
}

static void
view_clicked_cb (GtkWidget * caller, gpointer data)
{
	HelloView *view_data = (HelloView *) data;
	gchar *curr_text;

	gtk_label_get (GTK_LABEL (view_data->label), &curr_text);

	gnome_request_dialog (FALSE,	/* Password entry */
			      "Enter new text",	/* Prompt */
			      curr_text,	/* Default value */
			      0,	/* Maximum length */
			      view_change_string_cb,	/* Callback */
			      data,	/* Callback data */
			      NULL	/* Parent window */
	    );
}

static void
view_change_string_cb (gchar * string, gpointer data)
{
	HelloView *view_data = (HelloView *) data;

	if (string)
		hello_model_set_text (view_data->obj, string);
}
