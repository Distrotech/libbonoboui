/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar-separator-item.c
 *
 * Copyright (C) 2000  Helix Code, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ettore Perazzoli
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include "bonobo-ui-toolbar-separator-item.h"


#define PARENT_TYPE bonobo_ui_toolbar_item_get_type ()
static BonoboUIToolbarItemClass *parent_class = NULL;


#define BORDER_WIDTH 2


/* GtkWidget methods.  */

static void
impl_size_request (GtkWidget *widget,
		   GtkRequisition *requisition)
{
	int border_width;

	border_width = GTK_CONTAINER (widget)->border_width;

	requisition->width  = 2 * border_width + widget->style->klass->xthickness;
	requisition->height = 2 * border_width + widget->style->klass->ythickness;
}

static void
impl_draw (GtkWidget *widget,
	   GdkRectangle *area)
{
	BonoboUIToolbarItem *item;
	const GtkAllocation *allocation;
	GtkOrientation orientation;
	int border_width;

	item = BONOBO_UI_TOOLBAR_ITEM (widget);

	allocation = &widget->allocation;
	border_width = GTK_CONTAINER (widget)->border_width;

	orientation = bonobo_ui_toolbar_item_get_orientation (item);

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_paint_vline (widget->style, widget->window,
				 GTK_WIDGET_STATE (widget), area, widget,
				 "toolbar",
				 allocation->y + border_width,
				 allocation->y + allocation->height - border_width - 1,
				 allocation->x + border_width);
	else
		gtk_paint_hline (widget->style, widget->window,
				 GTK_WIDGET_STATE (widget), area, widget,
				 "toolbar",
				 allocation->x + border_width,
				 allocation->x + allocation->width - border_width - 1,
				 allocation->y + border_width);
}

static int
impl_expose_event (GtkWidget *widget,
		   GdkEventExpose *expose)
{
	gtk_widget_draw (widget, &expose->area);

	return FALSE;
}


static void
class_init (BonoboUIToolbarSeparatorItemClass *separator_item_class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (separator_item_class);
	widget_class->size_request = impl_size_request;
	widget_class->draw         = impl_draw;
	widget_class->expose_event = impl_expose_event;

	parent_class = gtk_type_class (bonobo_ui_toolbar_item_get_type ());
}

static void
init (BonoboUIToolbarSeparatorItem *toolbar_separator_item)
{
	gtk_container_set_border_width (GTK_CONTAINER (toolbar_separator_item), BORDER_WIDTH);
}


GtkType
bonobo_ui_toolbar_separator_item_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUIToolbarSeparatorItem",
			sizeof (BonoboUIToolbarSeparatorItem),
			sizeof (BonoboUIToolbarSeparatorItemClass),
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (PARENT_TYPE, &info);
	}

	return type;
}


GtkWidget *
bonobo_ui_toolbar_separator_item_new (void)
{
	BonoboUIToolbarSeparatorItem *new;

	new = gtk_type_new (bonobo_ui_toolbar_separator_item_get_type ());

	return GTK_WIDGET (new);
}
