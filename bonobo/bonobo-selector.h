/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __BONOBO_SELECTOR_H__
#define __BONOBO_SELECTOR_H__

#include <libgnomeui/gnome-dialog.h>

BEGIN_GNOME_DECLS

#define BONOBO_SELECTOR(obj)	GTK_CHECK_CAST(obj, bonobo_selector_get_type (), BonoboSelector)
#define BONOBO_SELECTOR_CLASS	GTK_CHECK_CLASS_CAST (klass, bonobo_selector_get_type (), BonoboSelectorClass)
#define GNOME_IS_BONOBO_SELECTOR(obj)	GTK_CHECK_TYPE (obj, bonobo_selector_get_type ())

typedef struct _BonoboSelector BonoboSelector;
typedef struct _BonoboSelectorClass BonoboSelectorClass;
typedef struct _BonoboSelectorPrivate BonoboSelectorPrivate;

struct _BonoboSelectorClass
{
	GnomeDialogClass parent_class;
	
	void (* ok)	(BonoboSelector *sel);
	void (* cancel)	(BonoboSelector *sel);
};

struct _BonoboSelector
{
	GnomeDialog dialog;
		
	BonoboSelectorPrivate *priv;
};

GtkType	   bonobo_selector_get_type        (void);

GtkWidget *bonobo_selector_new             (const gchar *title,
					    const gchar **interfaces_required);
gchar	  *bonobo_selector_get_selected_id (BonoboSelector *sel);

gchar	  *bonobo_selector_select_id       (const gchar *title,
					    const gchar **interfaces_required);

END_GNOME_DECLS

#endif /* __BONOBO_SELECTOR_H__ */

