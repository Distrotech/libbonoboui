/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * bonobo-ui-toolbar-toggle-button-item.h
 *
 * Author:
 *     Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#include <config.h>
#include <stdlib.h>

#include "bonobo-ui-toolbar-toggle-button-item.h"


#define PARENT_TYPE bonobo_ui_toolbar_button_item_get_type ()
static BonoboUIToolbarButtonItemClass *parent_class = NULL;

enum {
	TOGGLED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


/* GtkToggleButton callback.  */

static void
button_widget_toggled_cb (GtkToggleButton *toggle_button,
			  void *data)
{
	BonoboUIToolbarToggleButtonItem *button_item;

	button_item = BONOBO_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (data);

	g_signal_emit (button_item, signals[TOGGLED], 0);
}

static void
impl_set_state (BonoboUIToolbarItem *item,
		const char          *state)
{
	GtkButton *button;
	gboolean   active = atoi (state);

	button = bonobo_ui_toolbar_button_item_get_button_widget (
		BONOBO_UI_TOOLBAR_BUTTON_ITEM (item));

	if (GTK_WIDGET_STATE (GTK_WIDGET (button)) != active)
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (button), active);
}		


/* Gtk+ object initialization.  */

static void
class_init (BonoboUIToolbarToggleButtonItemClass *klass)
{
	GtkObjectClass *object_class;
	BonoboUIToolbarItemClass *item_class = (BonoboUIToolbarItemClass *) klass;

	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (bonobo_ui_toolbar_button_item_get_type ());

	item_class->set_state = impl_set_state;

	signals[TOGGLED] = g_signal_new ("toggled",
					 G_TYPE_FROM_CLASS (object_class),
					 GTK_RUN_FIRST,
					 G_STRUCT_OFFSET (BonoboUIToolbarToggleButtonItemClass, toggled),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE, 0);
}


static void
init (BonoboUIToolbarToggleButtonItem *toolbar_toggle_button_item)
{
	/* Nothing to do here.  */
}


GtkType
bonobo_ui_toolbar_toggle_button_item_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUIToolbarToggleButtonItem",
			sizeof (BonoboUIToolbarToggleButtonItem),
			sizeof (BonoboUIToolbarToggleButtonItemClass),
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

static void
proxy_toggle_click_cb (GtkWidget *button, GtkObject *item)
{
	gboolean active;
	char    *new_state;

	active = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (button));

	new_state = g_strdup_printf ("%d", active);

	g_signal_emit_by_name (item, "state_altered", new_state);

	g_free (new_state);
}

void
bonobo_ui_toolbar_toggle_button_item_construct (BonoboUIToolbarToggleButtonItem *toggle_button_item,
					     GdkPixbuf *icon,
					     const char *label)
{
	GtkWidget *button_widget;

	button_widget = gtk_toggle_button_new ();

	g_signal_connect_object (
		button_widget, "toggled",
		G_CALLBACK (button_widget_toggled_cb),
		toggle_button_item, 0);

	g_signal_connect_object (
		button_widget, "clicked",
		G_CALLBACK (proxy_toggle_click_cb),
		toggle_button_item, 0);

	bonobo_ui_toolbar_button_item_construct (
		BONOBO_UI_TOOLBAR_BUTTON_ITEM (toggle_button_item),
		GTK_BUTTON (button_widget), icon, label);
}

GtkWidget *
bonobo_ui_toolbar_toggle_button_item_new (GdkPixbuf *icon,
				       const char *label)
{
	BonoboUIToolbarToggleButtonItem *toggle_button_item;

	toggle_button_item = g_object_new (
		bonobo_ui_toolbar_toggle_button_item_get_type (), NULL);

	bonobo_ui_toolbar_toggle_button_item_construct (toggle_button_item, icon, label);

	return GTK_WIDGET (toggle_button_item);
}


void
bonobo_ui_toolbar_toggle_button_item_set_active (BonoboUIToolbarToggleButtonItem *item,
					      gboolean active)
{
	GtkButton *button_widget;

	g_return_if_fail (item != NULL);
	g_return_if_fail (BONOBO_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (item));

	button_widget = bonobo_ui_toolbar_button_item_get_button_widget (BONOBO_UI_TOOLBAR_BUTTON_ITEM (item));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button_widget), active);
}

gboolean
bonobo_ui_toolbar_toggle_button_item_get_active (BonoboUIToolbarToggleButtonItem *item)
{
	GtkButton *button_widget;

	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (item), FALSE);

	button_widget = bonobo_ui_toolbar_button_item_get_button_widget (BONOBO_UI_TOOLBAR_BUTTON_ITEM (item));

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button_widget));
}
