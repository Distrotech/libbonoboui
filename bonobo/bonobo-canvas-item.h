#ifndef _GNOME_BONOBO_ITEM_H_
#define _GNOME_BONOBO_ITEM_H_

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-canvas.h>
#include <bonobo/gnome-view-frame.h>

#define GNOME_BONOBO_ITEM(obj)          (GTK_CHECK_CAST((obj), gnome_bonobo_item_get_type (), GnomeBonoboItem))
#define GNOME_BONOBO_ITEM_CLASS(k)      (GTK_CHECK_CLASS_CAST ((k), gnome_bonobo_get_type (), GnomeBonoboItemClass))
#define GNOME_IS_BONOBO_ITEM(o)         (GTK_CHECK_TYPE((o), gnome_bonobo_get_type ()))

typedef struct _GnomeBonoboItemPrivate GnomeBonoboItemPrivate;

typedef struct {
	GnomeCanvasItem         canvas_item;
	GnomeViewFrame         *view_frame;
	GnomeBonoboItemPrivate *priv;
} GnomeBonoboItem;

typedef struct {
	GnomeCanvasItemClass parent_class;
} GnomeBonoboItemClass;

GtkType          gnome_bonobo_item_get_type (void);
GnomeCanvasItem *gnome_bonobo_item_new      (GnomeCanvasGroup *parent,
					     GnomeViewFrame   *view_frame);


#endif /* _GNOME_BONOBO_ITEM_H_ */
