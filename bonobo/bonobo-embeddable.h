/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Embeddable object.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#ifndef _GNOME_EMBEDDABLE_H_
#define _GNOME_EMBEDDABLE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <libgnomeui/gnome-canvas.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-canvas-component.h>

BEGIN_GNOME_DECLS
 
#define GNOME_EMBEDDABLE_TYPE        (gnome_embeddable_get_type ())
#define GNOME_EMBEDDABLE(o)          (GTK_CHECK_CAST ((o), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddable))
#define GNOME_EMBEDDABLE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddableClass))
#define GNOME_IS_EMBEDDABLE(o)       (GTK_CHECK_TYPE ((o), GNOME_EMBEDDABLE_TYPE))
#define GNOME_IS_EMBEDDABLE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_EMBEDDABLE_TYPE))

struct _GnomeEmbeddable;
struct _GnomeEmbeddablePrivate;
typedef struct _GnomeEmbeddable GnomeEmbeddable;
typedef struct _GnomeEmbeddablePrivate GnomeEmbeddablePrivate;
typedef struct _GnomeVerb GnomeVerb;

#include <bonobo/gnome-view.h>

#define GNOME_VIEW_FACTORY(fn) ((GnomeViewFactory)(fn))

typedef GnomeView * (*GnomeViewFactory)(GnomeEmbeddable *embeddable, const GNOME_ViewFrame view_frame, void *closure);
typedef GnomeCanvasComponent *(*GnomeItemCreator)(GnomeEmbeddable *embeddable, GnomeCanvas *canvas, void *user_data);
typedef void (*GnomeEmbeddableForeachViewFn) (GnomeView *view, void *data);
typedef void (*GnomeEmbeddableForeachItemFn) (GnomeCanvasComponent *comp, void *data);

struct _GnomeEmbeddable {
	GnomeObject base;

	char *host_name;
	char *host_appname;
	GNOME_ClientSite client_site;

	/*
	 * A list of GnomeVerb structures for the verbs supported by
	 * this component.
	 */
	GList *verbs;

	/*
	 * The URI this component represents
	 */
	char *uri;
	
	GnomeEmbeddablePrivate *priv;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*host_name_changed)  (GnomeEmbeddable *comp, const char *hostname);
	void (*uri_changed)        (GnomeEmbeddable *comp, const char *uri);
} GnomeEmbeddableClass;

struct _GnomeVerb {
	/*
	 * A unique string which identifies this verb.  This is the
	 * string which is used to activate the verb.
	 *
	 * Example: "next_page"
	 */
	char *name;

	/*
	 * A string which specifies the action the verb performs.
	 * This verb may be translated to the locale of the user
	 * and used as an entry in a popup menu.
	 *
	 * Example: "_Next Page"
	 */
	char *label;

	/*
	 * A string which gives a slightly more verbose description of
	 * the verb, for use in tooltips or status bars.  This string
	 * may be translated to the user's locale.
	 *
	 * Example: "Turn to the next page"
	 */
	char *hint;
};

GtkType          gnome_embeddable_get_type         (void);
GnomeEmbeddable *gnome_embeddable_new              (GnomeViewFactory factory,
						    void *data);
GnomeEmbeddable *gnome_embeddable_new_canvas_item  (GnomeItemCreator item_factory,
						    void *closure);
GnomeEmbeddable *gnome_embeddable_construct        (GnomeEmbeddable *embeddable,
						    GNOME_Embeddable corba_embeddable,
						    GnomeViewFactory factory,
						    void *data);
GnomeEmbeddable *gnome_embeddable_construct_full   (GnomeEmbeddable *embeddable,
						    GNOME_Embeddable corba_embeddable,
						    GnomeViewFactory factory,
						    void *factory_data,
						    GnomeItemCreator item_factory,
						    void *item_factory_data);
GNOME_Embeddable gnome_embeddable_corba_object_create (GnomeObject *object);

void             gnome_embeddable_add_verb         (GnomeEmbeddable *embeddable,
						    const char *verb_name,
						    const char *verb_label,
						    const char *verb_hint);
void             gnome_embeddable_add_verbs        (GnomeEmbeddable *embeddable,
						    const GnomeVerb *verbs);
void             gnome_embeddable_remove_verb      (GnomeEmbeddable *embeddable,
						    const char *verb_name);
void             gnome_embeddable_set_view_factory (GnomeEmbeddable *embeddable,
						    GnomeViewFactory factory,
						    void *data);
const GList	*gnome_embeddable_get_verbs	   (GnomeEmbeddable *embeddable);


const char      *gnome_embeddable_get_uri          (GnomeEmbeddable *embeddable);
void             gnome_embeddable_set_uri          (GnomeEmbeddable *embeddable,
						    const char *uri);

void             gnome_embeddable_foreach_view     (GnomeEmbeddable *embeddable,
						    GnomeEmbeddableForeachViewFn fn,
						    void *data);
void             gnome_embeddable_foreach_item     (GnomeEmbeddable *embeddable,
						    GnomeEmbeddableForeachItemFn fn,
						    void *data);

extern POA_GNOME_Embeddable__epv gnome_embeddable_epv;

END_GNOME_DECLS

#endif


