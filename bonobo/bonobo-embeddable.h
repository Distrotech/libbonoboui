/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Embeddable object.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@gnome-support.com)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#ifndef _GNOME_EMBEDDABLE_H_
#define _GNOME_EMBEDDABLE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>

BEGIN_GNOME_DECLS
 
#define GNOME_EMBEDDABLE_TYPE        (gnome_embeddable_get_type ())
#define GNOME_EMBEDDABLE(o)          (GTK_CHECK_CAST ((o), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddable))
#define GNOME_EMBEDDABLE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddableClass))
#define GNOME_IS_EMBEDDABLE(o)       (GTK_CHECK_TYPE ((o), GNOME_EMBEDDABLE_TYPE))
#define GNOME_IS_EMBEDDABLE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_EMBEDDABLE_TYPE))

struct _GnomeEmbeddable;
typedef struct _GnomeEmbeddable GnomeEmbeddable;
typedef struct _GnomeVerb GnomeVerb;

#include <bonobo/gnome-view.h>

typedef GnomeView * (*GnomeViewFactory)(GnomeEmbeddable *embeddable, const GNOME_ViewFrame view_frame, void *closure);

struct _GnomeEmbeddable {
	GnomeObject base;

	char *host_name;
	char *host_appname;
	GNOME_ClientSite client_site;

	/*
	 * The View factory
	 */
	GnomeViewFactory view_factory;
	void *view_factory_closure;

	/*
	 * The instantiated views for this Embeddable.
	 */
	GList *views;

	/*
	 * A list of GnomeVerb structures for the verbs supported by
	 * this component.
	 */
	GList *verbs;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*host_name_changed)  (GnomeEmbeddable *comp);
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
GnomeEmbeddable *gnome_embeddable_construct        (GnomeEmbeddable *bonobo_object,
						    GNOME_Embeddable corba_bonobo_object,
						    GnomeViewFactory factory,
						    void *data);
GNOME_Embeddable gnome_embeddable_corba_object_create (GnomeObject *object);
void             gnome_embeddable_add_verb         (GnomeEmbeddable *bonobo_object,
						    const char *verb_name,
						    const char *verb_label,
						    const char *verb_hint);
void             gnome_embeddable_add_verbs        (GnomeEmbeddable *bonobo_object,
						    const GnomeVerb *verbs);
void             gnome_embeddable_remove_verb      (GnomeEmbeddable *bonobo_object,
						    const char *verb_name);
void             gnome_embeddable_set_view_factory (GnomeEmbeddable *bonobo_object,
						    GnomeViewFactory factory,
						    void *data);

extern POA_GNOME_Embeddable__epv gnome_embeddable_epv;

END_GNOME_DECLS

#endif


