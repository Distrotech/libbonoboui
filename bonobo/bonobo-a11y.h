/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo accessibility helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */
#ifndef _BONOBO_A11Y_H_
#define _BONOBO_A11Y_H_

#include <gtk/gtkwidget.h>

typedef void  (*BonoboA11YClassInitFn)   (AtkObjectClass *klass);

AtkObject *bonobo_a11y_create_accessible_for (GtkWidget            *widget,
					      const char           *gail_parent_class,
					      BonoboA11YClassInitFn class_init);

#endif /* _BONOBO_A11Y_H_ */
