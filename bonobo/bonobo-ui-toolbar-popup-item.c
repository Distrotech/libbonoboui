/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * bonobo-ui-toolbar-popup-item.h
 *
 * Author:
 *    Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#include <config.h>
#include <bonobo/bonobo-ui-toolbar-popup-item.h>
#include <gtk/gtkarrow.h>

G_DEFINE_TYPE (BonoboUIToolbarPopupItem,
	       bonobo_ui_toolbar_popup_item,
	       BONOBO_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM)

static GtkWidget *arrow = NULL;

/* Utility functions.  */

static void 
set_arrow_orientation (BonoboUIToolbarPopupItem *popup_item)
{
	GtkOrientation orientation;

	orientation = bonobo_ui_toolbar_item_get_orientation (BONOBO_UI_TOOLBAR_ITEM (popup_item));

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_arrow_set (GTK_ARROW (arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	else
		gtk_arrow_set (GTK_ARROW (arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
}

static void
impl_set_orientation (BonoboUIToolbarItem *item,
		      GtkOrientation orientation)
{
	BonoboUIToolbarPopupItem *popup_item;

	BONOBO_UI_TOOLBAR_ITEM_CLASS(bonobo_ui_toolbar_popup_item_parent_class)->set_orientation (item, orientation);

	popup_item = BONOBO_UI_TOOLBAR_POPUP_ITEM (item);

	set_arrow_orientation (popup_item);
}

static void
bonobo_ui_toolbar_popup_item_class_init (
	BonoboUIToolbarPopupItemClass *popup_item_class)
{
	BonoboUIToolbarItemClass *toolbar_item_class;

	toolbar_item_class = BONOBO_UI_TOOLBAR_ITEM_CLASS (popup_item_class);
	toolbar_item_class->set_orientation = impl_set_orientation;

	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
}

static void
bonobo_ui_toolbar_popup_item_init (
	BonoboUIToolbarPopupItem *toolbar_popup_item)
{
}

void
bonobo_ui_toolbar_popup_item_construct (BonoboUIToolbarPopupItem *popup_item)
{
	g_return_if_fail (popup_item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_POPUP_ITEM (popup_item));

	set_arrow_orientation (popup_item);

	bonobo_ui_toolbar_toggle_button_item_construct (BONOBO_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (popup_item), arrow, NULL);
}

GtkWidget *
bonobo_ui_toolbar_popup_item_new (void)
{
	BonoboUIToolbarPopupItem *popup_item;

	popup_item = g_object_new (
		bonobo_ui_toolbar_popup_item_get_type (), NULL);

	bonobo_ui_toolbar_popup_item_construct (popup_item);

	return GTK_WIDGET (popup_item);
}
