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

#ifndef SAMPLE_CONTAINER_H
#define SAMPLE_CONTAINER_H

#include <bonobo.h>
#include <gnome.h>

/* Optional GUI elements (to speed up testing) */
#define GUI 1
#define FILE "/tmp/foo"
#if USING_OAF
#define OBJ_ID "OAFIID:bonobo-sample:" VERSION
#else
#define OBJ_ID "bonobo-object:sample"
#endif

typedef struct _SampleApp SampleApp;
typedef struct _Component Component;

struct _SampleApp {
	BonoboContainer *container;
	BonoboUIHandler *ui_handler;

	BonoboViewFrame *curr_view;
	GList *components;

	GtkWidget *app;
	GtkWidget *box;
	GtkWidget *fileselection;
};


Component *sample_app_add_component (SampleApp * app, gchar * goad_id);
void sample_app_remove_component (SampleApp * app, Component * component);
void sample_app_exit (SampleApp * app);
#endif
