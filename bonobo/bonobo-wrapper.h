/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Wrapper for plug/socket children in Bonobo
 *
 * Copyright (C) 1999 the Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_WRAPPER_H
#define GNOME_WRAPPER_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkbin.h>

BEGIN_GNOME_DECLS


#define GNOME_TYPE_WRAPPER            (gnome_wrapper_get_type ())
#define GNOME_WRAPPER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_WRAPPER, GnomeWrapper))
#define GNOME_WRAPPER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_WRAPPER, GnomeWrapperClass))
#define GNOME_IS_WRAPPER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_WRAPPER))
#define GNOME_IS_WRAPPER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_WRAPPER))


typedef struct _GnomeWrapper GnomeWrapper;
typedef struct _GnomeWrapperClass GnomeWrapperClass;
typedef struct _GnomeWrapperPrivate GnomeWrapperPrivate;

struct _GnomeWrapper {
	GtkBin bin;

	/* The InputOnly window that covers the child */
	GdkWindow *cover;

	/* Private data. */
	GnomeWrapperPrivate *priv;
};

struct _GnomeWrapperClass {
	GtkBinClass parent_class;
};


GtkType		 gnome_wrapper_get_type		(void);
GtkWidget	*gnome_wrapper_new		(void);

void		 gnome_wrapper_set_covered	(GnomeWrapper *wrapper, gboolean covered);
gboolean	 gnome_wrapper_is_covered	(GnomeWrapper *wrapper);

gboolean	 gnome_wrapper_get_visibility	(GnomeWrapper *wrapper);
void		 gnome_wrapper_set_visibility	(GnomeWrapper *wrapper, gboolean visible);

END_GNOME_DECLS

#endif
