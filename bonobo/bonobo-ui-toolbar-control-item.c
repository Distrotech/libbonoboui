/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * bonobo-ui-toolbar-control-item.c: A special toolbar item for controls.
 *
 * Author:
 *	Jon K Hellan (hellan@acm.org)
 *
 * Copyright 2000 Jon K Hellan.
 */

#include "config.h"
#include <gnome.h>
#include "bonobo-ui-toolbar-control-item.h"

static BonoboUIToolbarItemControlClass *parent_class = NULL;

struct _BonoboUIToolbarItemControlPrivate {
        /* The wrapped control */
        BonoboWidget *control;

        /* The eventbox which makes tooltips work */
        GtkWidget *eventbox;
};

/* GtkWidget methods.  */
static void
impl_set_tooltip (BonoboUIToolbarItem     *item,
                  GtkTooltips             *tooltips,
                  const char              *tooltip)
{
	BonoboUIToolbarItemControl *control_item;
	GtkWidget *eventbox;

	control_item = BONOBO_UI_TOOLBAR_CONTROL_ITEM (item);
	eventbox = control_item->priv->eventbox;
	
	if (tooltip && eventbox)
		gtk_tooltips_set_tip (tooltips, eventbox, tooltip, NULL);
}

static void
impl_destroy (GtkObject *object)
{
	g_free (BONOBO_UI_TOOLBAR_CONTROL_ITEM (object)->priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		 (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);	
}

/**
 * FIXME: Do somewhere - handle vertical mode.
 */

/* Gtk+ object initialization.  */

static void
class_init (BonoboUIToolbarItemControlClass *klass)
{
        BonoboUIToolbarItemClass *item_class;
	GtkObjectClass *object_class;
	
	item_class = BONOBO_UI_TOOLBAR_ITEM_CLASS (klass);
	object_class = GTK_OBJECT_CLASS (klass);

        item_class->set_tooltip = impl_set_tooltip;
	object_class->destroy = impl_destroy;

        parent_class = gtk_type_class (bonobo_ui_toolbar_item_get_type ());
}


static void
init (BonoboUIToolbarItemControl *control_item)
{
        control_item->priv = g_new0 (BonoboUIToolbarItemControlPrivate, 1);
}

GtkType
bonobo_ui_toolbar_control_item_get_type (void)
{
        static GtkType type = 0;

        if (type == 0) {
                static const GtkTypeInfo info = {
                        "BonoboUIToolbarItemControl",
                        sizeof (BonoboUIToolbarItemControl),
                        sizeof (BonoboUIToolbarItemControlClass),
                        (GtkClassInitFunc) class_init,
                        (GtkObjectInitFunc) init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL,
                };

                type = gtk_type_unique (bonobo_ui_toolbar_item_get_type (), &info);
        }

        return type;
}

static GtkWidget *
bonobo_ui_toolbar_control_item_construct (
        BonoboUIToolbarItemControl *control_item,
        Bonobo_Control              control_ref)
{
        BonoboUIToolbarItemControlPrivate *priv = control_item->priv;
	GtkWidget *w  = bonobo_widget_new_control_from_objref (
                bonobo_object_dup_ref (control_ref, NULL), CORBA_OBJECT_NIL);

        if (!w)
                return NULL;

	priv->control  = BONOBO_WIDGET (w); 
	gtk_widget_show   (GTK_WIDGET (priv->control));

        priv->eventbox = gtk_event_box_new ();
        gtk_container_add (GTK_CONTAINER (priv->eventbox),
                           GTK_WIDGET    (priv->control));
/*	priv->eventbox = gtk_button_new_with_label ("Kippers");*/
	gtk_widget_show   (priv->eventbox);

        gtk_container_add (GTK_CONTAINER (control_item),
			   priv->eventbox);

        return GTK_WIDGET (control_item);
}

GtkWidget *
bonobo_ui_toolbar_control_item_new (Bonobo_Control control_ref)
{
        BonoboUIToolbarItemControl *control_item;
	GtkWidget *ret;

        control_item = gtk_type_new (
                bonobo_ui_toolbar_control_item_get_type ());

        ret = bonobo_ui_toolbar_control_item_construct (
		control_item, control_ref);

	if (!ret)
		impl_destroy (GTK_OBJECT (control_item));

	return ret;
}
