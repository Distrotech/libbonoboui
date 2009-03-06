/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * bonobo-ui-toolbar-popup-item.h
 *
 * Author:
 *    Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <bonobo/bonobo-ui-toolbar-popup-item.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (BonoboUIToolbarPopupItem,
	       bonobo_ui_toolbar_popup_item,
	       BONOBO_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM)

#define GET_PRIVATE(inst) (G_TYPE_INSTANCE_GET_PRIVATE (inst, BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM, BonoboUIToolbarPopupItemPrivate))

struct _BonoboUIToolbarPopupItemPrivate
{
  GtkWidget *arrow;
};

/* Utility functions.  */

static void 
set_arrow_orientation (BonoboUIToolbarPopupItem *popup_item)
{
	BonoboUIToolbarPopupItemPrivate *priv = GET_PRIVATE (popup_item);
	GtkOrientation orientation;

	orientation = bonobo_ui_toolbar_item_get_orientation (BONOBO_UI_TOOLBAR_ITEM (popup_item));

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	else
		gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
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
bonobo_ui_toolbar_popup_item_finalize (GObject *object)
{
	BonoboUIToolbarPopupItemPrivate *priv = GET_PRIVATE (object);
	
	g_object_unref (priv->arrow);

	G_OBJECT_CLASS (bonobo_ui_toolbar_popup_item_parent_class)->finalize (object);
}

static void
bonobo_ui_toolbar_popup_item_class_init (
	BonoboUIToolbarPopupItemClass *popup_item_class)
{
	GObjectClass *gobject_class;
	BonoboUIToolbarItemClass *toolbar_item_class;

	gobject_class = G_OBJECT_CLASS (popup_item_class);
	toolbar_item_class = BONOBO_UI_TOOLBAR_ITEM_CLASS (popup_item_class);

	gobject_class->finalize = bonobo_ui_toolbar_popup_item_finalize;

	toolbar_item_class->set_orientation = impl_set_orientation;

	g_type_class_add_private (gobject_class, sizeof (BonoboUIToolbarPopupItemPrivate));
}

static void
bonobo_ui_toolbar_popup_item_init (
	BonoboUIToolbarPopupItem *toolbar_popup_item)
{
	BonoboUIToolbarPopupItemPrivate *priv = GET_PRIVATE (toolbar_popup_item);

	priv->arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	g_object_ref_sink (priv->arrow);
}

void
bonobo_ui_toolbar_popup_item_construct (BonoboUIToolbarPopupItem *popup_item)
{
	BonoboUIToolbarPopupItemPrivate *priv;

	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_POPUP_ITEM (popup_item));

	priv = GET_PRIVATE (popup_item);

	set_arrow_orientation (popup_item);

	/* Note! Despite bonobo_ui_toolbar_toggle_button_item_construct taking a |GdkPixbuf*| as 2nd argument,
	 * it is just passed unchecked to bonobo_ui_toolbar_button_item_construct which also takes a |GdkPixbuf*|,
	 * which is passed unchecked through to bonobo-ui-toolbar-button-item.c:set_image which supports arbitrary
	 * |GtkWidget*|'s. So we can just add a C cast (NOT a GObject cast) to silence the compiler warning.
	 */
	bonobo_ui_toolbar_toggle_button_item_construct (BONOBO_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (popup_item), (GdkPixbuf*) priv->arrow, NULL);
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
