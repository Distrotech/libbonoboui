/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GNOME_BONOBO_SELECTOR_H__
#define __GNOME_BONOBO_SELECTOR_H__


#include <gtk/gtk.h>
#include <libgnorba/gnorba.h>
#include <libgnomeui/gnome-dialog.h>

BEGIN_GNOME_DECLS

#define GNOME_BONOBO_SELECTOR(obj)	GTK_CHECK_CAST(obj, gnome_bonobo_selector_get_type (), GnomeBonoboSelector)
#define GNOME_BONOBO_SELECTOR_CLASS	GTK_CHECK_CLASS_CAST (klass, gnome_bonobo_selector_get_type (), GnomeBonoboSelectorClass)
#define GNOME_IS_BONOBO_SELECTOR(obj)	GTK_CHECK_TYPE (obj, gnome_bonobo_selector_get_type ())

typedef struct _GnomeBonoboSelector GnomeBonoboSelector;
typedef struct _GnomeBonoboSelectorClass GnomeBonoboSelectorClass;
typedef struct _GnomeBonoboSelectorPrivate GnomeBonoboSelectorPrivate;

struct _GnomeBonoboSelectorClass
{
	GnomeDialogClass parent_class;
	
	void (* ok)	(GnomeBonoboSelector *sel);
	void (* cancel)	(GnomeBonoboSelector *sel);
};

struct _GnomeBonoboSelector
{
	GnomeDialog dialog;
		
	GnomeBonoboSelectorPrivate *priv;
};

GtkType	    gnome_bonobo_selector_get_type		(void);

GtkWidget  *gnome_bonobo_selector_new			(const gchar *title,
							 const gchar **interfaces_required);
gchar	   *gnome_bonobo_selector_get_selected_goad_id  (GnomeBonoboSelector *sel);
gchar	   *gnome_bonobo_select_goad_id			(const gchar *title,
							 const gchar **interfaces_required);

END_GNOME_DECLS

#endif /* __GNOME_BONOBO_SELECTOR_H__ */

