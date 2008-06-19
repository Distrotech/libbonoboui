/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-wrapper.h: Wrapper for plug/socket children in Bonobo
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Copyright (C) 1999 the Free Software Foundation
 */

#ifndef BONOBO_WRAPPER_H
#define BONOBO_WRAPPER_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GNOME_TYPE_WRAPPER            (bonobo_wrapper_get_type ())
#define BONOBO_WRAPPER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_WRAPPER, BonoboWrapper))
#define BONOBO_WRAPPER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_WRAPPER, BonoboWrapperClass))
#define BONOBO_IS_WRAPPER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_WRAPPER))
#define BONOBO_IS_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_WRAPPER))


typedef struct _BonoboWrapperPrivate BonoboWrapperPrivate;

typedef struct {
	GtkBin bin;

	/* Private data. */
	BonoboWrapperPrivate *priv;
} BonoboWrapper;

typedef struct {
	GtkBinClass parent_class;
} BonoboWrapperClass;


GType		 bonobo_wrapper_get_type	(void) G_GNUC_CONST;
GtkWidget	*bonobo_wrapper_new		(void);

void		 bonobo_wrapper_set_covered	(BonoboWrapper *wrapper, gboolean covered);
gboolean	 bonobo_wrapper_is_covered	(BonoboWrapper *wrapper);

gboolean	 bonobo_wrapper_get_visibility	(BonoboWrapper *wrapper);
void		 bonobo_wrapper_set_visibility	(BonoboWrapper *wrapper, gboolean visible);

G_END_DECLS

#endif
