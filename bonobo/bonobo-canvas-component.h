/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_CANVAS_COMPONENT_H_
#define _GNOME_CANVAS_COMPONENT_H_

#include <libgnome/gnome-defs.h>
#include <bonobo/gnome-object.h>

BEGIN_GNOME_DECLS
 
#define GNOME_CANVAS_COMPONENT_TYPE        (gnome_canvas_component_get_type ())
#define GNOME_CANVAS_COMPONENT(o)          (GTK_CHECK_CAST ((o), GNOME_CANVAS_COMPONENT_TYPE, GnomeCanvasComponent))
#define GNOME_CANVAS_COMPONENT_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_CANVAS_COMPONENT__TYPE, GnomeCanvasComponentClass))
#define GNOME_IS_CANVAS_COMPONENT(o)       (GTK_CHECK_TYPE ((o), GNOME_CANVAS_COMPONENT_TYPE))
#define GNOME_IS_CANVAS_COMPONENT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_CANVAS_COMPONENT_TYPE))

typedef struct _GnomeCanvasComponent        GnomeCanvasComponent;
typedef struct _GnomeCanvasComponentPrivate GnomeCanvasComponentPrivate;
typedef struct _GnomeCanvasComponentClass   GnomeCanvasComponentClass;

struct _GnomeCanvasComponent {
	GnomeObject base;
	GnomeCanvasComponentPrivate *priv;
};

struct _GnomeCanvasComponentClass {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	 void (*set_bounds) (GnomeCanvasComponent *component, GNOME_Canvas_DRect *bbox, CORBA_Environment *ev);
};

GtkType           gnome_canvas_component_get_type      (void);
GNOME_Canvas_Item gnome_canvas_component_object_create (GnomeObject *object);
void              gnome_canvas_component_set_proxy     (GnomeCanvasComponent *comp,
							GNOME_Canvas_ItemProxy proxy);
GnomeCanvasComponent *gnome_canvas_component_construct (GnomeCanvasComponent *comp,
							GNOME_Canvas_Item    ccomp,
							GnomeCanvasItem     *item);
GnomeCanvasComponent *gnome_canvas_component_new (GnomeCanvasItem *item);
GnomeCanvasItem      *gnome_canvas_component_get_item  (GnomeCanvasComponent *comp);

/* CORBA default vector methods we provide */
extern POA_GNOME_Canvas_Item__epv  gnome_canvas_item_epv;
extern POA_GNOME_Canvas_Item__vepv gnome_canvas_item_vepv;

END_GNOME_DECLS

#endif /* */
