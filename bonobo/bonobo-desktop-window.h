
#ifndef _GNOME_VIEW_H_
#define _GNOME_VIEW_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwindow.h>
#include <bonobo/gnome-object.h>

BEGIN_GNOME_DECLS
 
#define GNOME_DESKTOP_WINDOW_TYPE        (gnome_desktop_window_get_type ())
#define GNOME_DESKTOP_WINDOW(o)          (GTK_CHECK_CAST ((o), GNOME_DESKTOP_WINDOW_TYPE, GnomeDesktopWindow))
#define GNOME_DESKTOP_WINDOW_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_DESKTOP_WINDOW_TYPE, GnomeDesktopWindowClass))
#define GNOME_IS_DESKTOP_WINDOW(o)       (GTK_CHECK_TYPE ((o), GNOME_DESKTOP_WINDOW_TYPE))
#define GNOME_IS_DESKTOP_WINDOW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_DESKTOP_WINDOW_TYPE))

typedef struct _GnomeDesktopWindow GnomeDesktopWindow;
typedef struct _GnomeDesktopWindowPrivate GnomeDesktopWindowPrivate;
typedef struct _GnomeDesktopWindowClass GnomeDesktopWindowClass;

struct _GnomeDesktopWindow {
	GnomeObject base;
	GtkWindow   *window;
	GnomeDesktopWindowPrivate *priv;
};

struct _GnomeDesktopWindowClass {
	GnomeObjectClass parent_class;
};

GtkType             gnome_desktop_window_get_type  (void);
GnomeDesktopWindow *gnome_desktop_window_construct (GnomeDesktopWindow *desktop_window,
						    GNOME_Desktop_Window corba_desktop_window,
						    GtkWindow *toplevel);
GnomeDesktopWindow *gnome_desktop_window_new       (GtkWindow *toplevel);
void                gnome_desktop_window_control   (GnomeObject *object,
						    GtkWindow *win);
GNOME_Desktop_Window gnome_desktop_window_corba_object_create (GnomeObject *object);

POA_GNOME_Desktop_Window__epv *gnome_desktop_window_get_epv   (void);

/* CORBA default vector methods we provide */
extern POA_GNOME_Desktop_Window__vepv gnome_desktop_window_vepv;

END_GNOME_DECLS

#endif /* _GNOME_DESKTOP_WINDOW_H_ */
