/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME ClientSite object.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#ifndef _GNOME_CLIENT_SITE_H_
#define _GNOME_CLIENT_SITE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <libgnomeui/gnome-canvas.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-object-client.h>
#include <bonobo/gnome-container.h>

BEGIN_GNOME_DECLS
 
#define GNOME_CLIENT_SITE_TYPE        (gnome_client_site_get_type ())
#define GNOME_CLIENT_SITE(o)          (GTK_CHECK_CAST ((o), GNOME_CLIENT_SITE_TYPE, GnomeClientSite))
#define GNOME_CLIENT_SITE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_CLIENT_SITE_TYPE, GnomeClientSiteClass))
#define GNOME_IS_CLIENT_SITE(o)       (GTK_CHECK_TYPE ((o), GNOME_CLIENT_SITE_TYPE))
#define GNOME_IS_CLIENT_SITE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_CLIENT_SITE_TYPE))

typedef struct _GnomeClientSite GnomeClientSite;
typedef struct _GnomeClientSitePrivate GnomeClientSitePrivate;

#include <bonobo/gnome-view-frame.h>

struct _GnomeClientSite {
	GnomeObject base;

	GnomeContainer    *container;
	GnomeObjectClient *bound_object;
	GList		  *view_frames;
	GList             *canvas_items;
	int               child_shown:1;

	GnomeClientSitePrivate *priv;
};

typedef struct {
	GnomeObjectClass parent_class;

	void (*show_window)  (GnomeClientSite *, CORBA_boolean shown);
	void (*queue_resize) (GnomeClientSite *);
	void (*save_object)  (GnomeClientSite *, GNOME_Persist_Status *status);
} GnomeClientSiteClass;

GtkType            gnome_client_site_get_type		(void);
GnomeClientSite   *gnome_client_site_new		(GnomeContainer *container);
GnomeClientSite   *gnome_client_site_construct		(GnomeClientSite  *client_site,
							 GNOME_ClientSite corba_client_site,
							 GnomeContainer   *container);

gboolean           gnome_client_site_bind_embeddable	(GnomeClientSite *client_site,
							 GnomeObjectClient *object);
GnomeObjectClient *gnome_client_site_get_embeddable	(GnomeClientSite *client_site);

/*
 * Proxy/Utility functions.
 */
GnomeViewFrame    *gnome_client_site_new_view		(GnomeClientSite *client_site);
GnomeCanvasItem   *gnome_client_site_new_item           (GnomeClientSite *client_site,
							 GnomeCanvasGroup *group);
GList		  *gnome_client_site_get_verbs		(GnomeClientSite *client_site);
void		   gnome_client_site_free_verbs		(GList *verb_list);

extern POA_GNOME_ClientSite__epv gnome_client_site_epv;

END_GNOME_DECLS

#endif

