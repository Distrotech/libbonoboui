/* $Id */
/*
  Sample-Container Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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

#ifndef SAMPLE_COMPONENT_H
#define SAMPLE_COMPONENT_H

#include <gnome.h>
#include <bonobo.h>

#include "container.h"

struct _Component
{
    SampleApp	  *container;

    BonoboClientSite   *client_site;
    BonoboObjectClient *server;
    gchar              *goad_id;
    
    GtkWidget	  *widget;
    GtkWidget	  *views_hbox;
    GList         *views;
};

void component_add_view (Component *component);
void component_del_view (Component *component);
void component_del (Component *component);
void component_load (Component *component, Bonobo_Stream stream);
void component_save (Component *component, Bonobo_Stream stream);
void component_save_id (Component *component, Bonobo_Stream stream);

void component_print (Component *component, GnomePrintContext *ctx,
		      gdouble x, gdouble y,
		      gdouble width, gdouble height);

GtkWidget *component_create_frame (Component *component, gchar *goad_id);

#endif
