/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar.c
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

#include "bonobo-ui-toolbar-item.h"
#include "bonobo-ui-toolbar-popup-item.h"

#include "bonobo-ui-toolbar.h"


#define PARENT_TYPE gtk_container_get_type ()
static GtkContainerClass *parent_class = NULL;

enum {
  ARG_0,
  ARG_ORIENTATION,
  ARG_IS_FLOATING,
};
struct _BonoboUIToolbarPrivate {
	/* The orientation of this toolbar.  */
	GtkOrientation orientation;

	/* Is the toolbar currently floating */
	gboolean is_floating;

	/* The style of this toolbar.  */
	BonoboUIToolbarStyle style;

	/* Sizes of the toolbar.  This is actually the height for
           horizontal toolbars and the width for vertical toolbars.  */
	int max_width, max_height;
	int total_width, total_height;

	/* List of all the items in the toolbar.  Both the ones that have been
           unparented because they don't fit, and the ones that are visible.
           The BonoboUIToolbarPopupItem is not here though.  */
	GList *items;

	/* Pointer to the first element in the `items' list that doesn't fit in
           the available space.  This is updated at size_allocate.  */
	GList *first_not_fitting_item;

	/* The pop-up button.  When clicked, it pops up a window with all the
           items that don't fit.  */
	BonoboUIToolbarItem *popup_item;

	/* The window we pop-up when the pop-up item is clicked.  */
	GtkWidget *popup_window;

	/* The vbox within the pop-up window.  */
	GtkWidget *popup_window_vbox;

	/* Whether we have moved items to the pop-up window.  This is to
           prevent the size_allocation code to incorrectly hide the pop-up
           button in that case.  */
	gboolean items_moved_to_popup_window;
};

enum {
	SET_ORIENTATION,
	SET_STYLE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


/* Width of the pop-up window.  */
#define POPUP_WINDOW_WIDTH 200


/* Utility functions.  */

static void
parentize_widget (BonoboUIToolbar *toolbar,
		  GtkWidget *widget)
{
	g_assert (widget->parent == NULL);

	/* The following is done according to the Bible, widget_system.txt, IV, 1.  */

	gtk_widget_set_parent (widget, GTK_WIDGET (toolbar));

	if (GTK_WIDGET_REALIZED (toolbar) && ! GTK_WIDGET_REALIZED (widget))
		gtk_widget_realize (widget);

	if (GTK_WIDGET_MAPPED (toolbar) && ! GTK_WIDGET_MAPPED (widget) &&  GTK_WIDGET_VISIBLE (widget))
		gtk_widget_map (widget);

	if (GTK_WIDGET_MAPPED (widget))
		gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
set_attributes_on_child (BonoboUIToolbarItem *item,
			 GtkOrientation orientation,
			 BonoboUIToolbarStyle style)
{
	bonobo_ui_toolbar_item_set_orientation (item, orientation);

	switch (style) {
	case BONOBO_UI_TOOLBAR_STYLE_PRIORITY_TEXT:
		if (! bonobo_ui_toolbar_item_get_want_label (item))
			bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		else if (orientation == GTK_ORIENTATION_HORIZONTAL)
			bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	case BONOBO_UI_TOOLBAR_STYLE_ICONS_AND_TEXT:
		if (orientation == GTK_ORIENTATION_VERTICAL)
			bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	case BONOBO_UI_TOOLBAR_STYLE_ICONS_ONLY:
		bonobo_ui_toolbar_item_set_style (item, BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		break;
	default:
		g_assert_not_reached ();
	}
}


/* Callbacks to do widget housekeeping.  */

static void
item_destroy_cb (GtkObject *object,
		 void *data)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GtkWidget *widget;

	toolbar = BONOBO_UI_TOOLBAR (data);
	priv = toolbar->priv;

	widget = GTK_WIDGET (object);
	priv->items = g_list_remove (priv->items, object);
}

static void
item_activate_cb (BonoboUIToolbarItem *item,
		  void *data)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;

	toolbar = BONOBO_UI_TOOLBAR (data);
	priv = toolbar->priv;

	bonobo_ui_toolbar_toggle_button_item_set_active (BONOBO_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (priv->popup_item),
						      FALSE);
}

static void
item_set_want_label_cb (BonoboUIToolbarItem *item,
			gboolean want_label,
			void *data)
{
	BonoboUIToolbar *toolbar;

	toolbar = BONOBO_UI_TOOLBAR (data);
	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}


/* The pop-up window foo.  */

/* Return TRUE if there are actually any items in the pop-up menu.  */
static void
create_popup_window (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GtkWidget *hbox;
	GList *p;
	int row_width;

	puts (__FUNCTION__);

	priv = toolbar->priv;

	row_width = 0;
	hbox = NULL;

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next) {
		GtkRequisition item_requisition;
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);

		if (! GTK_WIDGET_VISIBLE (item_widget))
			continue;

		if (item_widget->parent != NULL)
			gtk_container_remove (GTK_CONTAINER (item_widget->parent), item_widget);

		gtk_widget_get_child_requisition (item_widget, &item_requisition);

		set_attributes_on_child (BONOBO_UI_TOOLBAR_ITEM (item_widget),
					 GTK_ORIENTATION_HORIZONTAL,
					 priv->style);

		if (hbox == NULL
		    || (row_width > 0 && item_requisition.width + row_width > POPUP_WINDOW_WIDTH)) {
			hbox = gtk_hbox_new (FALSE, 0);
			gtk_box_pack_start (GTK_BOX (priv->popup_window_vbox), hbox, FALSE, TRUE, 0);
			gtk_widget_show (hbox);
			row_width = 0;
		}

		gtk_box_pack_start (GTK_BOX (hbox), item_widget, FALSE, TRUE, 0);

		row_width += item_requisition.width;
	}
}

static void
hide_popup_window (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	gdk_pointer_ungrab (GDK_CURRENT_TIME);

	gtk_grab_remove (priv->popup_window);
	gtk_widget_hide (priv->popup_window);

	priv->items_moved_to_popup_window = FALSE;

	/* Reset the attributes on all the widgets that were moved to the
           window and move them back to the toolbar.  */
	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar)) {
			set_attributes_on_child (BONOBO_UI_TOOLBAR_ITEM (item_widget),
						 priv->orientation, priv->style);
			gtk_container_remove (GTK_CONTAINER (item_widget->parent), item_widget);
			parentize_widget (toolbar, item_widget);
		}
	}

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
popup_window_button_release_cb (GtkWidget *widget,
				GdkEventButton *event,
				void *data)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;

	toolbar = BONOBO_UI_TOOLBAR (data);
	priv = toolbar->priv;

	bonobo_ui_toolbar_toggle_button_item_set_active (BONOBO_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (priv->popup_item),
						      FALSE);
}

static void
popup_window_map_cb (GtkWidget *widget,
		     void *data)
{
	BonoboUIToolbar *toolbar;

	toolbar = BONOBO_UI_TOOLBAR (data);

	if (gdk_pointer_grab (widget->window, TRUE,
			      (GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK
			       | GDK_LEAVE_NOTIFY_MASK
			       | GDK_POINTER_MOTION_MASK),
			      NULL, NULL, GDK_CURRENT_TIME) != 0) {
		g_warning ("Toolbar pop-up pointer grab failed.");
		return;
	}

	gtk_grab_add (widget);
}

static void
show_popup_window (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	const GtkAllocation *toolbar_allocation;
	int x, y;

	priv = toolbar->priv;

	priv->items_moved_to_popup_window = TRUE;

	create_popup_window (toolbar);

	gdk_window_get_origin (GTK_WIDGET (toolbar)->window, &x, &y);

	toolbar_allocation = & GTK_WIDGET (toolbar)->allocation;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		x += toolbar_allocation->x + toolbar_allocation->width;
	else
		y += toolbar_allocation->y + toolbar_allocation->height;

	gtk_widget_set_uposition (GTK_WIDGET (priv->popup_window), x, y);

	gtk_signal_connect (GTK_OBJECT (priv->popup_window), "map",
			    GTK_SIGNAL_FUNC (popup_window_map_cb), toolbar);

	gtk_widget_show (priv->popup_window);
}

static void
popup_item_toggled_cb (BonoboUIToolbarToggleButtonItem *toggle_button_item,
		       void *data)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	gboolean active;

	puts (__FUNCTION__);

	toolbar = BONOBO_UI_TOOLBAR (data);
	priv = toolbar->priv;

	active = bonobo_ui_toolbar_toggle_button_item_get_active (toggle_button_item);

	if (active)
		show_popup_window (toolbar);
	else 
		hide_popup_window (toolbar);
}


/* Layout handling.  */

static int
get_popup_item_size (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GtkRequisition requisition;

	priv = toolbar->priv;

	gtk_widget_get_child_requisition (GTK_WIDGET (priv->popup_item), &requisition);

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		return requisition.width;
	else
		return requisition.height;
}

/* Update the various sizes.  This is performed during ::size_request.  */
static void
update_sizes (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	int max_width, max_height;
	int total_width, total_height;
	GList *p;

	priv = toolbar->priv;

	max_width = max_height = total_width = total_height = 0;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;
		GtkRequisition item_requisition;

		item_widget = GTK_WIDGET (p->data);
		if (! GTK_WIDGET_VISIBLE (item_widget) || item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		gtk_widget_size_request (item_widget, &item_requisition);

		max_width     = MAX (max_width,  item_requisition.width);
		total_width  += item_requisition.width;
		max_height    = MAX (max_height, item_requisition.height);
		total_height += item_requisition.height;
	}

	priv->max_width = max_width;
	priv->total_width = total_width;
	priv->max_height = max_height;
	priv->total_height = total_height;
}

static void
allocate_popup_item (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GtkRequisition popup_item_requisition;
	GtkAllocation popup_item_allocation;
	GtkAllocation *toolbar_allocation;
	int border_width;

	priv = toolbar->priv;

	/* FIXME what if there is not enough space?  */

	toolbar_allocation = & GTK_WIDGET (toolbar)->allocation;

	border_width = GTK_CONTAINER (toolbar)->border_width;

	gtk_widget_get_child_requisition (GTK_WIDGET (priv->popup_item), &popup_item_requisition);

	popup_item_allocation.x = toolbar_allocation->x;
	popup_item_allocation.y = toolbar_allocation->y;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
		popup_item_allocation.x      += toolbar_allocation->width - popup_item_requisition.width - border_width;
		popup_item_allocation.y      += border_width;
		popup_item_allocation.width  = popup_item_requisition.width;
		popup_item_allocation.height = toolbar_allocation->height - 2 * border_width;
	} else {
		popup_item_allocation.x      += border_width;
		popup_item_allocation.y      += toolbar_allocation->height - popup_item_requisition.height - border_width;
		popup_item_allocation.width  = toolbar_allocation->width - 2 * border_width;
		popup_item_allocation.height = popup_item_requisition.height;
	}

	gtk_widget_size_allocate (GTK_WIDGET (priv->popup_item), &popup_item_allocation);
}

static void
setup_popup_item (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	if (priv->items_moved_to_popup_window) {
		gtk_widget_show (GTK_WIDGET (priv->popup_item));
		allocate_popup_item (toolbar);
		return;
	}

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);

		if (GTK_WIDGET_VISIBLE (item_widget)) {
			gtk_widget_show (GTK_WIDGET (priv->popup_item));
			allocate_popup_item (toolbar);
			return;
		}
	}

	gtk_widget_hide (GTK_WIDGET (priv->popup_item));
}

/* This is a dirty hack.  We cannot hide the items with gtk_widget_hide()
   because we want to let the user be in control of the physical hidden/shown
   state, so we just move the widget to a non-visible area.  */
static void
hide_not_fitting_items (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	const GtkAllocation *allocation;
	GtkAllocation child_allocation;
	GList *p;

	priv = toolbar->priv;

	allocation = & GTK_WIDGET (toolbar)->allocation;

	child_allocation.x      = -1;
	child_allocation.y      = -1;
	child_allocation.width  = 1;
	child_allocation.height = 1;

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next)
		gtk_widget_size_allocate (GTK_WIDGET (p->data), &child_allocation);
}

static void
size_allocate_horizontally (BonoboUIToolbar *toolbar,
			    const GtkAllocation *allocation)
{
	BonoboUIToolbarPrivate *priv;
	GtkAllocation child_allocation;
	GList *p;
	int available_width;
	int border_width;

	GTK_WIDGET (toolbar)->allocation = *allocation;

	priv = toolbar->priv;

	border_width = GTK_CONTAINER (toolbar)->border_width;

	available_width = allocation->width - 2 * border_width;
	if (available_width < 0)
		available_width = 0;

	child_allocation.x = allocation->x + border_width;
	child_allocation.y = allocation->y + border_width;

	priv->first_not_fitting_item = NULL;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *widget;
		GtkRequisition child_requisition;

		widget = GTK_WIDGET (p->data);
		if (! GTK_WIDGET_VISIBLE (widget) || widget->parent != GTK_WIDGET (toolbar))
			continue;

		gtk_widget_get_child_requisition (widget, &child_requisition);

		if (available_width < child_requisition.width) {
			priv->first_not_fitting_item = p;
			break;
		}

		child_allocation.width  = child_requisition.width;
		child_allocation.height = priv->max_height;

		gtk_widget_size_allocate (widget, &child_allocation);

		child_allocation.x += child_allocation.width;
		available_width    -= child_allocation.width;
	}

	/* Something did not fit allocate the arrow */
	if (p != NULL) {
		/* Does the arrow fit ? */
		available_width -= get_popup_item_size (toolbar);

		/* Damn, it does not.  Remove the previous element if possible */
		if (available_width < 0 && p->prev != NULL)
			priv->first_not_fitting_item = p->prev;
	}

	hide_not_fitting_items (toolbar);
	setup_popup_item (toolbar);
}

static void
size_allocate_vertically (BonoboUIToolbar *toolbar,
			  const GtkAllocation *allocation)
{
	BonoboUIToolbarPrivate *priv;
	GtkAllocation child_allocation;
	GList *p;
	int available_height;
	int border_width;

	GTK_WIDGET (toolbar)->allocation = *allocation;

	priv = toolbar->priv;

	border_width = GTK_CONTAINER (toolbar)->border_width;

	available_height = allocation->height - 2 * border_width;
	if (available_height < 0)
		available_height = 0;

	child_allocation.x = allocation->x + border_width;
	child_allocation.y = allocation->y + border_width;

	priv->first_not_fitting_item = NULL;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *widget;
		GtkRequisition child_requisition;

		widget = GTK_WIDGET (p->data);
		if (! GTK_WIDGET_VISIBLE (widget) || widget->parent != GTK_WIDGET (toolbar))
			continue;

		gtk_widget_get_child_requisition (widget, &child_requisition);

		if (available_height < child_requisition.height) {
			priv->first_not_fitting_item = p;
			break;
		}

		child_allocation.width  = priv->max_width;
		child_allocation.height = child_requisition.height;

		gtk_widget_size_allocate (widget, &child_allocation);

		child_allocation.y += child_allocation.height;
		available_height   -= child_allocation.height;
	}

	/* Something did not fit allocate the arrow */
	if (p != NULL) {
		/* Does the arrow fit ? */
		available_height -= get_popup_item_size (toolbar);

		/* Damn, it does not.  Remove the previous element if possible */
		if (available_height < 0 && p->prev != NULL)
			priv->first_not_fitting_item = p->prev;
	}

	hide_not_fitting_items (toolbar);
	setup_popup_item (toolbar);
}


/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GList *p;

	toolbar = BONOBO_UI_TOOLBAR (object);
	priv = toolbar->priv;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent == NULL)
			gtk_widget_destroy (item_widget);
	}

	if (GTK_WIDGET (priv->popup_item)->parent == NULL)
		gtk_widget_destroy (GTK_WIDGET (priv->popup_item));

	if (priv->popup_window != NULL)
		gtk_widget_destroy (priv->popup_window);

	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* GtkWidget methods.  */

static void
impl_size_request (GtkWidget *widget,
		   GtkRequisition *requisition)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	int border_width;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	g_assert (priv->popup_item != NULL);

	update_sizes (toolbar);

	border_width = GTK_CONTAINER (toolbar)->border_width;

	if (priv->is_floating) {
		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			requisition->width  = priv->total_width;
			requisition->height = priv->max_height;
		} else {
			requisition->width  = priv->max_width;
			requisition->height = priv->total_height;
		}
	} else {
		GtkRequisition popup_item_requisition;

		gtk_widget_size_request (GTK_WIDGET (priv->popup_item), &popup_item_requisition);
		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			requisition->width  = popup_item_requisition.width;
			requisition->height = MAX (popup_item_requisition.height, priv->max_height);
		} else {
			requisition->width  = MAX (popup_item_requisition.width, priv->max_width);
			requisition->height = popup_item_requisition.height;
		}
	}

	requisition->width  += 2 * border_width;
	requisition->height += 2 * border_width;
}

static void
impl_size_allocate (GtkWidget *widget,
		    GtkAllocation *allocation)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		size_allocate_horizontally (toolbar, allocation);
	else
		size_allocate_vertically (toolbar, allocation);
}

static void
impl_map (GtkWidget *widget)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GList *p;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	GTK_WIDGET_SET_FLAGS (toolbar, GTK_MAPPED);

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (GTK_WIDGET_VISIBLE (item_widget) && ! GTK_WIDGET_MAPPED (item_widget))
			gtk_widget_map (item_widget);
	}

	if (GTK_WIDGET_VISIBLE (priv->popup_item) && ! GTK_WIDGET_MAPPED (priv->popup_item))
		gtk_widget_map (GTK_WIDGET (priv->popup_item));
}

static void
impl_unmap (GtkWidget *widget)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GList *p;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (GTK_WIDGET_VISIBLE (item_widget) && GTK_WIDGET_MAPPED (item_widget))
			gtk_widget_unmap (item_widget);
	}

	if (GTK_WIDGET_VISIBLE (priv->popup_item) && GTK_WIDGET_MAPPED (priv->popup_item))
		gtk_widget_unmap (GTK_WIDGET (priv->popup_item));
}

static void
impl_draw (GtkWidget *widget,
	   GdkRectangle *area)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GdkRectangle item_area;
	GList *p;

	if (! GTK_WIDGET_DRAWABLE (widget))
		return;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (gtk_widget_intersect (item_widget, area, &item_area))
			gtk_widget_draw (item_widget, &item_area);
	}

	if (gtk_widget_intersect (GTK_WIDGET (priv->popup_item), area, &item_area))
		gtk_widget_draw (GTK_WIDGET (priv->popup_item), &item_area);
}

static int
impl_expose_event (GtkWidget *widget,
		   GdkEventExpose *event)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GdkEventExpose item_event;
	GList *p;

	if (! GTK_WIDGET_DRAWABLE (widget))
		return FALSE;

	toolbar = BONOBO_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	item_event = *event;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (! GTK_WIDGET_NO_WINDOW (item_widget))
			continue;

		if (gtk_widget_intersect (item_widget, &event->area, &item_event.area))
			gtk_widget_event (item_widget, (GdkEvent *) &item_event);
	}

	if (gtk_widget_intersect (GTK_WIDGET (priv->popup_item), &event->area, &item_event.area))
		gtk_widget_event (GTK_WIDGET (priv->popup_item), (GdkEvent *) &item_event);

	return FALSE;
}


/* GtkContainer methods.  */

static void
impl_remove (GtkContainer *container,
	     GtkWidget *child)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;

	toolbar = BONOBO_UI_TOOLBAR (container);
	priv = toolbar->priv;

	gtk_widget_unparent (child);

	if (child == GTK_WIDGET (priv->popup_item))
		priv->popup_item = NULL;

	gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
impl_forall (GtkContainer *container,
	     gboolean include_internals,
	     GtkCallback callback,
	     void *callback_data)
{
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarPrivate *priv;
	GList *p;

	toolbar = BONOBO_UI_TOOLBAR (container);
	priv = toolbar->priv;

	p = priv->items;
	while (p != NULL) {
		GtkWidget *child;
		GList *pnext;

		pnext = p->next;

		child = GTK_WIDGET (p->data);
		if (child->parent == GTK_WIDGET (toolbar))
			(* callback) (child, callback_data);

		p = pnext;
	}
}


/* BonoboUIToolbar signals.  */

static void
impl_set_orientation (BonoboUIToolbar *toolbar,
		      GtkOrientation orientation)
{
	BonoboUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	if (orientation == priv->orientation)
		return;

	priv->orientation = orientation;

	for (p = priv->items; p != NULL; p = p->next) {
		BonoboUIToolbarItem *item;

		item = BONOBO_UI_TOOLBAR_ITEM (p->data);
		set_attributes_on_child (item, orientation, priv->style);
	}

	bonobo_ui_toolbar_item_set_orientation (BONOBO_UI_TOOLBAR_ITEM (priv->popup_item), orientation);

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
impl_set_style (BonoboUIToolbar *toolbar,
		BonoboUIToolbarStyle style)
{
	BonoboUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	if (style == priv->style)
		return;

	priv->style = style;

	for (p = priv->items; p != NULL; p = p->next) {
		BonoboUIToolbarItem *item;

		item = BONOBO_UI_TOOLBAR_ITEM (p->data);
		set_attributes_on_child (item, priv->orientation, style);
	}

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
impl_get_arg(GtkObject* obj, GtkArg* arg, guint arg_id)
{
  BonoboUIToolbar *toolbar = BONOBO_UI_TOOLBAR (obj);
  BonoboUIToolbarPrivate *priv = toolbar->priv;

  switch (arg_id) {
  case ARG_ORIENTATION:
    GTK_VALUE_UINT(*arg) = bonobo_ui_toolbar_get_orientation (toolbar);
    break;

  case ARG_IS_FLOATING :
    GTK_VALUE_BOOL(*arg) = priv->is_floating;
    break;

  default:
    break;
  };
}

static void
impl_set_arg(GtkObject* obj, GtkArg* arg, guint arg_id)
{
  BonoboUIToolbar *toolbar = BONOBO_UI_TOOLBAR (obj);
  BonoboUIToolbarPrivate *priv = toolbar->priv;

  switch (arg_id) {

  case ARG_ORIENTATION:
    bonobo_ui_toolbar_set_orientation (toolbar, GTK_VALUE_UINT(*arg));
    break;

  case ARG_IS_FLOATING :
    priv->is_floating = GTK_VALUE_BOOL(*arg);
    break;

  default:
    break;
  };
}

static void
class_init (BonoboUIToolbarClass *toolbar_class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	object_class = GTK_OBJECT_CLASS (toolbar_class);
	object_class->destroy = impl_destroy;
	object_class->get_arg = impl_get_arg;
	object_class->set_arg = impl_set_arg;

	widget_class = GTK_WIDGET_CLASS (toolbar_class);
	widget_class->size_request  = impl_size_request;
	widget_class->size_allocate = impl_size_allocate;
	widget_class->map           = impl_map;
	widget_class->unmap         = impl_unmap;
	widget_class->draw          = impl_draw;
	widget_class->expose_event  = impl_expose_event;

	container_class = GTK_CONTAINER_CLASS (toolbar_class);
	container_class->remove = impl_remove;
	container_class->forall = impl_forall;

	toolbar_class->set_orientation = impl_set_orientation;
	toolbar_class->set_style       = impl_set_style;

	parent_class = gtk_type_class (gtk_container_get_type ());

	gtk_object_add_arg_type("BonoboUIToolbar::orientation",
				GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_ORIENTATION);
	gtk_object_add_arg_type("BonoboUIToolbar::is_floating",
				GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_IS_FLOATING);

	signals[SET_ORIENTATION]
		= gtk_signal_new ("set_orientation",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIToolbarClass, set_orientation),
				  gtk_marshal_NONE__INT,
				  GTK_TYPE_NONE, 1,
				  GTK_TYPE_INT);

	signals[SET_STYLE]
		= gtk_signal_new ("set_style",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (BonoboUIToolbarClass, set_style),
				  gtk_marshal_NONE__INT,
				  GTK_TYPE_NONE, 1,
				  GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
init (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;

	GTK_WIDGET_SET_FLAGS (toolbar, GTK_NO_WINDOW);
	GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);

	priv = g_new (BonoboUIToolbarPrivate, 1);

	priv->orientation                 = GTK_ORIENTATION_HORIZONTAL;
	priv->is_floating		  = FALSE;
	priv->style                       = BONOBO_UI_TOOLBAR_STYLE_ICONS_AND_TEXT;
	priv->max_width			  = 0;
	priv->total_width		  = 0;
	priv->max_height		  = 0;
	priv->total_height		  = 0;
	priv->popup_item                  = NULL;
	priv->items                       = NULL;
	priv->first_not_fitting_item      = NULL;
	priv->popup_window                = NULL;
	priv->popup_window_vbox           = NULL;
	priv->items_moved_to_popup_window = FALSE;

	toolbar->priv = priv;
}


GtkType
bonobo_ui_toolbar_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUIToolbar",
			sizeof (BonoboUIToolbar),
			sizeof (BonoboUIToolbarClass),
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

void
bonobo_ui_toolbar_construct (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;
	GtkWidget *frame;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar));

	priv = toolbar->priv;

	priv->popup_item = BONOBO_UI_TOOLBAR_ITEM (bonobo_ui_toolbar_popup_item_new ());
	bonobo_ui_toolbar_item_set_orientation (priv->popup_item, priv->orientation);
	parentize_widget (toolbar, GTK_WIDGET (priv->popup_item));

	gtk_signal_connect (GTK_OBJECT (priv->popup_item), "toggled",
			    GTK_SIGNAL_FUNC (popup_item_toggled_cb), toolbar);

	priv->popup_window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_signal_connect (GTK_OBJECT (priv->popup_window), "button_release_event",
			    GTK_SIGNAL_FUNC (popup_window_button_release_cb), toolbar);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (priv->popup_window), frame);

	priv->popup_window_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->popup_window_vbox);
	gtk_container_add (GTK_CONTAINER (frame), priv->popup_window_vbox);
}

GtkWidget *
bonobo_ui_toolbar_new (void)
{
	BonoboUIToolbar *toolbar;

	toolbar = gtk_type_new (bonobo_ui_toolbar_get_type ());

	bonobo_ui_toolbar_construct (toolbar);

	return GTK_WIDGET (toolbar);
}


void
bonobo_ui_toolbar_set_orientation (BonoboUIToolbar *toolbar,
				   GtkOrientation orientation)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar));
	g_return_if_fail (orientation == GTK_ORIENTATION_HORIZONTAL
			  || orientation == GTK_ORIENTATION_VERTICAL);

	gtk_signal_emit (GTK_OBJECT (toolbar), signals[SET_ORIENTATION], orientation);
}

GtkOrientation
bonobo_ui_toolbar_get_orientation (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;

	g_return_val_if_fail (toolbar != NULL, GTK_ORIENTATION_HORIZONTAL);
	g_return_val_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

	priv = toolbar->priv;

	return priv->orientation;
}


void
bonobo_ui_toolbar_set_style (BonoboUIToolbar *toolbar,
			  BonoboUIToolbarStyle style)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar));

	gtk_signal_emit (GTK_OBJECT (toolbar), signals[SET_STYLE], style);
}

BonoboUIToolbarStyle
bonobo_ui_toolbar_get_style (BonoboUIToolbar *toolbar)
{
	BonoboUIToolbarPrivate *priv;

	g_return_val_if_fail (toolbar != NULL, BONOBO_UI_TOOLBAR_STYLE_PRIORITY_TEXT);
	g_return_val_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar), BONOBO_UI_TOOLBAR_STYLE_PRIORITY_TEXT);

	priv = toolbar->priv;

	return priv->style;
}


void
bonobo_ui_toolbar_insert (BonoboUIToolbar *toolbar,
		       BonoboUIToolbarItem *item,
		       int position)
{
	BonoboUIToolbarPrivate *priv;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR (toolbar));
	g_return_if_fail (item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_ITEM (item));

	gtk_object_ref (GTK_OBJECT (item));
	gtk_object_sink (GTK_OBJECT (item));

	priv = toolbar->priv;
	priv->items = g_list_insert (priv->items, item, position);

	gtk_signal_connect_while_alive (GTK_OBJECT (item), "destroy",
					GTK_SIGNAL_FUNC (item_destroy_cb), toolbar,
					GTK_OBJECT (toolbar));
	gtk_signal_connect_while_alive (GTK_OBJECT (item), "activate",
					GTK_SIGNAL_FUNC (item_activate_cb), toolbar,
					GTK_OBJECT (toolbar));
	gtk_signal_connect_while_alive (GTK_OBJECT (item), "set_want_label",
					GTK_SIGNAL_FUNC (item_set_want_label_cb), toolbar,
					GTK_OBJECT (toolbar));

	set_attributes_on_child (item, priv->orientation, priv->style);
	parentize_widget (toolbar, GTK_WIDGET (item));

	g_assert (GTK_WIDGET (item)->parent == GTK_WIDGET (toolbar));

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}
