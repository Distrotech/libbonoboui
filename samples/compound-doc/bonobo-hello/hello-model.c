/* $Id$ */
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

#include "hello-model.h"
#include "hello-view.h"

static void hello_model_refresh_views (Hello * obj);

/* Initializer */
void
hello_model_init (Hello * obj, BonoboEmbeddable * bonobo_object)
{
	obj->bonobo_object = bonobo_object;
	obj->text = NULL;
	hello_model_set_text (obj, "Hello!");
}

/* Destructor of the object */
void
hello_model_destroy (Hello * obj)
{
	if (!obj)
		return;

/*    hello_model_clear (obj); */
}

void
hello_model_clear (Hello * obj)
{
	if (obj->text)
		g_free (obj->text);
	obj->text = NULL;
}

static void
hello_model_update_view (BonoboView * view, gpointer user_data)
{
	HelloView *view_data;

	view_data =
	    (HelloView *) gtk_object_get_data (GTK_OBJECT (view),
					       "view_data");;
	hello_view_refresh (view_data);
}

/* Iterate thru views and refresh them */
static void
hello_model_refresh_views (Hello * obj)
{
	bonobo_embeddable_foreach_view (obj->bonobo_object,
					hello_model_update_view, obj);
}

/* Change object data */
void
hello_model_set_text (Hello * obj, gchar * text)
{
	hello_model_clear (obj);
	obj->text = g_strdup (text);
	hello_model_refresh_views (obj);
}
