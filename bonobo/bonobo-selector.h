/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GNOME_BONOBO_SELECTOR_H__
#define __GNOME_BONOBO_SELECTOR_H__

#include <gtk/gtk.h>
#ifdef BONOBO_USE_GNOME2
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif
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

#ifdef BONOBO_USE_GNOME2
char	   *gnome_bonobo_select_iid			(const char *title,
							 const char *requirements,
							 const char **sort_order);

GtkWidget  *gnome_bonobo_selector_new			(const char *title,
							 const char *requirements,
							 const char **sort_order);
char	   *gnome_bonobo_selector_get_selected_iid      (GnomeBonoboSelector *sel);

#else
GtkWidget  *gnome_bonobo_selector_new			(const char *title,
							 const char **interfaces_required);
char	   *gnome_bonobo_selector_get_selected_goad_id  (GnomeBonoboSelector *sel);
char	   *gnome_bonobo_select_goad_id			(const char *title,
							 const char **interfaces_required);
#endif

END_GNOME_DECLS

#endif /* __GNOME_BONOBO_SELECTOR_H__ */

