#ifndef _BONOBO_UI_TOOLBAR_H_
#define _BONOBO_UI_TOOLBAR_H_

#include <gtk/gtkenums.h>
#include <gtk/gtktooltips.h>

BEGIN_GNOME_DECLS

/*
 * FIXME: This container should be cooler; gtkcoolbar perhaps.
 */

#define BONOBO_UI_TOOLBAR_TYPE        (bonobo_ui_toolbar_get_type ())
#define BONOBO_UI_TOOLBAR(o)          (GTK_CHECK_CAST ((o), BONOBO_UI_TOOLBAR_TYPE, BonoboUIToolbar))
#define BONOBO_UI_TOOLBAR_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_UI_TOOLBAR_TYPE, BonoboUIToolbarClass))
#define BONOBO_IS_UI_TOOLBAR(o)       (GTK_CHECK_TYPE ((o), BONOBO_UI_TOOLBAR_TYPE))
#define BONOBO_IS_UI_TOOLBAR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_UI_TOOLBAR_TYPE))

typedef struct _BonoboUIToolbar BonoboUIToolbar;

struct _BonoboUIToolbar {
	GtkHBox           box;

	GtkTooltips      *tooltips;
	GtkReliefStyle    relief;
	GtkToolbarStyle   look;

	gpointer          dummy;
};

typedef struct {
	GtkHBoxClass      hbox_class;

	void            (*geometry_changed) (BonoboUIToolbar *toolbar);

	gpointer          dummy;
} BonoboUIToolbarClass;

GtkType    bonobo_ui_toolbar_get_type     (void);
GtkWidget *bonobo_ui_toolbar_new          (void);

void       bonobo_ui_toolbar_add          (BonoboUIToolbar *toolbar,
					    GtkWidget        *widget);

void       bonobo_ui_toolbar_set_relief   (BonoboUIToolbar *toolbar,
					    GtkReliefStyle    relief);

void       bonobo_ui_toolbar_set_style    (BonoboUIToolbar *toolbar,
					    GtkToolbarStyle   look);

void       bonobo_ui_toolbar_set_tooltips (BonoboUIToolbar *toolbar,
					    gboolean          enable);

GtkTooltips *bonobo_ui_toolbar_get_tooltips    (BonoboUIToolbar *toolbar);

void         bonobo_ui_toolbar_set_homogeneous (BonoboUIToolbar *toolbar,
						gboolean         homogeneous);

END_GNOME_DECLS

#endif /* _BONOBO_UI_TOOLBAR_H_ */



