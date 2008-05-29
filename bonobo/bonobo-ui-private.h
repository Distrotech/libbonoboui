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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <bonobo/bonobo-ui-node.h>
#include <bonobo/bonobo-ui-engine.h>
#include <bonobo/bonobo-ui-node-private.h>
#include <bonobo/bonobo-ui-toolbar-control-item.h>

G_BEGIN_DECLS

/* To dump lots of sequence information */
#define noDEBUG_UI

/* To debug render issues in plug/socket/control */
#define noDEBUG_CONTROL

void       bonobo_socket_add_id           (BonoboSocket   *socket,
					   GdkNativeWindow xid);
int        bonobo_ui_preferences_shutdown (void);
void       bonobo_ui_image_set_pixbuf     (GtkImage     *image,
					   GdkPixbuf    *pixbuf);
void       bonobo_ui_util_xml_set_image   (GtkImage     *image,
					   BonoboUINode *node,
					   BonoboUINode *cmd_node,
					   GtkIconSize   icon_size);
void       bonobo_ui_engine_dispose       (BonoboUIEngine *engine);
GtkWidget *bonobo_ui_toolbar_button_item_get_image (BonoboUIToolbarButtonItem *item);

GtkWidget *bonobo_ui_internal_toolbar_new (void);

GList *bonobo_ui_internal_toolbar_get_children (GtkWidget *toolbar);

#ifdef G_OS_WIN32
const char *_bonobo_ui_get_localedir      (void);
const char *_bonobo_ui_get_datadir	  (void);
const char *_bonobo_ui_get_uidir	  (void);

#undef BONOBO_LOCALEDIR
#define BONOBO_LOCALEDIR _bonobo_ui_get_localedir()
#undef BONOBO_DATADIR
#define BONOBO_DATADIR _bonobo_ui_get_datadir()
#undef BONOBO_UIDIR
#define BONOBO_UIDIR _bonobo_ui_get_uidir()

#endif

#ifndef   DEBUG_UI

static inline void dbgprintf (const char *format, ...) { }

#else  /* DEBUG_UI */

#include <stdio.h>

#define dbgprintf(format...) fprintf(stderr, format)

#endif /* DEBUG_UI */

G_END_DECLS

#endif /* _BONOBO_UI_PRIVATE_H_ */

