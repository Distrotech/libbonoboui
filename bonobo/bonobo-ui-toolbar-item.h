/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar-item.h
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

#ifndef _BONOBO_UI_TOOLBAR_ITEM_H_
#define _BONOBO_UI_TOOLBAR_ITEM_H_

#include <gtk/gtkbin.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define BONOBO_TYPE_UI_TOOLBAR_ITEM			(bonobo_ui_toolbar_item_get_type ())
#define BONOBO_UI_TOOLBAR_ITEM(obj)			(GTK_CHECK_CAST ((obj), BONOBO_TYPE_UI_TOOLBAR_ITEM, BonoboUIToolbarItem))
#define BONOBO_UI_TOOLBAR_ITEM_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), BONOBO_TYPE_UI_TOOLBAR_ITEM, BonoboUIToolbarItemClass))
#define BONOBO_IS_UI_TOOLBAR_ITEM(obj)			(GTK_CHECK_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_ITEM))
#define BONOBO_IS_UI_TOOLBAR_ITEM_CLASS(klass)		(GTK_CHECK_CLASS_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_ITEM))


typedef enum _BonoboUIToolbarItemStyle BonoboUIToolbarItemStyle;
enum _BonoboUIToolbarItemStyle {
	BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL,
	BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL,
	BONOBO_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY,
	BONOBO_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY
};


typedef struct _BonoboUIToolbarItem        BonoboUIToolbarItem;
typedef struct _BonoboUIToolbarItemPrivate BonoboUIToolbarItemPrivate;
typedef struct _BonoboUIToolbarItemClass   BonoboUIToolbarItemClass;

struct _BonoboUIToolbarItem {
	GtkBin parent;

	BonoboUIToolbarItemPrivate *priv;
};

struct _BonoboUIToolbarItemClass {
	GtkBinClass parent_class;

	void (* set_orientation) (BonoboUIToolbarItem *item,
				  GtkOrientation orientation);
	void (* set_style)       (BonoboUIToolbarItem *item,
				  BonoboUIToolbarItemStyle style);
	void (* set_want_label)  (BonoboUIToolbarItem *item,
				  gboolean want_label);
	void (* activate)        (BonoboUIToolbarItem *item);
};


GtkType                 bonobo_ui_toolbar_item_get_type         (void);
GtkWidget              *bonobo_ui_toolbar_item_new              (void);

void                    bonobo_ui_toolbar_item_set_orientation  (BonoboUIToolbarItem      *item,
							      GtkOrientation          orientation);
GtkOrientation          bonobo_ui_toolbar_item_get_orientation  (BonoboUIToolbarItem      *item);

void                    bonobo_ui_toolbar_item_set_style        (BonoboUIToolbarItem      *item,
							      BonoboUIToolbarItemStyle  style);
BonoboUIToolbarItemStyle  bonobo_ui_toolbar_item_get_style        (BonoboUIToolbarItem      *item);

/* FIXME ugly names.  */
void                    bonobo_ui_toolbar_item_set_want_label   (BonoboUIToolbarItem      *button_item,
							      gboolean                prefer_text);
gboolean                bonobo_ui_toolbar_item_get_want_label   (BonoboUIToolbarItem      *button_item);
void                    bonobo_ui_toolbar_item_activate         (BonoboUIToolbarItem      *item);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BONOBO_UI_TOOLBAR_ITEM_H__ */
