/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar-button-item.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "bonobo-ui-toolbar-icon.h"

#include "bonobo-ui-toolbar-button-item.h"


/* Spacing between the icon and the label.  */
#define SPACING 2

#define PARENT_TYPE bonobo_ui_toolbar_item_get_type ()
static BonoboUIToolbarItemClass *parent_class = NULL;

struct _BonoboUIToolbarButtonItemPrivate {
	/* The icon for the button.  */
	GtkWidget *icon;

	/* The label for the button.  */
	GtkWidget *label;

	/* The box that packs the icon and the label.  It can either be an hbox
           or a vbox, depending on the style.  */
	GtkWidget *box;

	/* The widget containing the button */
	GtkButton *button_widget;
};

enum {
	CLICKED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };


/* Utility functions.  */

static GtkWidget *
create_pixmap_widget_from_pixbuf (GdkPixbuf *pixbuf)
{
	GtkWidget *pixmap_widget;

	pixmap_widget = bonobo_ui_toolbar_icon_new_from_pixbuf (pixbuf);
	bonobo_ui_toolbar_icon_set_draw_mode (BONOBO_UI_TOOLBAR_ICON (pixmap_widget),
					      BONOBO_UI_TOOLBAR_ICON_COLOR);

	return pixmap_widget;
}

static void
set_icon (BonoboUIToolbarButtonItem *button_item,
	  GdkPixbuf *pixbuf)
{
	BonoboUIToolbarButtonItemPrivate *priv;

	priv = button_item->priv;

	if (priv->icon != NULL)
		gtk_widget_destroy (priv->icon);

	if (pixbuf != NULL)
		priv->icon = create_pixmap_widget_from_pixbuf (pixbuf);
	else
		priv->icon = NULL;
}

static void
set_label (BonoboUIToolbarButtonItem *button_item,
	   const char *label)
{
	BonoboUIToolbarButtonItemPrivate *priv;

	priv = button_item->priv;

	if (priv->label != NULL)
		gtk_widget_destroy (priv->label);

	if (label != NULL)
		priv->label = gtk_label_new (label);
	else
		priv->label = NULL;
}


/* Layout.  */

static void
layout_pixmap_and_label (BonoboUIToolbarButtonItem *button_item,
			 BonoboUIToolbarItemStyle style)
{
	BonoboUIToolbarButtonItemPrivate *priv;
	GtkWidget *button;

	priv = button_item->priv;

	if (priv->icon != NULL) {
		gtk_widget_ref (priv->icon);
		if (priv->icon->parent != NULL)
			gtk_container_remove (GTK_CONTAINER (priv->icon->parent),
					      priv->icon);
	}
	if (priv->label != NULL) {
		gtk_widget_ref (priv->label);
		if (priv->label->parent != NULL)
			gtk_container_remove (GTK_CONTAINER (priv->label->parent),
					      priv->label);
	}

	if (priv->box != NULL)
		gtk_widget_destroy (priv->box);

	if (style == BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL)
		priv->box = gtk_vbox_new (FALSE, SPACING);
	else
		priv->box = gtk_hbox_new (FALSE, SPACING);

	button = GTK_BIN (button_item)->child;
	gtk_container_add (GTK_CONTAINER (button), priv->box);

	gtk_widget_show (priv->box);

	if (priv->icon != NULL) {
		gtk_box_pack_start (GTK_BOX (priv->box), priv->icon, TRUE, TRUE, 0);
		gtk_widget_unref (priv->icon);
		if (style == BONOBO_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY)
			gtk_widget_hide (priv->icon);
		else
			gtk_widget_show (priv->icon);
	}

	if (priv->label != NULL) {
		gtk_box_pack_start (GTK_BOX (priv->box), priv->label, FALSE, TRUE, 0);
		gtk_widget_unref (priv->label);
		if (style == BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY)
			gtk_widget_hide (priv->label);
		else
			gtk_widget_show (priv->label);
	}
}


/* Callback for the GtkButton.  */

static void
button_widget_clicked_cb (GtkButton *button,
			  void *data)
{
	BonoboUIToolbarButtonItem *button_item;

	button_item = BONOBO_UI_TOOLBAR_BUTTON_ITEM (data);

	gtk_signal_emit (GTK_OBJECT (button_item), signals[CLICKED]);

	bonobo_ui_toolbar_item_activate (BONOBO_UI_TOOLBAR_ITEM (button_item));
}


/* GtkObject methods.  */

static void
impl_destroy (GtkObject *object)
{
	BonoboUIToolbarButtonItem *button_item;
	BonoboUIToolbarButtonItemPrivate *priv;

	button_item = BONOBO_UI_TOOLBAR_BUTTON_ITEM (object);
	priv = button_item->priv;

	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* BonoboUIToolbarItem signals.  */

static void
impl_set_style (BonoboUIToolbarItem *item,
		BonoboUIToolbarItemStyle style)
{
	BonoboUIToolbarButtonItem *button_item;

	button_item = BONOBO_UI_TOOLBAR_BUTTON_ITEM (item);

	layout_pixmap_and_label (button_item, style);

	if (BONOBO_UI_TOOLBAR_ITEM_CLASS (parent_class)->set_style != NULL)
		(* BONOBO_UI_TOOLBAR_ITEM_CLASS (parent_class)->set_style) (item, style);
}

static void
impl_set_tooltip (BonoboUIToolbarItem *item,
		  GtkTooltips         *tooltips,
		  const char          *tooltip)
{
	BonoboUIToolbarButtonItem *button_item;
	GtkButton *button;

	button_item = BONOBO_UI_TOOLBAR_BUTTON_ITEM (item);

	if (tooltip && (button = button_item->priv->button_widget))
		gtk_tooltips_set_tip (
			tooltips, GTK_WIDGET (button), tooltip, NULL);
}


/* GTK+ object initialization.  */

static void
class_init (BonoboUIToolbarButtonItemClass *button_item_class)
{
	GtkObjectClass *object_class;
	BonoboUIToolbarItemClass *item_class;

	object_class = GTK_OBJECT_CLASS (button_item_class);
	object_class->destroy = impl_destroy;

	item_class = BONOBO_UI_TOOLBAR_ITEM_CLASS (button_item_class);
	item_class->set_style = impl_set_style;
	item_class->set_tooltip = impl_set_tooltip;

	parent_class = gtk_type_class (bonobo_ui_toolbar_item_get_type ());

	signals[CLICKED] = 
		gtk_signal_new ("clicked",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIToolbarButtonItemClass, clicked),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
init (BonoboUIToolbarButtonItem *toolbar_button_item)
{
	BonoboUIToolbarButtonItemPrivate *priv;

	priv = g_new (BonoboUIToolbarButtonItemPrivate, 1);
	priv->icon  = NULL;
	priv->label = NULL;
	priv->box   = NULL;

	toolbar_button_item->priv = priv;
}


GtkType
bonobo_ui_toolbar_button_item_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUIToolbarButtonItem",
			sizeof (BonoboUIToolbarButtonItem),
			sizeof (BonoboUIToolbarButtonItemClass),
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
bonobo_ui_toolbar_button_item_construct (BonoboUIToolbarButtonItem *button_item,
					 GtkButton *button_widget,
					 GdkPixbuf *pixbuf,
					 const char *label)
{
	BonoboUIToolbarButtonItemPrivate *priv;

	g_return_if_fail (button_item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));
	g_return_if_fail (button_widget != NULL);
	g_return_if_fail (GTK_IS_BUTTON (button_widget));
	g_return_if_fail (GTK_BIN (button_item)->child == NULL);

	priv = button_item->priv;
	g_assert (priv->icon == NULL);
	g_assert (priv->label == NULL);

	priv->button_widget = button_widget;
	gtk_widget_show (GTK_WIDGET (button_widget));

	gtk_signal_connect_while_alive (GTK_OBJECT (button_widget), "clicked",
					GTK_SIGNAL_FUNC (button_widget_clicked_cb), button_item,
					GTK_OBJECT (button_item));

	gtk_button_set_relief (button_widget, GTK_RELIEF_NONE);

	GTK_WIDGET_UNSET_FLAGS (button_widget, GTK_CAN_FOCUS);

	gtk_container_add (GTK_CONTAINER (button_item), GTK_WIDGET (button_widget));

	set_icon  (button_item, pixbuf);
	set_label (button_item, label);

	layout_pixmap_and_label (button_item, bonobo_ui_toolbar_item_get_style (BONOBO_UI_TOOLBAR_ITEM (button_item)));
}

/**
 * bonobo_ui_toolbar_button_item_new:
 * @pixmap: 
 * @label: 
 * 
 * Create a new toolbar button item.
 * 
 * Return value: A pointer to the newly created widget.
 **/
GtkWidget *
bonobo_ui_toolbar_button_item_new (GdkPixbuf *icon,
				   const char *label)
{
	BonoboUIToolbarButtonItem *button_item;
	GtkWidget *button_widget;

	button_item = gtk_type_new (bonobo_ui_toolbar_button_item_get_type ());

	button_widget = gtk_button_new ();
	bonobo_ui_toolbar_button_item_construct (button_item, GTK_BUTTON (button_widget), icon, label);

	return GTK_WIDGET (button_item);
}


void
bonobo_ui_toolbar_button_item_set_icon (BonoboUIToolbarButtonItem *button_item,
					GdkPixbuf *icon)
{
	g_return_if_fail (button_item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));

	set_icon (button_item, icon);
	layout_pixmap_and_label (button_item, bonobo_ui_toolbar_item_get_style (BONOBO_UI_TOOLBAR_ITEM (button_item)));
}

void
bonobo_ui_toolbar_button_item_set_label (BonoboUIToolbarButtonItem *button_item,
				      const char *label)
{
	g_return_if_fail (button_item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));

	set_label (button_item, label);
	layout_pixmap_and_label (button_item, bonobo_ui_toolbar_item_get_style (
		BONOBO_UI_TOOLBAR_ITEM (button_item)));
}


GtkButton *
bonobo_ui_toolbar_button_item_get_button_widget (BonoboUIToolbarButtonItem *button_item)
{
	g_return_val_if_fail (button_item != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM (button_item), NULL);

	return GTK_BUTTON (GTK_BIN (button_item)->child);
}
