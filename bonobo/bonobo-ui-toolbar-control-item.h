/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
#ifndef _BONOBO_UI_TOOLBAR_CONTROL_ITEM_H_
#define _BONOBO_UI_TOOLBAR_CONTROL_ITEM_H_

#include <libgnome/gnome-defs.h>
#include "bonobo-ui-toolbar-item.h"
#include "bonobo-widget.h"

BEGIN_GNOME_DECLS

#define BONOBO_TYPE_UI_TOOLBAR_CONTROL_ITEM            (bonobo_ui_toolbar_control_item_get_type ())
#define BONOBO_UI_TOOLBAR_CONTROL_ITEM(obj)            (GTK_CHECK_CAST ((obj), BONOBO_TYPE_UI_TOOLBAR_CONTROL_ITEM, BonoboUIToolbarItemControl))
#define BONOBO_UI_TOOLBAR_CONTROL_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BONOBO_TYPE_UI_TOOLBAR_CONTROL_ITEM, BonoboUIToolbarItemControlClass))
#define BONOBO_IS_UI_TOOLBAR_CONTROL_ITEM(obj)         (GTK_CHECK_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_CONTROL_ITEM))
#define BONOBO_IS_UI_TOOLBAR_CONTROL_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((obj), BONOBO_TYPE_UI_TOOLBAR_CONTROL_ITEM))


typedef struct _BonoboUIToolbarItemControl        BonoboUIToolbarItemControl;
typedef struct _BonoboUIToolbarItemControlPrivate BonoboUIToolbarItemControlPrivate;
typedef struct _BonoboUIToolbarItemControlClass   BonoboUIToolbarItemControlClass;

struct _BonoboUIToolbarItemControl {
	BonoboUIToolbarItem parent;

	BonoboUIToolbarItemControlPrivate *priv;
};

struct _BonoboUIToolbarItemControlClass {
	BonoboUIToolbarItemClass parent_class;
};

GtkType       bonobo_ui_toolbar_control_item_get_type (void);
GtkWidget    *bonobo_ui_toolbar_control_item_new      (Bonobo_Control control_ref);

END_GNOME_DECLS

#endif /* _BONOBO_UI_TOOLBAR_CONTROL_ITEM_H_ */
