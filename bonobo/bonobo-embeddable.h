/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Embeddable object.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _BONOBO_EMBEDDABLE_H_
#define _BONOBO_EMBEDDABLE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <libgnomeui/gnome-canvas.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-canvas-component.h>

BEGIN_GNOME_DECLS
 
#define BONOBO_EMBEDDABLE_TYPE        (bonobo_embeddable_get_type ())
#define BONOBO_EMBEDDABLE(o)          (GTK_CHECK_CAST ((o), BONOBO_EMBEDDABLE_TYPE, BonoboEmbeddable))
#define BONOBO_EMBEDDABLE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_EMBEDDABLE_TYPE, BonoboEmbeddableClass))
#define BONOBO_IS_EMBEDDABLE(o)       (GTK_CHECK_TYPE ((o), BONOBO_EMBEDDABLE_TYPE))
#define BONOBO_IS_EMBEDDABLE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_EMBEDDABLE_TYPE))

struct _BonoboEmbeddable;
struct _BonoboEmbeddablePrivate;
typedef struct _BonoboEmbeddable BonoboEmbeddable;
typedef struct _BonoboEmbeddablePrivate BonoboEmbeddablePrivate;
typedef struct _GnomeVerb GnomeVerb;

#include <bonobo/bonobo-view.h>

#define BONOBO_VIEW_FACTORY(fn) ((BonoboViewFactory)(fn))

typedef BonoboView * (*BonoboViewFactory)(BonoboEmbeddable *embeddable, const Bonobo_ViewFrame view_frame, void *closure);
typedef BonoboCanvasComponent *(*GnomeItemCreator)(BonoboEmbeddable *embeddable, GnomeCanvas *canvas, void *user_data);
typedef void (*BonoboEmbeddableForeachViewFn) (BonoboView *view, void *data);
typedef void (*BonoboEmbeddableForeachItemFn) (BonoboCanvasComponent *comp, void *data);

struct _BonoboEmbeddable {
	BonoboObject base;

	char *host_name;
	char *host_appname;
	Bonobo_ClientSite client_site;

	/*
	 * A list of GnomeVerb structures for the verbs supported by
	 * this component.
	 */
	GList *verbs;

	/*
	 * The URI this component represents
	 */
	char *uri;
	
	BonoboEmbeddablePrivate *priv;
};

typedef struct {
	BonoboObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*host_name_changed)  (BonoboEmbeddable *comp, const char *hostname);
	void (*uri_changed)        (BonoboEmbeddable *comp, const char *uri);
} BonoboEmbeddableClass;

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

GtkType           bonobo_embeddable_get_type         (void);
BonoboEmbeddable *bonobo_embeddable_new              (BonoboViewFactory  factory,
						      void              *data);
BonoboEmbeddable *bonobo_embeddable_new_canvas_item  (GnomeItemCreator item_factory,
						      void *closure);
BonoboEmbeddable *bonobo_embeddable_construct        (BonoboEmbeddable *embeddable,
						      Bonobo_Embeddable corba_embeddable,
						      BonoboViewFactory factory,
						      void *data);
BonoboEmbeddable *bonobo_embeddable_construct_full   (BonoboEmbeddable *embeddable,
						      Bonobo_Embeddable corba_embeddable,
						      BonoboViewFactory factory,
						      void *factory_data,
						      GnomeItemCreator item_factory,
						      void *item_factory_data);
Bonobo_Embeddable bonobo_embeddable_corba_object_create (BonoboObject *object);

void             bonobo_embeddable_add_verb         (BonoboEmbeddable *embeddable,
						     const char *verb_name,
						     const char *verb_label,
						     const char *verb_hint);
void             bonobo_embeddable_add_verbs        (BonoboEmbeddable *embeddable,
						     const GnomeVerb *verbs);
void             bonobo_embeddable_remove_verb      (BonoboEmbeddable *embeddable,
						     const char *verb_name);
void             bonobo_embeddable_set_view_factory (BonoboEmbeddable *embeddable,
						     BonoboViewFactory factory,
						     void *data);
const GList	*bonobo_embeddable_get_verbs	   (BonoboEmbeddable *embeddable);


const char      *bonobo_embeddable_get_uri          (BonoboEmbeddable *embeddable);
void             bonobo_embeddable_set_uri          (BonoboEmbeddable *embeddable,
						    const char *uri);

void             bonobo_embeddable_foreach_view     (BonoboEmbeddable *embeddable,
						    BonoboEmbeddableForeachViewFn fn,
						    void *data);
void             bonobo_embeddable_foreach_item     (BonoboEmbeddable *embeddable,
						    BonoboEmbeddableForeachItemFn fn,
						    void *data);

POA_Bonobo_Embeddable__epv *bonobo_embeddable_get_epv (void);

END_GNOME_DECLS

#endif


