/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* bonobo-ui-toolbar-popup-item.h
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

#ifndef _BONOBO_UI_TOOLBAR_POPUP_ITEM_H_
#define _BONOBO_UI_TOOLBAR_POPUP_ITEM_H_

#include <libgnome/gnome-defs.h>
#include "bonobo-ui-toolbar-toggle-button-item.h"

BEGIN_GNOME_DECLS

#define BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM            (bonobo_ui_toolbar_popup_item_get_type ())
#define BONOBO_UI_TOOLBAR_POPUP_ITEM(obj)            (GTK_CHECK_CAST ((obj), BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM, BonoboUIToolbarPopupItem))
#define BONOBO_UI_TOOLBAR_POPUP_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM, BonoboUIToolbarPopupItemClass))
#define BONOBO_IS_UI_TOOLBAR_POPUP_ITEM(obj)	     (GTK_CHECK_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM))
#define BONOBO_IS_UI_TOOLBAR_POPUP_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_POPUP_ITEM))


typedef struct _BonoboUIToolbarPopupItem        BonoboUIToolbarPopupItem;
typedef struct _BonoboUIToolbarPopupItemPrivate BonoboUIToolbarPopupItemPrivate;
typedef struct _BonoboUIToolbarPopupItemClass   BonoboUIToolbarPopupItemClass;

struct _BonoboUIToolbarPopupItem {
	BonoboUIToolbarToggleButtonItem parent;
};

struct _BonoboUIToolbarPopupItemClass {
	BonoboUIToolbarToggleButtonItemClass parent_class;
};


GtkType    bonobo_ui_toolbar_popup_item_get_type  (void);
GtkWidget *bonobo_ui_toolbar_popup_item_new       (void);
void       bonobo_ui_toolbar_popup_item_construct (BonoboUIToolbarPopupItem *);

END_GNOME_DECLS

#endif /* _BONOBO_UI_TOOLBAR_POPUP_ITEM_H_ */
