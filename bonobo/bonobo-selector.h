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

struct _GnomeBonoboSelectorClass
{
	GnomeDialogClass parent_class;
	
	void (* ok)	(GnomeBonoboSelector *sel);
	void (* cancel)	(GnomeBonoboSelector *sel);
};

struct _GnomeBonoboSelector
{
	GnomeDialog dialog;
		
	gpointer priv;
};

GtkType gnome_bonobo_selector_get_type	(void);

/* title: Dialog title 
 * interfaces_required: must implement these interfaces. NULL-terminated
 * If interfaces_required is NULL, by default we assume 
 * IDL:GNOME/BonoboObject:1.0
 */
GtkWidget  *gnome_bonobo_selector_new	(const gchar *title, 
	const gchar **interfaces_required);

/* You are responsible for freeing the return values of all the
 * following functions */

gchar *gnome_bonobo_selector_get_selected_goad_id (GnomeBonoboSelector *sel);

/* Activates the selected object, and returns the goad id.*/
gchar *gnome_bonobo_selector_activate_selected (GnomeBonoboSelector *sel,
	GoadActivationFlags flags);

/* Runs a modal dialog version, and returns goad id */
gchar *gnome_bonobo_select_goad_id	(const gchar *title,
	const gchar **interfaces_required);

/* Ditto, but activates object too */
gchar *gnome_bonobo_select_activate	(const gchar *title,
	const gchar **interfaces_required, GoadActivationFlags flags);

END_GNOME_DECLS

#endif /* __GNOME_BONOBO_SELECTOR_H__ */

