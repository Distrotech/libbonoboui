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

#ifndef BONOBO_HELLO_VIEW_H
#define BONOBO_HELLO_VIEW_H

#include <bonobo.h>
#include "hello-model.h"

/* View data */
typedef struct {
	BonoboView *view;
	Hello *obj;

	GtkWidget *label;
	GtkWidget *widget;
} HelloView;

BonoboView *hello_view_factory (BonoboEmbeddable * bonobo_object,
				const Bonobo_ViewFrame view_frame,
				void *data);
void hello_view_refresh (HelloView * view);


#endif
