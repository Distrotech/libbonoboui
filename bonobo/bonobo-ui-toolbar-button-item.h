/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar-button-item.h
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

#ifndef _BONOBO_UI_TOOLBAR_BUTTON_ITEM_H_
#define _BONOBO_UI_TOOLBAR_BUTTON_ITEM_H_

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "bonobo-ui-toolbar-item.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define BONOBO_TYPE_UI_TOOLBAR_BUTTON_ITEM		(bonobo_ui_toolbar_button_item_get_type ())
#define BONOBO_UI_TOOLBAR_BUTTON_ITEM(obj)		(GTK_CHECK_CAST ((obj), BONOBO_TYPE_UI_TOOLBAR_BUTTON_ITEM, BonoboUIToolbarButtonItem))
#define BONOBO_UI_TOOLBAR_BUTTON_ITEM_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BONOBO_TYPE_UI_TOOLBAR_BUTTON_ITEM, BonoboUIToolbarButtonItemClass))
#define BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM(obj)		(GTK_CHECK_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_BUTTON_ITEM))
#define BONOBO_IS_UI_TOOLBAR_BUTTON_ITEM_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_BUTTON_ITEM))


typedef struct _BonoboUIToolbarButtonItem        BonoboUIToolbarButtonItem;
typedef struct _BonoboUIToolbarButtonItemPrivate BonoboUIToolbarButtonItemPrivate;
typedef struct _BonoboUIToolbarButtonItemClass   BonoboUIToolbarButtonItemClass;

struct _BonoboUIToolbarButtonItem {
	BonoboUIToolbarItem parent;

	BonoboUIToolbarButtonItemPrivate *priv;
};

struct _BonoboUIToolbarButtonItemClass {
	BonoboUIToolbarItemClass parent_class;

	/* Signals.  */
	void (* clicked)	(BonoboUIToolbarButtonItem *toolbar_button_item);
	void (* set_want_label) (BonoboUIToolbarButtonItem *toolbar_button_item);
};


GtkType    bonobo_ui_toolbar_button_item_get_type           (void);
void       bonobo_ui_toolbar_button_item_construct          (BonoboUIToolbarButtonItem *item,
							     GtkButton                 *button_widget,
							     GdkPixbuf                 *icon,
							     const char                *label);
GtkWidget *bonobo_ui_toolbar_button_item_new                (GdkPixbuf                 *icon,
							     const char                *label);

void       bonobo_ui_toolbar_button_item_set_icon           (BonoboUIToolbarButtonItem *button_item,
							     GdkPixbuf                 *icon);
void       bonobo_ui_toolbar_button_item_set_label          (BonoboUIToolbarButtonItem *button_item,
							     const char                *label);

GtkButton *bonobo_ui_toolbar_button_item_get_button_widget  (BonoboUIToolbarButtonItem *button_item);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BONOBO_UI_TOOLBAR_BUTTON_ITEM_H_ */