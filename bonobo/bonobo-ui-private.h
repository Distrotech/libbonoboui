/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo UI internal prototypes / helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _BONOBO_UI_PRIVATE_H_
#define _BONOBO_UI_PRIVATE_H_

#include <gtk/gtkmisc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkimage.h>
#include <bonobo/bonobo-ui-node.h>
#include <bonobo/bonobo-ui-engine.h>
#include <bonobo/bonobo-ui-node-private.h>
#include <bonobo/bonobo-ui-toolbar-control-item.h>

/* To debug plug/socket/control render issues */
#undef DEBUG_CONTROL

/* To dump lots of sequence information */
#define noDEBUG_UI

int        bonobo_ui_preferences_shutdown (void);
void       bonobo_ui_image_set_pixbuf     (GtkImage       *image,
					   GdkPixbuf      *pixbuf);
GdkPixbuf *bonobo_ui_util_xml_get_pixbuf  (GtkWidget      *widget,
					   BonoboUINode   *node,
					   GtkIconSize     icon_size);
void       bonobo_ui_engine_dispose       (BonoboUIEngine *engine);
void       bonobo_ui_toolbar_control_item_set_sensitive
                                          (BonoboUIToolbarControlItem *item,
					   gboolean                    sensitive);

#ifndef   DEBUG_UI

static inline void dprintf (const char *format, ...) { };

#else  /* DEBUG_UI */

#include <stdio.h>

#define dprintf(format...) fprintf(stderr, format)

#endif /* DEBUG_UI */

#endif /* _BONOBO_UI_PRIVATE_H_ */

