/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Wrapper for plug/socket children in Bonobo
 *
 * Copyright (C) 1999 the Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef BONOBO_WRAPPER_H
#define BONOBO_WRAPPER_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkbin.h>

BEGIN_GNOME_DECLS


#define GNOME_TYPE_WRAPPER            (bonobo_wrapper_get_type ())
#define BONOBO_WRAPPER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_WRAPPER, BonoboWrapper))
#define BONOBO_WRAPPER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_WRAPPER, BonoboWrapperClass))
#define BONOBO_IS_WRAPPER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_WRAPPER))
#define BONOBO_IS_WRAPPER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_WRAPPER))


typedef struct _BonoboWrapper BonoboWrapper;
typedef struct _BonoboWrapperClass BonoboWrapperClass;
typedef struct _BonoboWrapperPrivate BonoboWrapperPrivate;

struct _BonoboWrapper {
	GtkBin bin;

	/* Private data. */
	BonoboWrapperPrivate *priv;
};

struct _BonoboWrapperClass {
	GtkBinClass parent_class;
};


GtkType		 bonobo_wrapper_get_type		(void);
GtkWidget	*bonobo_wrapper_new		(void);

void		 bonobo_wrapper_set_covered	(BonoboWrapper *wrapper, gboolean covered);
gboolean	 bonobo_wrapper_is_covered	(BonoboWrapper *wrapper);

gboolean	 bonobo_wrapper_get_visibility	(BonoboWrapper *wrapper);
void		 bonobo_wrapper_set_visibility	(BonoboWrapper *wrapper, gboolean visible);

END_GNOME_DECLS

#endif
