/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo accessibility wrappers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */
#ifndef _BONOBO_A11Y_H_
#define _BONOBO_A11Y_H_

#include <glib-object.h>

void   bonobo_a11y_clobber_atk_junk_code (void);
void   bonobo_a11y_register_type_for     (GType          atk_object_type,
					  GType          gtk_widget_type);
GType  bonobo_a11y_get_derived_type_for  (GType          widget_type,
					  GClassInitFunc class_init);

#endif /* _BONOBO_A11Y_H_ */
