#ifndef _GNOME_VIEW_H_
#define _GNOME_VIEW_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>

BEGIN_GNOME_DECLS
 
#define GNOME_VIEW_TYPE        (gnome_view_get_type ())
#define GNOME_VIEW_ITEM(o)     (GTK_CHECK_CAST ((o), GNOME_VIEW_TYPE, GnomeView))
#define GNOME_VIEW_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_VIEW_TYPE, GnomeViewClass))
#define GNOME_IS_VIEW(o)       (GTK_CHECK_TYPE ((o), GNOME_VIEW_TYPE))
#define GNOME_IS_VIEW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_VIEW_TYPE))

typedef struct {
	GnomeObject base;
	GtkWidget   *widget;
} GnomeView;

typedef struct {
	GnomeObjectClass parent_class;
} GnomeViewClass;

GtkType      gnome_view_get_type  (void);

GnomeObject *gnome_view_new       (GtkWidget *widget);

#endif
