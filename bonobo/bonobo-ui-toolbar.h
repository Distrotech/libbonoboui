#ifndef _BONOBO_APP_TOOLBAR_H_
#define _BONOBO_APP_TOOLBAR_H_

#include <gtk/gtkenums.h>
#include <gtk/gtktooltips.h>

BEGIN_GNOME_DECLS

/*
 * FIXME: This container should be cooler; gtkcoolbar perhaps.
 */

#define BONOBO_APP_TOOLBAR_TYPE        (bonobo_app_toolbar_get_type ())
#define BONOBO_APP_TOOLBAR(o)          (GTK_CHECK_CAST ((o), BONOBO_APP_TOOLBAR_TYPE, BonoboAppToolbar))
#define BONOBO_APP_TOOLBAR_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_APP_TOOLBAR_TYPE, BonoboAppToolbarClass))
#define BONOBO_IS_APP_TOOLBAR(o)       (GTK_CHECK_TYPE ((o), BONOBO_APP_TOOLBAR_TYPE))
#define BONOBO_IS_APP_TOOLBAR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_APP_TOOLBAR_TYPE))

typedef struct _BonoboAppToolbar BonoboAppToolbar;

struct _BonoboAppToolbar {
	GtkHBox           box;

	GtkTooltips      *tooltips;
	GtkReliefStyle    relief;
	GtkToolbarStyle   look;

	gpointer          dummy;
};

typedef struct {
	GtkHBoxClass      hbox_class;

	void            (*geometry_changed) (BonoboAppToolbar *toolbar);

	gpointer          dummy;
} BonoboAppToolbarClass;

GtkType    bonobo_app_toolbar_get_type     (void);
GtkWidget *bonobo_app_toolbar_new          (void);

void       bonobo_app_toolbar_add          (BonoboAppToolbar *toolbar,
					    GtkWidget        *widget);

void       bonobo_app_toolbar_set_relief   (BonoboAppToolbar *toolbar,
					    GtkReliefStyle    relief);

void       bonobo_app_toolbar_set_style    (BonoboAppToolbar *toolbar,
					    GtkToolbarStyle   look);

void       bonobo_app_toolbar_set_tooltips (BonoboAppToolbar *toolbar,
					    gboolean          enable);

GtkTooltips *bonobo_app_toolbar_get_tooltips (BonoboAppToolbar *toolbar);

END_GNOME_DECLS

#endif /* _BONOBO_APP_TOOLBAR_H_ */



