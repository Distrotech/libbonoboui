/**
 * Bonobo Desktop Window Control implementation.
 * Enables applications to export their geometry control through CORBA.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _BONOBO_DESKTOP_WINDOW_H_
#define _BONOBO_DESKTOP_WINDOW_H_

#include <gmacros.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwindow.h>
#include <bonobo/bonobo-object.h>

G_BEGIN_DECLS
 
#define BONOBO_DESKTOP_WINDOW_TYPE        (bonobo_desktop_window_get_type ())
#define BONOBO_DESKTOP_WINDOW(o)          (GTK_CHECK_CAST ((o), BONOBO_DESKTOP_WINDOW_TYPE, BonoboDesktopWindow))
#define BONOBO_DESKTOP_WINDOW_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_DESKTOP_WINDOW_TYPE, BonoboDesktopWindowClass))
#define BONOBO_IS_DESKTOP_WINDOW(o)       (GTK_CHECK_TYPE ((o), BONOBO_DESKTOP_WINDOW_TYPE))
#define BONOBO_IS_DESKTOP_WINDOW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_DESKTOP_WINDOW_TYPE))

typedef struct _BonoboDesktopWindowPrivate BonoboDesktopWindowPrivate;

typedef struct {
	BonoboObject base;
	GtkWindow   *window;
	BonoboDesktopWindowPrivate *priv;
} BonoboDesktopWindow;

typedef struct {
	BonoboObjectClass parent_class;

	POA_Bonobo_Desktop_Window__epv epv;
} BonoboDesktopWindowClass;

GtkType              bonobo_desktop_window_get_type  (void);
BonoboDesktopWindow *bonobo_desktop_window_construct (BonoboDesktopWindow *desk_win,
						      GtkWindow           *toplevel);
BonoboDesktopWindow *bonobo_desktop_window_new       (GtkWindow           *toplevel);
void                 bonobo_desktop_window_control   (BonoboObject        *object,
						      GtkWindow           *win);

G_END_DECLS

#endif /* _BONOBO_DESKTOP_WINDOW_H_ */
