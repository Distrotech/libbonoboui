#ifndef _GNOME_CLIENT_SITE_H_
#define _GNOME_CLIENT_SITE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/gnome-unknown.h>
#include <bonobo/gnome-object-client.h>
#include <bonobo/gnome-container.h>

BEGIN_GNOME_DECLS
 
#define GNOME_CLIENT_SITE_TYPE        (gnome_client_site_get_type ())
#define GNOME_CLIENT_SITE(o)          (GTK_CHECK_CAST ((o), GNOME_CLIENT_SITE_TYPE, GnomeClientSite))
#define GNOME_CLIENT_SITE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_CLIENT_SITE_TYPE, GnomeClientSiteClass))
#define GNOME_IS_CLIENT_SITE(o)       (GTK_CHECK_TYPE ((o), GNOME_CLIENT_SITE_TYPE))
#define GNOME_IS_CLIENT_SITE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_CLIENT_SITE_TYPE))

typedef struct {
	GnomeUnknown base;

	GnomeContainer    *container;
	GnomeUnknownClient *bound_object;
	int               child_shown:1;
} GnomeClientSite;

typedef struct {
	GnomeUnknownClass parent_class;

	void (*show_window)  (GnomeClientSite *, CORBA_boolean shown);
	void (*queue_resize) (GnomeClientSite *);
	void (*save_object)  (GnomeClientSite *, GNOME_Persist_Status *status);
} GnomeClientSiteClass;

GtkType          gnome_client_site_get_type       (void);
GnomeClientSite *gnome_client_site_new            (GnomeContainer *container);
GnomeClientSite *gnome_client_site_construct      (GnomeClientSite  *client_site,
						   GNOME_ClientSite corba_client_site,
						   GnomeContainer   *container);

void             gnome_client_site_set_moniker    (GnomeClientSite *client_site,
						   GnomeMoniker   *moniker);

gboolean         gnome_client_site_bind_bonobo_object (GnomeClientSite *client_site,
						   GnomeUnknownClient *object);

extern POA_GNOME_ClientSite__epv gnome_client_site_epv;

END_GNOME_DECLS

#endif

