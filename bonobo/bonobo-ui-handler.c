/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * GNOME::UIHandler.
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 *
 * Author:
 *    Nat Friedman (nat@nat.org)
 */

/*
 * Notes to self:
 *  - Toolbar radiogroups!
 *  - Check exceptions in all *_remote_* functions.
 *  - use gnome_preferences_[get/set]_[menus/toolbars] !
 *  - routines to override the toolbar characteristics
 *  - implement set_pos.
 *  - docs.
 *  - Make toplevel checks raise an exception of some sort.
 *  - FIXMEs
 *  - CORBA_Object_releases.
 */

#include <config.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-ui-handler.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkaccellabel.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtkmain.h>
#include <libgnome/libgnome.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-app.h>
#include <libgnomeui/gnome-app-helper.h>
#include <libgnomeui/gtkpixmapmenuitem.h>
#include <libgnomeui/gnome-preferences.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-uidefs.h>

/*
 * Global variables.
 */

/* Parent object class in the GTK hierarchy. */
static GnomeObjectClass *gnome_ui_handler_parent_class;

/*
 * The entry point vectors for the GNOME_UIHandler server used to
 * handle remote menu/toolbar merging.
 */
static POA_GNOME_UIHandler__epv gnome_ui_handler_epv;
static POA_GNOME_UIHandler__vepv gnome_ui_handler_vepv;

/*
 * Forward declarations.
 */

typedef struct _ToolbarItemInternal ToolbarItemInternal;
typedef struct _ToolbarToolbarInternal ToolbarToolbarInternal;
typedef struct _ToolbarItemLocalInternal ToolbarItemLocalInternal;
typedef struct _ToolbarToolbarLocalInternal ToolbarToolbarLocalInternal;

/*
 * Signals.
 */
enum {
	MENU_ITEM_ACTIVATED,
	MENU_ITEM_REMOVED,
	MENU_ITEM_OVERRIDDEN,
	MENU_ITEM_REINSTATED,

	TOOLBAR_ITEM_ACTIVATED,
	TOOLBAR_ITEM_REMOVED,
	TOOLBAR_ITEM_OVERRIDDEN,
	TOOLBAR_ITEM_REINSTATED,

	LAST_SIGNAL
};

static guint uih_signals [LAST_SIGNAL];

/*
 * CORBA does not distinguish betwen a zero-length string and a NULL
 * char pointer; everything is a zero-length string to it.  These
 * macros translate between NULL pointers and zero-length strings.
 */
#define CORBIFY_STRING(s) ((s) == NULL ? "" : (s))
#define UNCORBIFY_STRING(s) ((strlen (s) == 0) ? NULL : (s))

#define COPY_STRING(s) (s == NULL ? NULL : g_strdup (s))

/*
 * Internal data structure definitions.
 */

/*
 * This structure is used to maintain an internal representation of a
 * menu item.
 */
typedef struct _MenuItemInternal {

	/*
	 * The GnomeUIHandler.
	 */
	GnomeUIHandler *uih;

	/*
	 * A copy of the GnomeUIHandlerMenuItem for this toolbar item.
	 */
	GnomeUIHandlerMenuItem *item;

	/*
	 * If this item is a subtree or a radio group, this list
	 * contains the item's children.  It is a list of path
	 * strings.  This list is kept in order of the position of the
	 * menu items in the subtree.
	 */
	GList *children;

	/*
	 * The UIHandler CORBA interface for the containee which owns
	 * this particular menu item.
	 */
	GNOME_UIHandler uih_corba;

	/*
	 * If this item is a radio group, then this list points to the
	 * MenuItemInternal structures for the members of the group.
	 */
	GSList *radio_items;

	/*
	 * The sensitivity of the menu item (whether its grayed out or
	 * not).
	 */
	gboolean sensitive;

	/*
	 * If this is a toggle item or a radio item, whether this item
	 * is selected or not.
	 */
	gboolean active;
} MenuItemInternal;

/*
 * This structure is used to hold local callback data for a menu item.
 */
typedef struct {

	/*
	 * A list of child paths, if appropriate.  
	 */
	GList				*children;

	GnomeUIHandlerCallbackFunc	 callback;
	gpointer			 callback_data;
} MenuItemLocalInternal;

/*
 * Prototypes for some internal functions.
 */
static char			 *path_escape_forward_slashes		(char *str);
static char			 *path_unescape_forward_slashes		(char *str);
static gchar			**path_tokenize				(char *path);
static gchar			 *remove_ulines				(const char *str);
static gchar			 *path_get_parent			(char *path);
static GtkWidget		 *uih_toplevel_create_pixmap		(GtkWidget *window,
									 GnomeUIHandlerPixmapType pixmap_type,
									 gpointer pixmap_info);
static void			  pixmap_free_data			(GnomeUIHandlerPixmapType pixmap_type,
									 gpointer pixmap_info);
static gpointer			  pixmap_copy_data			(GnomeUIHandlerPixmapType pixmap_type,
									 gpointer pixmap_info);
static gpointer			  pixmap_xpm_copy_data			(gpointer data);
static GNOME_UIHandler_iobuf	 *pixmap_data_to_corba			(GnomeUIHandlerPixmapType type, gpointer data);
static gpointer	                  pixmap_corba_to_data			(GNOME_UIHandler_PixmapType corba_pixmap_type,
									 GNOME_UIHandler_iobuf *corba_pixmap_data);
static GNOME_UIHandler_PixmapType pixmap_corba_to_type			(GNOME_UIHandler_PixmapType type);
static GNOME_UIHandler_PixmapType pixmap_type_to_corba			(GnomeUIHandlerPixmapType type);
static gint			  pixmap_xpm_get_length			(gpointer data, int *num_lines);
static GnomeUIHandlerPixmapType	  uiinfo_pixmap_type_to_uih		(GnomeUIPixmapType ui_type);
static void			  uih_toplevel_add_containee		(GnomeUIHandler *uih, GNOME_UIHandler containee);

/*
 * Prototypes for some internal menu functions.
 */
static gint		 	  menu_toplevel_save_accels		(gpointer data);
static GnomeUIHandlerMenuItem    *menu_make_item			(char *path, GnomeUIHandlerMenuItemType type,
									 char *label, char *hint,
									 int pos, GnomeUIHandlerPixmapType pixmap_type,
									 gpointer pixmap_data,
									 guint accelerator_key, GdkModifierType ac_mods,
									 GnomeUIHandlerCallbackFunc callback,
									 gpointer callback_data);
static GnomeUIHandlerMenuItem	 *menu_copy_item			(GnomeUIHandlerMenuItem *item);
static void			  menu_free_data			(GnomeUIHandlerMenuItem *item);
static void			  menu_free				(GnomeUIHandlerMenuItem *item);
static GNOME_UIHandler_MenuType	  menu_type_to_corba			(GnomeUIHandlerMenuItemType type);
static GnomeUIHandlerMenuItemType menu_corba_to_type			(GNOME_UIHandler_MenuType type);
static void			  menu_local_do_path			(char *parent_path, GnomeUIHandlerMenuItem *item);
static MenuItemLocalInternal	 *menu_local_get_item			(GnomeUIHandler *uih, char *path);
static void			  menu_local_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerMenuItem *item);
static void			  menu_local_add_parent_entry		(GnomeUIHandler *uih, char *path);
static void			  menu_local_remove_parent_entry	(GnomeUIHandler *uih, char *path, gboolean warn);
static void			  menu_local_remove_item		(GnomeUIHandler *uih, char *path);
static void			  menu_local_remove_item_recursive	(GnomeUIHandler *uih, char *path);
static MenuItemInternal		 *menu_toplevel_get_item		(GnomeUIHandler *uih, char *path);
static MenuItemInternal		 *menu_toplevel_get_item_for_containee	(GnomeUIHandler *uih, char *path,
									 GNOME_UIHandler containee_uih);
static gboolean			  menu_toplevel_item_is_head		(GnomeUIHandler *uih, MenuItemInternal *internal);
static gboolean			  uih_toplevel_check_toplevel		(GnomeUIHandler *uih);
static GtkWidget		 *menu_toplevel_get_shell		(GnomeUIHandler *uih, char *path);
static GtkWidget		 *menu_toplevel_create_label		(GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item,
									 GtkWidget *parent_menu_shell_widget, GtkWidget *menu_widget);
static GtkWidget		 *menu_toplevel_create_item_widget	(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerMenuItem *item);
static GtkWidget		 *menu_toplevel_create_item_widget	(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerMenuItem *item);
static void			  menu_toplevel_install_global_accelerators (GnomeUIHandler *uih,
									    GnomeUIHandlerMenuItem *item,
									    GtkWidget *menu_widget);
static void			  menu_toplevel_connect_signal		(GtkWidget *menu_widget, MenuItemInternal *internal);
static void			  menu_toplevel_create_widgets_recursive (GnomeUIHandler *uih, MenuItemInternal *internal);
static void			  menu_toplevel_put_hint_in_appbar	(GtkWidget *menu_item, gpointer data);
static void			  menu_toplevel_remove_hint_from_appbar	(GtkWidget *menu_item, gpointer data);
static void			  menu_toplevel_put_hint_in_statusbar	(GtkWidget *menu_item, gpointer data);
static void			  menu_toplevel_remove_hint_from_statusbar (GtkWidget *menu_item, gpointer data);
static void			  menu_toplevel_create_hint		(GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item,
									 GtkWidget *menu_item);
static gint			  menu_toplevel_item_activated		(GtkWidget *menu_item, gpointer data);
static MenuItemInternal		 *menu_toplevel_store_data		(GnomeUIHandler *uih, GNOME_UIHandler uih_corba,
									 GnomeUIHandlerMenuItem *item);
static void			  menu_toplevel_override_notify_recursive(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_override_notify		(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_check_override		(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerMenuItem *item,
									 GNOME_UIHandler uih_corba);
static void			  menu_remote_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerMenuItem *item);
static void			  menu_toplevel_remove_widgets		(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_remove_widgets_recursive (GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_add_parent_entry	(GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item);
static void			  menu_toplevel_remove_parent_entry	(GnomeUIHandler *uih, char *path, gboolean warn);
static void			  menu_toplevel_remove_data		(GnomeUIHandler *uih, MenuItemInternal *internal);
static void			  menu_toplevel_remove_item_internal	(GnomeUIHandler *uih, MenuItemInternal *internal,
									 gboolean replace);
static void			  menu_toplevel_remove_item		(GnomeUIHandler *uih, char *path);
static void			  menu_remote_remove_item		(GnomeUIHandler *uih, char *path);
static GnomeUIHandlerMenuItem	 *menu_toplevel_fetch			(GnomeUIHandler *uih, char *path);
static GnomeUIHandlerMenuItem	 *menu_remote_fetch			(GnomeUIHandler *uih, char *path);
static GList			 *menu_toplevel_get_children		(GnomeUIHandler *uih, char *parent_path);
static GList			 *menu_remote_get_children		(GnomeUIHandler *uih, char *parent_path);
static gint			  menu_remote_get_pos			(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_set_sensitivity_internal (GnomeUIHandler *uih, MenuItemInternal *internal,
									 gboolean sensitivity);
static void			  menu_toplevel_set_sensitivity		(GnomeUIHandler *uih, char *path, gboolean sensitive);

static void			  menu_remote_set_sensitivity		(GnomeUIHandler *uih, char *path, gboolean sensitive);
static gboolean			  menu_remote_get_sensitivity		(GnomeUIHandler *uih, char *path);
static void			  menu_remote_set_label			(GnomeUIHandler *uih, char *path, gchar *label_text);
static gchar			 *menu_toplevel_get_label		(GnomeUIHandler *uih, char *path);
static gchar			 *menu_remote_get_label			(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_set_hint		(GnomeUIHandler *uih, char *path, char *hint);
static void			  menu_remote_set_hint			(GnomeUIHandler *uih, char *path, char *hint);
static gchar			 *menu_toplevel_get_hint		(GnomeUIHandler *uih, char *path);
static gchar			 *menu_remote_get_hint			(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_set_pixmap		(GnomeUIHandler *uih, char *path,
				 					 GnomeUIHandlerPixmapType type, gpointer data);
static void			  menu_remote_set_pixmap		(GnomeUIHandler *uih, char *path,
				 					 GnomeUIHandlerPixmapType type, gpointer data);
static void			  menu_toplevel_get_pixmap		(GnomeUIHandler *uih, char *path,
				 					 GnomeUIHandlerPixmapType *type, gpointer *data);
static void			  menu_remote_get_pixmap		(GnomeUIHandler *uih, char *path,
				 					 GnomeUIHandlerPixmapType *type, gpointer *data);
static void			  menu_toplevel_set_accel		(GnomeUIHandler *uih, char *path,
				 					 guint accelerator_key, GdkModifierType ac_mods);
static void			  menu_remote_set_accel			(GnomeUIHandler *uih, char *path,
				 					 guint accelerator_key, GdkModifierType ac_mods);
static void			  menu_toplevel_get_accel		(GnomeUIHandler *uih, char *path,
				 					 guint *accelerator_key, GdkModifierType *ac_mods);
static void			  menu_remote_get_accel			(GnomeUIHandler *uih, char *path,
				 					 guint *accelerator_key, GdkModifierType *ac_mods);
static void			  menu_local_set_callback		(GnomeUIHandler *uih, char *path,
				 					 GnomeUIHandlerCallbackFunc callback,
				 					 gpointer callback_data);
static void			  menu_local_get_callback		(GnomeUIHandler *uih, char *path,
									 GnomeUIHandlerCallbackFunc *callback,
									 gpointer *callback_data);
static gboolean			  menu_toplevel_get_sensitivity		(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_set_toggle_state_internal(GnomeUIHandler *uih, MenuItemInternal *internal,
				 					 gboolean state);
static void			  menu_toplevel_set_toggle_state	(GnomeUIHandler *uih, char *path, gboolean state);
static gboolean			  menu_toplevel_get_toggle_state	(GnomeUIHandler *uih, char *path);
static void			  menu_toplevel_set_radio_state_internal (GnomeUIHandler *uih, MenuItemInternal *internal,
				 					 gboolean state);
static void			  menu_toplevel_set_radio_state		(GnomeUIHandler *uih, char *path, gboolean state);
static void			  menu_remote_set_radio_state		(GnomeUIHandler *uih, char *path, gboolean state); 

/*
 * Prototypes for some internal Toolbar functions.
 */
static void			  toolbar_toplevel_item_create_widgets	(GnomeUIHandler *uih, ToolbarItemInternal *internal);
static void			  toolbar_toplevel_item_override_notify (GnomeUIHandler *uih, char *path);

static void			  toolbar_local_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerToolbarItem *item);
static void			  toolbar_remote_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerToolbarItem *item);
static void			  toolbar_toplevel_create_item		(GnomeUIHandler *uih, char *parent_path,
									 GnomeUIHandlerToolbarItem *item,
									 GNOME_UIHandler uih_corba);
static void			  toolbar_parse_uiinfo_one		(GnomeUIHandlerToolbarItem *item, GnomeUIInfo *uii);
static void			  toolbar_parse_uiinfo_tree		(GnomeUIHandlerToolbarItem *tree, GnomeUIInfo *uii);
static void			  toolbar_parse_uiinfo_one_with_data	(GnomeUIHandlerToolbarItem *item,
									 GnomeUIInfo *uii, void *data);
static void			  toolbar_parse_uiinfo_tree_with_data	(GnomeUIHandlerToolbarItem *tree,
									 GnomeUIInfo *uii, void *data);

/*
 * Menu CORBA prototypes.
 */
static void			  impl_register_containee		(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_Environment *ev);
static void			  impl_unregister_containee		(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_Environment *ev);
static GNOME_UIHandler		  impl_get_toplevel			(PortableServer_Servant servant,
									 CORBA_Environment *ev);

static void			  impl_menu_create			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path,
									 GNOME_UIHandler_MenuType menu_type,
									 CORBA_char *label, CORBA_char *hint,
									 CORBA_long pos,
									 GNOME_UIHandler_PixmapType pixmap_type,
									 GNOME_UIHandler_iobuf *pixmap_data,
									 CORBA_unsigned_long accelerator_key,
									 CORBA_long modifier,
									 CORBA_Environment *ev);
static void			  impl_menu_remove			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path, CORBA_Environment *ev);
static CORBA_boolean		  impl_menu_fetch			(PortableServer_Servant servant, CORBA_char *path,
									 GNOME_UIHandler_MenuType *type, CORBA_char **label,
									 CORBA_char **hint, CORBA_long *pos,
									 GNOME_UIHandler_PixmapType *pixmap_type,
									 GNOME_UIHandler_iobuf **pixmap_data,
									 CORBA_unsigned_long *accelerator_key,
									 CORBA_long *modifier,
									 CORBA_Environment *ev);
static CORBA_boolean		  impl_menu_get_children		(PortableServer_Servant servant,
									 CORBA_char *parent_path,
									 GNOME_UIHandler_StringSeq **child_paths,
									 CORBA_Environment *ev);
static CORBA_long		  impl_menu_get_pos			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_set_sensitivity		(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path, CORBA_boolean sensitive,
									 CORBA_Environment *ev);
static CORBA_boolean		  impl_menu_get_sensitivity		(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static CORBA_char		 *impl_menu_get_label			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_set_label			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path, CORBA_char *label,
									 CORBA_Environment *ev);
static void			  impl_menu_set_hint			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path, CORBA_char *hint,
									 CORBA_Environment *ev);
static CORBA_char		 *impl_menu_get_hint			(PortableServer_Servant servant,
									 CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_set_pixmap			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path,
									 GNOME_UIHandler_PixmapType corba_pixmap_type,
									 GNOME_UIHandler_iobuf *corba_pixmap_data,
									 CORBA_Environment *ev);
static void			  impl_menu_get_pixmap			(PortableServer_Servant servant,
									 CORBA_char *path,
									 GNOME_UIHandler_PixmapType *corba_pixmap_type,
									 GNOME_UIHandler_iobuf **corba_pixmap_data,
									 CORBA_Environment *ev);
static void			  impl_menu_set_accel			(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path, CORBA_long accelerator_key,
									 CORBA_long ac_mods, CORBA_Environment *ev);
static void			  impl_menu_get_accel			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_long *accelerator_key, CORBA_long *ac_mods,
									 CORBA_Environment *ev);
static void			  impl_menu_set_toggle_state		(PortableServer_Servant servant,
									 GNOME_UIHandler containee,
									 CORBA_char *path, CORBA_boolean state,
									 CORBA_Environment *ev);
static CORBA_boolean		  impl_menu_get_toggle_state		(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_activated			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_removed			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_overridden			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);
static void			  impl_menu_reinstated			(PortableServer_Servant servant, CORBA_char *path,
									 CORBA_Environment *ev);

static void			  impl_toolbar_create			(PortableServer_Servant servant,
									 GNOME_UIHandler containee,
									 CORBA_char *name, CORBA_Environment *ev);

static void			  impl_toolbar_create_item		(PortableServer_Servant servant,
									 GNOME_UIHandler containee_uih,
									 CORBA_char *path,
									 GNOME_UIHandler_ToolbarType menu_type,
									 CORBA_char *label,
									 CORBA_char *hint,
									 CORBA_long pos,
									 GNOME_UIHandler_PixmapType pixmap_type,
									 GNOME_UIHandler_iobuf *pixmap_data,
									 CORBA_unsigned_long accelerator_key,
									 CORBA_long modifier,
									 CORBA_Environment *ev);

static void			  impl_toolbar_overridden		(PortableServer_Servant servant,
									 CORBA_char *path,
									 CORBA_Environment *ev);

/*
 * Basic GtkObject management.
 *
 * These are the GnomeUIHandler construction/deconstruction functions.
 */
static CORBA_Object
create_gnome_ui_handler (GnomeObject *object)
{
	POA_GNOME_UIHandler *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_UIHandler *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_ui_handler_vepv;

	CORBA_exception_init (&ev);

	POA_GNOME_UIHandler__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return gnome_object_activate_servant (object, servant);
}

static void
gnome_ui_handler_destroy (GtkObject *object)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (object);

	/* FIXME: Fill me in */

	GTK_OBJECT_CLASS (gnome_ui_handler_parent_class)->destroy (object);
}

static void
init_ui_handler_corba_class (void)
{
	/*
	 * The entry point vectors for the GNOME::UIHandler class.
	 */

	/* General server management. */
	gnome_ui_handler_epv.register_containee = impl_register_containee;
	gnome_ui_handler_epv.unregister_containee = impl_unregister_containee;
	gnome_ui_handler_epv.get_toplevel = impl_get_toplevel;

	/* Menu management. */
	gnome_ui_handler_epv.menu_create = impl_menu_create;
	gnome_ui_handler_epv.menu_remove = impl_menu_remove;
	gnome_ui_handler_epv.menu_fetch = impl_menu_fetch;
	gnome_ui_handler_epv.menu_get_children = impl_menu_get_children;
	gnome_ui_handler_epv.menu_get_pos = impl_menu_get_pos;
	gnome_ui_handler_epv.menu_set_sensitivity = impl_menu_set_sensitivity;
	gnome_ui_handler_epv.menu_get_sensitivity = impl_menu_get_sensitivity;
	gnome_ui_handler_epv.menu_set_label = impl_menu_set_label;
	gnome_ui_handler_epv.menu_get_label = impl_menu_get_label;
	gnome_ui_handler_epv.menu_set_hint = impl_menu_set_hint;
	gnome_ui_handler_epv.menu_get_hint = impl_menu_get_hint;
	gnome_ui_handler_epv.menu_set_pixmap = impl_menu_set_pixmap;
	gnome_ui_handler_epv.menu_get_pixmap = impl_menu_get_pixmap;
	gnome_ui_handler_epv.menu_set_accel = impl_menu_set_accel;
	gnome_ui_handler_epv.menu_get_accel = impl_menu_get_accel;
	gnome_ui_handler_epv.menu_set_toggle_state = impl_menu_set_toggle_state;
	gnome_ui_handler_epv.menu_get_toggle_state = impl_menu_get_toggle_state;
	
	/* Menu notification. */
	gnome_ui_handler_epv.menu_activated = impl_menu_activated;
	gnome_ui_handler_epv.menu_removed = impl_menu_removed;
	gnome_ui_handler_epv.menu_overridden = impl_menu_overridden;
	gnome_ui_handler_epv.menu_reinstated = impl_menu_reinstated;

	/* Toolbar functions. */
	gnome_ui_handler_epv.toolbar_create = impl_toolbar_create;
	gnome_ui_handler_epv.toolbar_create_item = impl_toolbar_create_item;
	gnome_ui_handler_epv.toolbar_overridden = impl_toolbar_overridden;

	/* Setup the vector of epvs */
	gnome_ui_handler_vepv.GNOME_Unknown_epv = &gnome_object_epv;
	gnome_ui_handler_vepv.GNOME_UIHandler_epv = &gnome_ui_handler_epv;
}

static void
gnome_ui_handler_class_init (GnomeUIHandlerClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_ui_handler_parent_class = gtk_type_class (gnome_object_get_type ());

	object_class->destroy = gnome_ui_handler_destroy;

	uih_signals [MENU_ITEM_ACTIVATED] =
		gtk_signal_new ("menu_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, menu_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [MENU_ITEM_REMOVED] =
		gtk_signal_new ("menu_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, menu_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [MENU_ITEM_OVERRIDDEN] =
		gtk_signal_new ("menu_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, menu_item_reinstated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_ACTIVATED] =
		gtk_signal_new ("toolbar_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, toolbar_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_REMOVED] =
		gtk_signal_new ("toolbar_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, toolbar_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_OVERRIDDEN] =
		gtk_signal_new ("toolbar_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, toolbar_item_reinstated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, uih_signals,
				      LAST_SIGNAL);

	init_ui_handler_corba_class ();
}

static void
gnome_ui_handler_init (GnomeObject *object)
{
}

/**
 * gnome_ui_handler_get_type:
 *
 * Returns: the GtkType corresponding to the GnomeUIHandler class.
 */
GtkType
gnome_ui_handler_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/UIHandler:1.0",
			sizeof (GnomeUIHandler),
			sizeof (GnomeUIHandlerClass),
			(GtkClassInitFunc) gnome_ui_handler_class_init,
			(GtkObjectInitFunc) gnome_ui_handler_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_ui_handler_construct:
 */
GnomeUIHandler *
gnome_ui_handler_construct (GnomeUIHandler *ui_handler, GNOME_UIHandler corba_uihandler)
{
	MenuItemLocalInternal *root_cb;
	MenuItemInternal *root;
	GList *l;

	g_return_val_if_fail (ui_handler != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (ui_handler), NULL);
	g_return_val_if_fail (corba_uihandler != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (ui_handler), corba_uihandler);

	ui_handler->top = g_new0 (GnomeUIHandlerTopLevelData, 1);

	/*
	 * Initialize the hash tables.
	 */
	ui_handler->path_to_menu_callback = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->path_to_toolbar_callback = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->path_to_toolbar_toolbar = g_hash_table_new (g_str_hash, g_str_equal);

	ui_handler->top->path_to_menu_item = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_toolbar_item = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->name_to_toolbar = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_toolbar_item_widget = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->name_to_toolbar_widget = g_hash_table_new (g_str_hash, g_str_equal);

	ui_handler->top->path_to_menu_widget = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_menu_shell = g_hash_table_new (g_str_hash, g_str_equal);

	/*
	 * Create the root menu item entry in the internal data structures so
	 * that the user doesn't have to do this himself.
	 */

	/* Create the toplevel entry. */
	root = g_new0 (MenuItemInternal, 1);
	root->uih = ui_handler;
	root->uih_corba = gnome_object_corba_objref (GNOME_OBJECT (ui_handler));
	root->sensitive = TRUE;
	l = g_list_prepend (NULL, root);
	g_hash_table_insert (ui_handler->top->path_to_menu_item, g_strdup ("/"), l);

	/* Create the local entry. */
	root_cb = g_new0 (MenuItemLocalInternal, 1);
	l = g_list_prepend (NULL, root_cb);
	g_hash_table_insert (ui_handler->path_to_menu_callback, g_strdup ("/"), l);

	return ui_handler;
}


/**
 * gnome_ui_handler_new:
 */
GnomeUIHandler *
gnome_ui_handler_new (void)
{
	GNOME_UIHandler corba_uihandler;
	GnomeUIHandler *uih;
	
	uih = gtk_type_new (gnome_ui_handler_get_type ());

	corba_uihandler = create_gnome_ui_handler (GNOME_OBJECT (uih));
	if (corba_uihandler == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (uih));
		return NULL;
	}
	

	return gnome_ui_handler_construct (uih, corba_uihandler);
}

/**
 * gnome_ui_handler_set_app:
 *
 */
void
gnome_ui_handler_set_app (GnomeUIHandler *uih, GnomeApp *app)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));

	uih->top->app = app;
}

static void
impl_register_containee (PortableServer_Servant servant,
			 GNOME_UIHandler containee_uih,
			 CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	uih_toplevel_add_containee (uih, containee_uih);
}

static GNOME_UIHandler
impl_get_toplevel (PortableServer_Servant servant,
		   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	GNOME_UIHandler ret;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), CORBA_OBJECT_NIL);

	if (uih->top_level_uih == CORBA_OBJECT_NIL)
		return gnome_object_corba_objref (GNOME_OBJECT (uih));


	ret = CORBA_Object_duplicate (uih->top_level_uih, ev);

	return ret;
}

/**
 * gnome_ui_handler_set_container:
 */
void
gnome_ui_handler_set_container (GnomeUIHandler *uih, GNOME_UIHandler container)
{
	CORBA_Environment ev;
	GNOME_UIHandler top_level;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	/*
	 * Unregister with our current top-level container, if we have one.
	 */
	gnome_ui_handler_unset_container (uih);

	/*
	 * Our container will hand us a pointer to the top-level
	 * UIHandler, with which we will communicate directly
	 * for the remainder of the session.  We never speak
	 * to our container again.
	 */
	CORBA_exception_init (&ev);

	top_level = GNOME_UIHandler_get_toplevel (container, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		gnome_object_check_env (GNOME_OBJECT (uih), (CORBA_Object) container, &ev);
	else {
		uih->top_level_uih = CORBA_Object_duplicate (top_level, &ev);

		/* Regiter with the top-level UIHandler. */
		GNOME_UIHandler_register_containee (
			uih->top_level_uih,
			gnome_object_corba_objref (GNOME_OBJECT (uih)), &ev);

		if (ev._major != CORBA_NO_EXCEPTION){
			gnome_object_check_env (
				GNOME_OBJECT (uih),
				(CORBA_Object) uih->top_level_uih, &ev);
		}

	}
	CORBA_exception_free (&ev);
}

/**
 * gnome_ui_handler_uset_container:
 */
void
gnome_ui_handler_unset_container (GnomeUIHandler *uih)
{
	CORBA_Environment ev;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	CORBA_exception_init (&ev);

	/*
	 * Unregister with an existing container, if we have one
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {

		GNOME_UIHandler_unregister_containee (
			uih->top_level_uih,
			gnome_object_corba_objref (GNOME_OBJECT (uih)), &ev);

		if (ev._major != CORBA_NO_EXCEPTION){
			gnome_object_check_env (
				GNOME_OBJECT (uih),
				(CORBA_Object) uih->top_level_uih, &ev);
		}
		
#if 0
		CORBA_Object_release (uih->top_level_uih, &ev);
#endif

		/* FIXME: Check the exception */

		uih->top_level_uih = CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);
	/* FIXME: probably I should remove all local menu item data ?*/
}

static void
uih_toplevel_add_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	CORBA_Environment ev;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	uih->top->containee_uihs = g_list_prepend (uih->top->containee_uihs, CORBA_Object_duplicate (containee, &ev));
	CORBA_exception_free (&ev);
}

typedef struct {
	GnomeUIHandler *uih;
	GNOME_UIHandler containee;
	GList *removal_list;
} removal_closure_t;

/*
 * This is a helper function used by gnome_ui_handler_remove_containee
 * to find all of the menu items associated with a given containee.
 */
static void
menu_toplevel_find_containee_items (gpointer path, gpointer value, gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	GList *l = (GList *) value;
	GList *curr;
	CORBA_Environment ev;

	/*
	 * Walk the list and add all internal items to the removal list
	 * if they match the specified containee.
	 */
	CORBA_exception_init (&ev);
	for (curr = l; curr != NULL; curr = curr->next) {
		MenuItemInternal *internal = (MenuItemInternal *) curr->data;

		if (CORBA_Object_is_equivalent (internal->uih_corba, closure->containee, &ev)) {
			closure->removal_list = g_list_prepend (closure->removal_list, internal);
		}
	}
	CORBA_exception_free (&ev);
}

static gint
menu_toplevel_prune_compare_function (gconstpointer a, gconstpointer b)
{
	MenuItemInternal *ai, *bi;
	int len_a, len_b;

	ai = (MenuItemInternal *) a;
	bi = (MenuItemInternal *) b;

	len_a = strlen (ai->item->path);
	len_b = strlen (bi->item->path);

	if (len_a > len_b)
		return -1;
	if (len_a < len_b)
		return 1;
	return 0;
}

static void
uih_toplevel_unregister_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	removal_closure_t *closure;
	GNOME_UIHandler remove_me;
	CORBA_Environment ev;
	GList *curr;

	/*
	 * Remove the containee from the list of containees.
	 */
	remove_me = NULL;
	for (curr = uih->top->containee_uihs; curr != NULL; curr = curr->next) {

		CORBA_exception_init (&ev);
		if (CORBA_Object_is_equivalent ((GNOME_UIHandler) curr->data, containee, &ev)) {
			remove_me = curr->data;
			CORBA_exception_free (&ev);
			break;
		}
		CORBA_exception_free (&ev);
	}

	if (remove_me == CORBA_OBJECT_NIL)
		return;

	uih->top->containee_uihs = g_list_remove (uih->top->containee_uihs, remove_me);
#if 0				/* FIXME: fuckity fuck */
	CORBA_exception_init (&ev);
	CORBA_Object_release (remove_me, &ev);
	CORBA_exception_free (&ev);
#endif

	/*
	 * Create a simple closure to pass some data into the removal
	 * function.
	 */
	closure = g_new0 (removal_closure_t, 1);
	closure->uih = uih;
	closure->containee = containee;

	/*
	 * Remove the menu items associated with this containee.
	 */

	/* Build a list of internal menu item structures which should be removed. */
	g_hash_table_foreach (uih->top->path_to_menu_item, menu_toplevel_find_containee_items, closure);

	/*
	 * This list may contain a subtree and its children.  In that
	 * case, we need to be sure that either (a) the children
	 * are removed from the list, or (b) the children are
	 * earlier in the list, so that they get removed first.
	 *
	 * This takes care of that.
	 */
	closure->removal_list = g_list_sort (closure->removal_list, menu_toplevel_prune_compare_function);

	/* Remove them */
	for (curr = closure->removal_list; curr != NULL; curr = curr->next)
		menu_toplevel_remove_item_internal (uih, (MenuItemInternal *) curr->data, TRUE);
	g_list_free (closure->removal_list);
	g_free (closure);

	/*
	 * FIXME: Do the same for toolbars
	 */
}

static void
uih_remote_unregister_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_unregister_containee (uih->top_level_uih, containee, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}
	
	CORBA_exception_free (&ev);
}

static void
impl_unregister_containee (PortableServer_Servant servant,
			   GNOME_UIHandler containee_uih,
			   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	uih_toplevel_unregister_containee (uih, containee_uih);
}

/**
 * gnome_ui_handler_remove_containee:
 */
void
gnome_ui_handler_remove_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		uih_remote_unregister_containee (uih, containee);
		return;
	}

	uih_toplevel_unregister_containee (uih, containee);
}

/**
 * gnome_ui_handler_set_accelgroup:
 */
void
gnome_ui_handler_set_accelgroup (GnomeUIHandler *uih, GtkAccelGroup *accelgroup)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	uih->top->accelgroup = accelgroup;
}

/**
 * gnome_ui_handler_get_accelgroup:
 */
GtkAccelGroup *
gnome_ui_handler_get_accelgroup (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return uih->top->accelgroup;
}

/*
 * This helper function replaces all instances of "/" with "\/" and
 * replaces "\" with "\\".
 */
static char *
path_escape_forward_slashes (char *str)
{
	char *p = str;
	char *new, *newp;
	char *final;

	new = g_malloc (strlen (str) * 2 + 1);
	newp = new;

	while (*p != '\0') {
		if (*p == '/') {
			*newp++ = '\\';
			*newp++ = '/';
		} else if (*p == '\\') {
			*newp++ = '\\';
			*newp++ = '\\';
		} else {
			*newp++ = *p;
		}

		p++;
	}

	*newp = '\0';

	final = g_strdup (new);
	g_free (new);
	return final;
}

/*
 * This helper function replaces all instances of "\/" with "/"
 * and all instances of "\\" with "\".
 */
static char *
path_unescape_forward_slashes (char *str)
{
	char *p = str;
	char *new, *newp;
	char *final;

	new = g_malloc (strlen (str) + 1);
	newp = new;

	while (*p != '\0') {
		if (*p == '\\' && *(p + 1) == '/') {
			*newp++ = '/';
			p++;
		} else if (*p == '\\' && *(p + 1) == '\\') {
			*newp++ = '\\';
			p++;
		} else {
			*newp++ = *p;
		}

		p++;
	}

	*newp = '\0';

	final = g_strdup (new);
	g_free (new);
	return final;
}

/*
 * The path-building routine.
 */
char *
gnome_ui_handler_build_path (char *comp1, ...)
{
	char *path;
	char *path_component;
	va_list args;

	path = NULL;
	va_start (args, comp1);
	for (path_component = comp1 ;
	     path_component != NULL;
	     path_component = va_arg (args, char *))
	{
		char *old_path = path;
		char *escaped_component = path_escape_forward_slashes (path_component);

		path = g_strconcat (old_path, "/", escaped_component);
		g_free (old_path);
	}
	va_end (args);
	
	return path;
}

/*
 * We have to manually tokenize the path (instead of using g_strsplit,
 * or a similar function) to deal with the forward-slash escaping.  So
 * we split the string on all instances of "/", except when "/" is
 * preceded by a backslash.
 */
static gchar **
path_tokenize (char *path)
{
	int num_toks;
	int tok_idx;
	gchar **toks;
	char *p, *q;

	/*
	 * Count the number of tokens.
	 */
	num_toks = 1;
	q = NULL;
	for (p = path; *p != '\0'; p ++) {
		if (*p == '/' && (q == NULL || *q != '\\'))
			num_toks ++;
		q = p;
	}

	/*
	 * Allocate space for the token array.
	 */
	toks = g_new0 (gchar *, num_toks + 1);

	/*
	 * Fill it.
	 */
	tok_idx = 0;
	q = NULL;
	for (p = path; *p != '\0'; p ++) {

		if (*p == '/' && (q == NULL || *q != '\\')) {
			int tok_len;

			tok_len = p - path;
			toks [tok_idx] = g_malloc0 (tok_len + 1);
			strncpy (toks [tok_idx], path, tok_len);
			path = p + 1;
			tok_idx ++;
		}

		q = p;
	}

	/*
	 * Copy the last bit.
	 */
	if (*path != '\0') {
		int tok_len;

		tok_len = p - path;

		toks [tok_idx] = g_new0 (gchar, tok_len + 1);
		strncpy (toks [tok_idx], path, tok_len);
	}

	return toks;
}

/*
 * Given a string, this small helper function returns a
 * newly-allocated copy of the string with all of the underscore
 * characters removed.  It is used to generate a menu item path out of
 * the item's label.
 */
static gchar *
remove_ulines (const char *str)
{
	char *retval;
	const char *p;
	char *q;

	retval = g_new0 (char, strlen (str) + 1);

	q = retval;
	for (p = str; *p != '\0'; p ++) {
		if (*p != '_')
			*q++ = *p;
	}

	return retval;
}

static gchar *
path_get_parent (char *path)
{
	char *parent_path;
	char **toks;
	int i;

	toks = path_tokenize (path);

	parent_path = NULL;
	i = 0;
	while (toks [i + 1] != NULL) {
		char *new_path;

		if (parent_path == NULL)
			new_path = g_strdup (toks [i]);
		else
			new_path = g_strconcat (parent_path, "/", toks[i], NULL);

		g_free (parent_path);
		parent_path = new_path;

		i++;
	}

	if ((parent_path == NULL) || strlen (parent_path) == 0) {
		g_free (parent_path);
		parent_path = g_strdup ("/");
	}

	/*
	 * Free the token array.
	 */
	for (i = 0; toks [i] != NULL; i ++) {
		g_free (toks [i]);
	}
	g_free (toks);


	return parent_path;
} 

static GtkWidget *
uih_toplevel_create_pixmap (GtkWidget *window, GnomeUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
{
	GtkWidget *pixmap;
	char *name;

	pixmap = NULL;

	switch (pixmap_type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		break;

	case GNOME_UI_HANDLER_PIXMAP_STOCK:
		pixmap = gnome_stock_pixmap_widget (window, pixmap_info);
		break;

	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
		name = gnome_pixmap_file (pixmap_info);

		if (name == NULL)
			g_warning ("Could not find GNOME pixmap file %s",
				   (char *) pixmap_info);
		else {
			pixmap = gnome_pixmap_new_from_file (name);
		}

		g_free (name);
		break;

	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		if (pixmap_info != NULL)
			pixmap = gnome_pixmap_new_from_xpm_d (pixmap_info);
		break;

	case GNOME_UI_HANDLER_PIXMAP_RGB_DATA:
	case GNOME_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("Unsupported pixmap type (RGB[A]_DATA)\n");
		break;

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		
	}

	return pixmap;
}

static void
pixmap_free_data (GnomeUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
{
	int num_lines, i;

	switch (pixmap_type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		break;

	case GNOME_UI_HANDLER_PIXMAP_STOCK:
	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
		g_free (pixmap_info);
		break;

	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		pixmap_xpm_get_length (pixmap_info, &num_lines);

		for (i = 0; i < num_lines; i ++)
			g_free (((char **) pixmap_info) [i]);

		g_free (pixmap_info);
		break;

	case GNOME_UI_HANDLER_PIXMAP_RGB_DATA:
	case GNOME_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("Unsupported pixmap type (RGB[A]_DATA)\n");
		break;

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		
	}
}

static gpointer 
pixmap_copy_data (GnomeUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
{
	switch (pixmap_type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case GNOME_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup ((char *) pixmap_info);

	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
		return g_strdup ((char *) pixmap_info);

	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		return pixmap_xpm_copy_data (pixmap_info);

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		return NULL;
	}
}

static GNOME_UIHandler_PixmapType
pixmap_type_to_corba (GnomeUIHandlerPixmapType type)
{
	switch (type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		return GNOME_UIHandler_PixmapTypeNone;
	case GNOME_UI_HANDLER_PIXMAP_STOCK:
		return GNOME_UIHandler_PixmapTypeStock;
	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
		return GNOME_UIHandler_PixmapTypeFilename;
	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		return GNOME_UIHandler_PixmapTypeXPMData;
	case GNOME_UI_HANDLER_PIXMAP_RGB_DATA:
		return GNOME_UIHandler_PixmapTypeRGBData;
	case GNOME_UI_HANDLER_PIXMAP_RGBA_DATA:
		return GNOME_UIHandler_PixmapTypeRGBAData;
	default:
		g_warning ("pixmap_type_to_corba: Unknown pixmap type [%d]!\n", (int) type);
		return GNOME_UIHandler_PixmapTypeNone;
	}
}


static GnomeUIHandlerPixmapType
pixmap_corba_to_type (GNOME_UIHandler_PixmapType type)
{
	switch (type) {
	case GNOME_UIHandler_PixmapTypeNone:
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	case GNOME_UIHandler_PixmapTypeStock:
		return GNOME_UI_HANDLER_PIXMAP_STOCK;
	case GNOME_UIHandler_PixmapTypeFilename:
		return GNOME_UI_HANDLER_PIXMAP_FILENAME;
	case GNOME_UIHandler_PixmapTypeXPMData:
		return GNOME_UI_HANDLER_PIXMAP_XPM_DATA;
	case GNOME_UIHandler_PixmapTypeRGBData:
		return GNOME_UI_HANDLER_PIXMAP_RGB_DATA;
	case GNOME_UIHandler_PixmapTypeRGBAData:
		return GNOME_UI_HANDLER_PIXMAP_RGBA_DATA;
	default:
		g_warning ("pixmap_corba_to_type: Unknown pixmap type [%d]!\n", (int) type);
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	}
}

static gint
pixmap_xpm_get_length (gpointer data, int *num_lines)
{
	int width, height, num_colors, chars_per_pixel;
	char **lines;
	int length;
	int i;

	lines = (char **) data;

	sscanf (lines[0], "%i %i %i %i", &width, &height, &num_colors, &chars_per_pixel);

	*num_lines = height + num_colors + 1;

	length = 0;
	for (i = 0; i < *num_lines; i ++)
		length += strlen (lines [i]) + 1;

	return length;
}

static gpointer
pixmap_xpm_copy_data (gpointer src)
{
	int num_lines;
	char **dest;
	int i;

	pixmap_xpm_get_length (src, &num_lines);
	dest = g_new0 (char *, num_lines);

	/*
	 * Copy the XPM data into the destination buffer.
	 */
	for (i = 0; i < num_lines; i ++)
		dest [i] = g_strdup (((char **) src) [i]);

	return (gpointer) dest;
}

/*
 * XPM data in its normal, "unflattened" form is usually an array of
 * strings.  This converts normal XPM data to a completely flat
 * sequence of characters which can be transmitted over CORBA.
 */
static gpointer
pixmap_xpm_flatten (char **src, int *length)
{
	int num_lines;
	int dest_offset;
	char *flat;
	int i;

	*length = pixmap_xpm_get_length (src, &num_lines);

	flat = g_malloc0 (*length + 1);

	dest_offset = 0;
	for (i = 0; i < num_lines; i ++) {
		int line_len;
		char *line;

		line = ((char **) src) [i];
		line_len = strlen (line);

		memcpy (flat + dest_offset, line, line_len + 1);
		dest_offset += line_len + 1;
	}

	/*
	 * Put an extra null at the end of the flattened version.
	 */
	flat [*length] = '\0';

	return flat;
}

/*
 * After a flattened XPM file has been received via CORBA, it can be
 * converted to the normal, unflattened form with this function.
 */
static gpointer
pixmap_xpm_unflatten (char *src, int length)
{
	gboolean just_hit_end;
	GList *line_copies, *curr;
	char **unflattened;
	int num_lines, i;
	char *p;

	/*
	 * Count the number of lines in the flattened buffer.  Store
	 * copies of each line.
	 */
	num_lines = 0;
	line_copies = NULL;
	just_hit_end = TRUE;
	for (p = src; ((p[0] != '\0') || (p[1] != '\0')) && ((p - src) < length); p ++) {
		if (just_hit_end) {
			line_copies = g_list_append (line_copies, g_strdup (p));
			just_hit_end = FALSE;
		}
		
		if (*p == '\0') {
			num_lines ++;
			just_hit_end = TRUE;
		}
	}

	num_lines ++;

	unflattened = g_new (char *, num_lines);
	for (curr = line_copies, i = 0; curr != NULL; curr = curr->next, i ++) {
		unflattened [i] = curr->data;
	}

	g_list_free (line_copies);

	return unflattened;
}

static GNOME_UIHandler_iobuf *
pixmap_data_to_corba (GnomeUIHandlerPixmapType type, gpointer data)
{
	GNOME_UIHandler_iobuf *buffer;
	gpointer temp_xpm_buffer;

	buffer = GNOME_UIHandler_iobuf__alloc ();
	CORBA_sequence_set_release (buffer, TRUE);

	switch (type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;

	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
	case GNOME_UI_HANDLER_PIXMAP_STOCK:
		g_warning ("Marshalling pixmap filename across CORBA!\n");
		buffer->_length = strlen ((char *) data) + 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (strlen ((char *) data));
		strcpy (buffer->_buffer, (char *) data);
		return buffer;

	case GNOME_UI_HANDLER_PIXMAP_RGB_DATA:
	case GNOME_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("pixmap_data_to_corba: Pixmap type (RGB[A]) not yet supported!\n");
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;

	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		temp_xpm_buffer = pixmap_xpm_flatten (data, &(buffer->_length));
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (buffer->_length);
		memcpy (buffer->_buffer, temp_xpm_buffer, buffer->_length);
		g_free (temp_xpm_buffer);
		return buffer;

	default:
		g_warning ("pixmap_data_to_corba: Unknown pixmap type [%d]\n", type);
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;
	}

	return buffer;
}

static gpointer
pixmap_corba_to_data (GNOME_UIHandler_PixmapType corba_pixmap_type,
		      GNOME_UIHandler_iobuf *corba_pixmap_data)
{
	GnomeUIHandlerPixmapType type;
	gpointer pixmap_data;

	type = pixmap_corba_to_type (corba_pixmap_type);

	switch (type) {
	case GNOME_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case GNOME_UI_HANDLER_PIXMAP_FILENAME:
	case GNOME_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup (corba_pixmap_data->_buffer);

	case GNOME_UI_HANDLER_PIXMAP_RGB_DATA:
	case GNOME_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("pixmap_corba_to_data: Pixmap type (RGB[A]) not yet supported!\n");
		return NULL;

	case GNOME_UI_HANDLER_PIXMAP_XPM_DATA:
		pixmap_data = pixmap_xpm_unflatten (corba_pixmap_data->_buffer,
						    corba_pixmap_data->_length);
		return pixmap_data;

	default:
		g_warning ("pixmap_corba_to_data: Unknown pixmap type [%d]\n", type);
		return NULL;
	}
}

static GnomeUIHandlerPixmapType
uiinfo_pixmap_type_to_uih (GnomeUIPixmapType ui_type)
{
	switch (ui_type) {
	case GNOME_APP_PIXMAP_NONE:
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	case GNOME_APP_PIXMAP_STOCK:
		return GNOME_UI_HANDLER_PIXMAP_STOCK;
	case GNOME_APP_PIXMAP_FILENAME:
		return GNOME_UI_HANDLER_PIXMAP_FILENAME;
	case GNOME_APP_PIXMAP_DATA:
		return GNOME_UI_HANDLER_PIXMAP_XPM_DATA;
	default:
		g_warning ("Unknown GnomeUIPixmapType: %d\n", ui_type);
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	}
}

/*
 *
 * Menus.
 *
 * This section contains the implementations for the Menu item
 * manipulation class methods and the CORBA menu manipulation
 * interface methods.
 *
 */

static GnomeUIHandlerMenuItem *
menu_make_item (char *path, GnomeUIHandlerMenuItemType type,
		char *label, char *hint,
		int pos, GnomeUIHandlerPixmapType pixmap_type,
		gpointer pixmap_data,
		guint accelerator_key, GdkModifierType ac_mods,
		GnomeUIHandlerCallbackFunc callback,
		gpointer callback_data)

{
	GnomeUIHandlerMenuItem *item;

	item = g_new0 (GnomeUIHandlerMenuItem, 1);
	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->pos = pos;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods = ac_mods;
	item->callback = callback;
	item->callback_data = callback_data;

	return item;
}

static GnomeUIHandlerMenuItem *
menu_copy_item (GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *copy;	

	copy = g_new0 (GnomeUIHandlerMenuItem, 1);

	copy->path = COPY_STRING (item->path);
	copy->type = item->type;
	copy->hint = COPY_STRING (item->hint);
	copy->pos = item->pos;
	copy->label = COPY_STRING (item->label);
	copy->children = NULL; /* FIXME: Make sure this gets handled properly. */

	copy->pixmap_data = pixmap_copy_data (item->pixmap_type, item->pixmap_data);
	copy->pixmap_type = item->pixmap_type;

	copy->accelerator_key = item->accelerator_key;
	copy->ac_mods = item->ac_mods;
	copy->callback = item->callback;
	copy->callback_data = item->callback_data;

	return copy;
}

static void
menu_free_data (GnomeUIHandlerMenuItem *item)
{
	g_free (item->path);
	item->path = NULL;

	g_free (item->label);
	item->label = NULL;
	
	g_free (item->hint);
	item->hint = NULL;
	
	pixmap_free_data (item->pixmap_type, item->pixmap_data);
}

static void
menu_free (GnomeUIHandlerMenuItem *item)
{
	menu_free_data (item);
	g_free (item);
}

static GNOME_UIHandler_MenuType
menu_type_to_corba (GnomeUIHandlerMenuItemType type)
{
	switch (type) {

	case GNOME_UI_HANDLER_MENU_END:
		g_warning ("Passing MenuTypeEnd through CORBA!\n");
		return GNOME_UIHandler_MenuTypeEnd;

	case GNOME_UI_HANDLER_MENU_ITEM:
		return GNOME_UIHandler_MenuTypeItem;

	case GNOME_UI_HANDLER_MENU_SUBTREE:
		return GNOME_UIHandler_MenuTypeSubtree;

	case GNOME_UI_HANDLER_MENU_RADIOITEM:
		return GNOME_UIHandler_MenuTypeRadioItem;

	case GNOME_UI_HANDLER_MENU_RADIOGROUP:
		return GNOME_UIHandler_MenuTypeRadioGroup;

	case GNOME_UI_HANDLER_MENU_TOGGLEITEM:
		return GNOME_UIHandler_MenuTypeToggleItem;

	case GNOME_UI_HANDLER_MENU_SEPARATOR:
		return GNOME_UIHandler_MenuTypeSeparator;

	default:
		g_warning ("Unknown GnomeUIHandlerMenuItemType %d!\n", (int) type);
		return GNOME_UIHandler_MenuTypeItem;
		
	}
}

static GnomeUIHandlerMenuItemType
menu_corba_to_type (GNOME_UIHandler_MenuType type)
{
	switch (type) {
	case GNOME_UIHandler_MenuTypeEnd:
		g_warning ("Warning: Getting MenuTypeEnd from CORBA!");
		return GNOME_UI_HANDLER_MENU_END;

	case GNOME_UIHandler_MenuTypeItem:
		return GNOME_UI_HANDLER_MENU_ITEM;

	case GNOME_UIHandler_MenuTypeSubtree:
		return GNOME_UI_HANDLER_MENU_SUBTREE;

	case GNOME_UIHandler_MenuTypeRadioItem:
		return GNOME_UI_HANDLER_MENU_RADIOITEM;

	case GNOME_UIHandler_MenuTypeRadioGroup:
		return GNOME_UI_HANDLER_MENU_RADIOGROUP;

	case GNOME_UIHandler_MenuTypeToggleItem:
		return GNOME_UI_HANDLER_MENU_TOGGLEITEM;

	case GNOME_UIHandler_MenuTypeSeparator:
		return GNOME_UI_HANDLER_MENU_SEPARATOR;
	default:
		g_warning ("Unknown GNOME_UIHandler menu type %d!\n", (int) type);
		return GNOME_UI_HANDLER_MENU_ITEM;
		
	}
}
/**
 * gnome_ui_handler_create_menubar:
 */
void
gnome_ui_handler_create_menubar (GnomeUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uih->top->app != NULL);
	g_return_if_fail (GNOME_IS_APP (uih->top->app));

	uih->top->menubar = gtk_menu_bar_new ();

	gnome_app_set_menus (uih->top->app, GTK_MENU_BAR (uih->top->menubar));
}

/**
 * gnome_ui_handler_set_menubar:
 */
void
gnome_ui_handler_set_menubar (GnomeUIHandler *uih, GtkWidget *menubar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (menubar);
	g_return_if_fail (GTK_IS_MENU_BAR (menubar));

	uih->top->menubar = menubar;
}

/**
 * gnome_ui_handler_get_menubar:
 */
GtkWidget *
gnome_ui_handler_get_menubar (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return uih->top->menubar;
}

/**
 * gnome_ui_handler_create_popup_menu:
 */
void
gnome_ui_handler_create_popup_menu (GnomeUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	uih->top->menubar = gtk_menu_new ();
}

static void
menu_toplevel_popup_deactivated (GtkMenuShell *menu_shell, gpointer data)
{
	gtk_main_quit ();
}

/**
 * gnome_ui_handler_do_popup_menu:
 */
void
gnome_ui_handler_do_popup_menu (GnomeUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	gtk_menu_popup (GTK_MENU (uih->top->menubar), NULL, NULL, NULL, NULL, 0,
			GDK_CURRENT_TIME);

	gtk_signal_connect (GTK_OBJECT (uih->top->menubar), "deactivate",
			    GTK_SIGNAL_FUNC (menu_toplevel_popup_deactivated),
			    NULL);

	gtk_main ();
}

/**
 * gnome_ui_handler_set_statusbar:
 */
void
gnome_ui_handler_set_statusbar (GnomeUIHandler *uih, GtkWidget *statusbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (statusbar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (statusbar) || GNOME_IS_APPBAR (statusbar));

	uih->top->statusbar = statusbar;
}

/**
 * gnome_ui_handler_get_statusbar:
 */
GtkWidget *
gnome_ui_handler_get_statusbar (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return uih->top->statusbar;
}

static void
uih_local_do_path (char *parent_path, char *item_label, char **item_path)
{
	gchar **parent_toks;
	gchar **item_toks;
	int i;

	parent_toks = path_tokenize (parent_path);
	item_toks = NULL;

	/*
	 * If there is a path set on the item already, make sure it
	 * matches the parent path.
	 */
	if (*item_path != NULL) {
		int i;
		int paths_match = TRUE;
	
		item_toks = path_tokenize (*item_path);

		for (i = 0; parent_toks [i] != NULL; i ++) {
			if (item_toks [i] == NULL || strcmp (item_toks [i], parent_toks [i])) {
				paths_match = FALSE;
				break;
			}
		}

		/*
		 * Make sure there is exactly one more element in the
		 * item_toks array.
		 */
		if (paths_match) {
			if (item_toks [i] == NULL || item_toks [i + 1] != NULL)
				paths_match = FALSE;
		}

		if (! paths_match)
			g_warning ("uih_local_do_path: Item path [%s] does not jibe with parent path [%s]!\n",
				   *item_path, parent_path);
	}

	/*
	 * Build a path for the item.
	 */
	if (*item_path == NULL) {
		char *path_component;

		/*
		 * If the item has a label, then use the label
		 * as the path component.
		 */
		if (item_label != NULL) {
			char *tmp;

			tmp = remove_ulines (item_label);
			path_component = path_escape_forward_slashes (tmp);
			g_free (tmp);
		} else {

			/*
			 * FIXME: This is an ugly hack to give us
			 * a unique path for an item with no label.
			 */
			path_component = g_new0 (char, 32);
			snprintf (path_component, 32, "%lx", (unsigned long) ((char *) path_component + rand () % 10000));
		}

		if (parent_path [strlen (parent_path) - 1] == '/')
			*item_path = g_strconcat (parent_path, path_component, NULL);
		else
			*item_path = g_strconcat (parent_path, "/", path_component, NULL);

		g_free (path_component);
	}

	/*
	 * Free the token lists.
	 */
	if (item_toks != NULL) {
		for (i = 0; item_toks [i] != NULL; i ++)
			g_free (item_toks [i]);
		g_free (item_toks);
	}

	for (i = 0; parent_toks [i] != NULL; i ++)
		g_free (parent_toks [i]);
	g_free (parent_toks);
}

/*
 * This function checks to make sure that the path of a given
 * GnomeUIHandlerMenuItem is consistent with the path of its parent.
 * If the item does not have a path, one is created for it.  If the
 * path is not consistent with the parent's path, an error is printed
 */
static void
menu_local_do_path (char *parent_path, GnomeUIHandlerMenuItem *item)
{
	uih_local_do_path (parent_path, item->label, & item->path);
}


/*
 * Callback data is maintained in a stack, like toplevel internal menu
 * item data.  This is so that a local UIHandler can override its
 * own menu items.
 */
static MenuItemLocalInternal *
menu_local_get_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_menu_callback, path);
	if (l == NULL)
		return NULL;

	return (MenuItemLocalInternal *) l->data;
}

static void
menu_local_create_item (GnomeUIHandler *uih, char *parent_path,
			GnomeUIHandlerMenuItem *item)
{
	MenuItemLocalInternal *internal_cb;
	GList *l, *new_list;

	/*
	 * If this item doesn't have a path set, set one for it.
	 */
	menu_local_do_path (parent_path, item);

	/*
	 * Create a data structure for locally storing this menu item.
	 */
	internal_cb = g_new0 (MenuItemLocalInternal, 1);
	internal_cb->callback = item->callback;
	internal_cb->callback_data = item->callback_data;

	/*
	 * Add this item to the list.
	 */
	l = g_hash_table_lookup (uih->path_to_menu_callback, item->path);
	new_list = g_list_prepend (l, internal_cb);

	/*
	 * Store it.
	 */
	if (l == NULL)
		g_hash_table_insert (uih->path_to_menu_callback, g_strdup (item->path), new_list);
	else
		g_hash_table_insert (uih->path_to_menu_callback, item->path, new_list);


	menu_local_add_parent_entry (uih, item->path);
}

static void
menu_local_remove_parent_entry (GnomeUIHandler *uih, char *path, gboolean warn)
{
	MenuItemLocalInternal *parent;
	char *parent_path;
	GList *curr;

	parent_path = path_get_parent (path);
	parent = menu_local_get_item (uih, parent_path);
	g_free (parent_path);

	if (parent == NULL)
		return;

	for (curr = parent->children; curr != NULL; curr = curr->next) {
		if (! strcmp (path, (char *) curr->data)) {
			parent->children = g_list_remove_link (parent->children, curr);
			g_free (curr->data);
			g_list_free_1 (curr);
			return;
		}
			 
	}

	if (warn)
		g_warning ("menu_local_remove_parent_entry: No entry in parent for child path [%s]!\n", path);
}

static void
menu_local_add_parent_entry (GnomeUIHandler *uih, char *path)
{
	MenuItemLocalInternal *parent_internal_cb;
	char *parent_path;

	menu_local_remove_parent_entry (uih, path, FALSE);

	parent_path = path_get_parent (path);
	parent_internal_cb = menu_local_get_item (uih, parent_path);
	g_free (parent_path);

	if (parent_internal_cb == NULL) {
		/*
		 * If we don't have an entry for the parent,
		 * it doesn't matter.
		 */
		return;
	}

	parent_internal_cb->children = g_list_prepend (parent_internal_cb->children, g_strdup (path));
}

static void
menu_local_remove_item (GnomeUIHandler *uih, char *path)
{
	MenuItemLocalInternal *internal_cb;
	GList *l, *new_list, *curr;

	/*
	 * Don't remove "/" or the user will have to recreate it.
	 */
	if (! strcmp (path, "/"))
		return;
	
	l = g_hash_table_lookup (uih->path_to_menu_callback, path);

	if (l == NULL)
		return;

	/*
	 * Free the MenuItemLocalInternal structure.
	 */
	internal_cb = (MenuItemLocalInternal *) l->data;
	for (curr = internal_cb->children; curr != NULL; curr = curr->next) {
		g_free ((char *) curr->data);
	}
	g_list_free (internal_cb->children);
	g_free (internal_cb);

	/*
	 * Remove the list link.
	 */
	new_list = g_list_remove_link (l, l);
	g_list_free_1 (l);

	/*
	 * If there are no items left in the list, remove the hash
	 * table entry.
	 */
	if (new_list == NULL) {
		char *orig_key;

		g_hash_table_lookup_extended (uih->path_to_menu_callback, path, (gpointer *) &orig_key, NULL);
		g_hash_table_remove (uih->path_to_menu_callback, path);
		g_free (orig_key);

		menu_local_remove_parent_entry (uih, path, TRUE);
	} else
		g_hash_table_insert (uih->path_to_menu_callback, path, new_list);
	
}

static void
menu_local_remove_item_recursive (GnomeUIHandler *uih, char *path)
{
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Cannot recursive remove menu item with path [%s]; item does not exist!\n", path);
		return;
	}

	/*
	 * Remove this item's children.
	 */
	while (internal_cb->children != NULL)
		menu_local_remove_item_recursive (uih, (char *) internal_cb->children);

	/*
	 * Remove the item itself.
	 */
	menu_local_remove_item (uih, path);
}


/*
 * This function grabs the current active menu item associated with a
 * given menu path string.
 */
static MenuItemInternal *
menu_toplevel_get_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_menu_item, path);

	if (l == NULL)
		return NULL;

	return (MenuItemInternal *) l->data;
}

static MenuItemInternal *
menu_toplevel_get_item_for_containee (GnomeUIHandler *uih, char *path, GNOME_UIHandler containee_uih)
{
	GList *l, *curr;

	l = g_hash_table_lookup (uih->top->path_to_menu_item, path);

	for (curr = l; curr != NULL; curr = curr->next) {
		MenuItemInternal *internal;
		CORBA_Environment ev;

		internal = (MenuItemInternal *) curr->data;

		CORBA_exception_init (&ev);

		if (CORBA_Object_is_equivalent (containee_uih, internal->uih_corba, &ev)) {
			CORBA_exception_free (&ev);

			return internal;
		}

		CORBA_exception_free (&ev);
	}

	return NULL;
}

static gboolean
menu_toplevel_item_is_head (GnomeUIHandler *uih, MenuItemInternal *internal)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_menu_item, internal->item->path);

	if (l->data == internal)
		return TRUE;

	return FALSE;
}

static gboolean
uih_toplevel_check_toplevel (GnomeUIHandler *uih)
{
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		g_warning ("CORBA method invoked on non-toplevel UIHandler!\n");
		return FALSE;
	}

	return TRUE;
}

static GtkWidget *
menu_toplevel_get_widget (GnomeUIHandler *uih, char *path)
{
	return g_hash_table_lookup (uih->top->path_to_menu_widget, path);
}

/*
 * This function maps a path to either a subtree menu shell or the
 * top-level menu bar.  It returns NULL if the specified path is
 * not a subtree path (or "/").
 */
static GtkWidget *
menu_toplevel_get_shell (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;
	char *parent_path;

	if (! strcmp (path, "/"))
		return uih->top->menubar;

	internal = menu_toplevel_get_item (uih, path);
	if (internal->item->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		GtkMenuShell *menu_shell;
		parent_path = path_get_parent (path);

		menu_shell =  g_hash_table_lookup (uih->top->path_to_menu_shell, parent_path);
		g_free (parent_path);

		return GTK_WIDGET (menu_shell);
	}

	return g_hash_table_lookup (uih->top->path_to_menu_shell, path);
}

/*
 * This function creates the GtkLabel for a menu item and adds it to
 * the GtkMenuItem.  It also sets up the menu item's accelerators
 * appropriately.
 */
static GtkWidget *
menu_toplevel_create_label (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item,
			    GtkWidget *parent_menu_shell_widget, GtkWidget *menu_widget)
{
	GtkWidget *label;
	guint keyval;

	/*
	 * Create the label, translating the provided text, which is
	 * supposed to be untranslated.  This text gets translated in
	 * the domain of the application.
	 */
	label = gtk_accel_label_new (gettext (item->label));

	/*
	 * Setup the widget.
	 */
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	/*
	 * Insert it into the menu item widget and setup the
	 * accelerator.
	 */
	gtk_container_add (GTK_CONTAINER (menu_widget), label);
	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_widget);

	/*
	 * Setup the underline accelerators.
	 *
	 * FIXME: Should this be an option?
	 */
	keyval = gtk_label_parse_uline (GTK_LABEL (label), item->label);

	if (keyval != GDK_VoidSymbol) {
		if (GTK_IS_MENU (parent_menu_shell_widget))
			gtk_widget_add_accelerator (menu_widget,
						    "activate_item",
						    gtk_menu_ensure_uline_accel_group (GTK_MENU (parent_menu_shell_widget)),
						    keyval, 0,
						    0);

		if (GTK_IS_MENU_BAR (parent_menu_shell_widget) && uih->top->accelgroup != NULL)
			gtk_widget_add_accelerator (menu_widget,
						    "activate_item",
						    uih->top->accelgroup,
						    keyval, GDK_MOD1_MASK,
						    0);
	}

	return label;
}

/*
 * This routine creates the GtkMenuItem widget for a menu item.
 */
static GtkWidget *
menu_toplevel_create_item_widget (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	GtkWidget *menu_widget;
	MenuItemInternal *rg;

	switch (item->type) {
	case GNOME_UI_HANDLER_MENU_ITEM:
	case GNOME_UI_HANDLER_MENU_SUBTREE:

		/*
		 * Create a GtkMenuItem widget if it's a plain item.  If it contains
		 * a pixmap, create a GtkPixmapMenuItem.
		 */
		if (item->pixmap_data != NULL && item->pixmap_type != GNOME_UI_HANDLER_PIXMAP_NONE) {
			GtkWidget *pixmap;

			menu_widget = gtk_pixmap_menu_item_new ();

			pixmap = uih_toplevel_create_pixmap (GTK_WIDGET (menu_widget), item->pixmap_type, item->pixmap_data);
			if (pixmap != NULL) {
				gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_widget),
								 GTK_WIDGET (pixmap));
				gtk_widget_show (GTK_WIDGET (pixmap));
			}
		} else {
			menu_widget = gtk_menu_item_new ();
		}

		break;

	case GNOME_UI_HANDLER_MENU_RADIOITEM:
		/*
		 * The parent path should be the path to the radio
		 * group lead item.
		 */
		rg = menu_toplevel_get_item (uih, parent_path);

		/*
		 * Create the new radio menu item and add it to the
		 * radio group.
		 */
		menu_widget = gtk_radio_menu_item_new (rg->radio_items);
		rg->radio_items = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_widget));

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_widget), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_widget), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_TOGGLEITEM:
		menu_widget = gtk_check_menu_item_new ();

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_widget), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_widget), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_SEPARATOR:

		/*
		 * A separator is just a plain menu item with its sensitivity
		 * set to FALSE.
		 */
		menu_widget = gtk_menu_item_new ();
		gtk_widget_set_sensitive (menu_widget, FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_RADIOGROUP:
		g_assert_not_reached ();
		return NULL;

	default:
		g_warning ("menu_toplevel_create_item_widget: Invalid GnomeUIHandlerMenuItemType %d\n",
			   (int) item->type);
		return NULL;
			
	}

	if (uih->top->accelgroup == NULL)
		gtk_widget_lock_accelerators (menu_widget);

	/*
	 * Create a label for the menu item.
	 */
	if (item->label != NULL) {
		GtkWidget *parent_menu_shell;

		parent_menu_shell = menu_toplevel_get_shell (uih, parent_path);

		/* Create the label. */
		menu_toplevel_create_label (uih, item, parent_menu_shell, menu_widget);
	}

	/*
	 * Add the hint for this menu item.
	 */
	if (item->hint != NULL)
		menu_toplevel_create_hint (uih, item, menu_widget);
	
	gtk_widget_show (menu_widget);

	return menu_widget;
}

static gint
menu_toplevel_save_accels (gpointer data)
{
	gchar *file_name;

	file_name = g_concat_dir_and_file (gnome_user_accels_dir, gnome_app_id);
	gtk_item_factory_dump_rc (file_name, NULL, TRUE);
	g_free (file_name);

	return TRUE;
}

static void
menu_toplevel_install_global_accelerators (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *menu_widget)
{
	static guint save_accels_id = 0;
	char *globalaccelstr;

	g_return_if_fail (gnome_app_id != NULL);
	globalaccelstr = g_strconcat (gnome_app_id, ">", item->path, "<", NULL);
	gtk_item_factory_add_foreign (menu_widget, globalaccelstr, uih->top->accelgroup,
				      item->accelerator_key, item->ac_mods);
	g_free (globalaccelstr);

	if (! save_accels_id)
		save_accels_id = gtk_quit_add (1, menu_toplevel_save_accels, NULL);

}

/*
 * This function does all the work associated with creating a menu
 * item's GTK widgets.
 */
static void
menu_toplevel_create_widgets (GnomeUIHandler *uih, char *parent_path, MenuItemInternal *internal)
{
	GnomeUIHandlerMenuItem *item;
	GtkWidget *parent_menu_shell_widget;
	GtkWidget *menu_widget;

	item = internal->item;

	/* No widgets to create for a radio group. */
	if (item->type == GNOME_UI_HANDLER_MENU_RADIOGROUP)
		return;
	
	/*
	 * Make sure that the parent exists before creating the item.
	 */
	parent_menu_shell_widget = menu_toplevel_get_shell (uih, parent_path);
	g_return_if_fail (parent_menu_shell_widget != NULL);

	/*
	 * Create the GtkMenuItem widget for this item, and stash it
	 * in the hash table.
	 */
	menu_widget = menu_toplevel_create_item_widget (uih, parent_path, item);
	g_hash_table_insert (uih->top->path_to_menu_widget, g_strdup (item->path), menu_widget);

	/*
	 * Insert the new GtkMenuItem into the menu shell of the
	 * parent.
	 */
	gtk_menu_shell_insert (GTK_MENU_SHELL (parent_menu_shell_widget), menu_widget, internal->item->pos);

	/*
	 * If it's a subtree, create the menu shell.
	 */
	if (item->type == GNOME_UI_HANDLER_MENU_SUBTREE) {
		GtkMenu *menu;
		GtkWidget *tearoff;

		/* Create the menu shell. */
		menu = GTK_MENU (gtk_menu_new ());

		/* Setup the accelgroup for this menu shell. */
		gtk_menu_set_accel_group (menu, uih->top->accelgroup);

		/*
		 * Create the tearoff item at the beginning of the menu shell,
		 * if appropriate.
		 */
		if (gnome_preferences_get_menus_have_tearoff ()) {
			tearoff = gtk_tearoff_menu_item_new ();
			gtk_widget_show (tearoff);
			gtk_menu_prepend (GTK_MENU (menu), tearoff);
		}

		/*
		 * Associate this menu shell with the menu item for
		 * this submenu.
		 */
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget), GTK_WIDGET (menu));

		/*
		 * Store the menu shell for later retrieval (e.g. when
		 * we add submenu items into this shell).
		 */
		g_hash_table_insert (uih->top->path_to_menu_shell, g_strdup (item->path), menu);
	}

	/*
	 * Install the global accelerators.
	 */
	menu_toplevel_install_global_accelerators (uih, item, menu_widget);

	/*
	 * Connect to the "activate" signal for this menu item.
	 */
	menu_toplevel_connect_signal (menu_widget, internal);

	/*
	 * Set the sensitivity of the menu item.
	 */
	menu_toplevel_set_sensitivity_internal (uih, internal, internal->sensitive);

	/*
	 * Set the item's active state.
	 */
	if (internal->item->type == GNOME_UI_HANDLER_MENU_TOGGLEITEM)
		menu_toplevel_set_toggle_state_internal (uih, internal, internal->active);
	else if (internal->item->type == GNOME_UI_HANDLER_MENU_RADIOITEM)
		menu_toplevel_set_radio_state_internal (uih, internal, internal->active);
}

static void
menu_toplevel_connect_signal (GtkWidget *menu_widget, MenuItemInternal *internal)
{
	gtk_signal_connect (GTK_OBJECT (menu_widget), "activate",
			    GTK_SIGNAL_FUNC (menu_toplevel_item_activated), internal);
}

static void
menu_toplevel_create_widgets_recursive (GnomeUIHandler *uih, MenuItemInternal *internal)
{
	char *parent_path;
	GList *curr;

	parent_path = path_get_parent (internal->item->path);

	/*
	 * Create the widgets for this menu item.
	 */
	menu_toplevel_create_widgets (uih, parent_path, internal);

	g_free (parent_path);

	/*
	 * Recurse into its children.
	 */
	for (curr = internal->children; curr != NULL; curr = curr->next) {
		MenuItemInternal *child;

		child = menu_toplevel_get_item (uih, (char *) curr->data);
		menu_toplevel_create_widgets_recursive (uih, child);
	}
}

/*
 * This callback puts the hint for a menu item into the GnomeAppBar
 * when the menu item is selected.
 */
static void
menu_toplevel_put_hint_in_appbar (GtkWidget *menu_item, gpointer data)
{
  gchar *hint = gtk_object_get_data (GTK_OBJECT (menu_item),
                                     "menu_item_hint");
  GtkWidget *bar = GTK_WIDGET (data);

  g_return_if_fail (bar != NULL);
  g_return_if_fail (GNOME_IS_APPBAR (bar));

  if (hint == NULL)
	  return;

  gnome_appbar_set_status (GNOME_APPBAR (bar), hint);
}

/*
 * This callback removes a menu item's hint from the GnomeAppBar
 * when the menu item is deselected.
 */
static void
menu_toplevel_remove_hint_from_appbar (GtkWidget *menu_item, gpointer data)
{
  GtkWidget *bar = GTK_WIDGET (data);

  g_return_if_fail (bar != NULL);
  g_return_if_fail (GNOME_IS_APPBAR(bar));

  gnome_appbar_refresh (GNOME_APPBAR (bar));
}

/*
 * This callback puts a menu item's hint into a GtkStatusBar when the
 * menu item is selected.
 */
static void
menu_toplevel_put_hint_in_statusbar (GtkWidget *menu_item, gpointer data)
{
	gchar *hint = gtk_object_get_data (GTK_OBJECT (menu_item), "menu_item_hint");
	GtkWidget *bar = GTK_WIDGET (data);
	guint id;
	
	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (bar));

	if (hint == NULL)
		return;

	id = gtk_statusbar_get_context_id (GTK_STATUSBAR (bar), "gnome-ui-handler:menu-hint");
	
	gtk_statusbar_push (GTK_STATUSBAR (bar), id, hint);
}

/*
 * This callback removes a menu item's hint from a GtkStatusBar when
 * the menu item is deselected.
 */
static void
menu_toplevel_remove_hint_from_statusbar (GtkWidget *menu_item, gpointer data)
{
	GtkWidget *bar = GTK_WIDGET (data);
	guint id;

	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (bar));

	id = gtk_statusbar_get_context_id (GTK_STATUSBAR (bar), "gnome-ui-handler:menu-hint");
	
	gtk_statusbar_pop (GTK_STATUSBAR (bar), id);
}

static void
menu_toplevel_create_hint (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *menu_item)
{
	char *old_hint;

	old_hint = gtk_object_get_data (GTK_OBJECT (menu_item), "menu_item_hint");
	g_free (old_hint);

	/* FIXME: Do we have to i18n this here? */
	if (item->hint == NULL)
		gtk_object_set_data (GTK_OBJECT (menu_item), "menu_item_hint", NULL);
	else
		gtk_object_set_data (GTK_OBJECT (menu_item), "menu_item_hint", g_strdup (item->hint));

	if (uih->top->statusbar == NULL)
		return;

	if (GTK_IS_STATUSBAR (uih->top->statusbar)) {
		gtk_signal_connect (GTK_OBJECT (menu_item), "select",
				    GTK_SIGNAL_FUNC (menu_toplevel_put_hint_in_statusbar),
				    uih->top->statusbar);
		gtk_signal_connect (GTK_OBJECT (menu_item), "deselect",
				    GTK_SIGNAL_FUNC (menu_toplevel_remove_hint_from_statusbar),
				    uih->top->statusbar);

		return;
	}

	if (GNOME_IS_APPBAR (uih->top->statusbar)) {
		gtk_signal_connect (GTK_OBJECT (menu_item), "select",
				    GTK_SIGNAL_FUNC (menu_toplevel_put_hint_in_appbar),
				    uih->top->statusbar);
		gtk_signal_connect (GTK_OBJECT (menu_item), "deselect",
				    GTK_SIGNAL_FUNC (menu_toplevel_remove_hint_from_appbar),
				    uih->top->statusbar);

		return;
	}
}

/*
 * This function is the callback through which all GtkMenuItem
 * activations pass.  If a user selects a menu item, this function is
 * the first to know about it.
 */
static gint
menu_toplevel_item_activated (GtkWidget *menu_item, gpointer data)
{
	MenuItemInternal *internal = (MenuItemInternal *) data;

	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_activated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (internal->uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}
	
	CORBA_exception_free (&ev);

	return FALSE;
}

static void
impl_menu_activated (PortableServer_Servant servant,
		     CORBA_char *path,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received activation notification about a menu item I don't own [%s]!\n", path);
		return;
	}

	if (internal_cb->callback != NULL)
		(*internal_cb->callback) (uih, internal_cb->callback_data, path);
	
	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_ACTIVATED], path, internal_cb->callback_data);
}

/*
 * This routine stores the top-level UIHandler's internal data about a
 * given menu item.
 */
static MenuItemInternal *
menu_toplevel_store_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal;
	CORBA_Environment ev;
	char *parent_path;
	GList *l;

	/*
	 * Make sure the parent exists.
	 */
	parent_path = path_get_parent (item->path);
	if (menu_toplevel_get_item (uih, parent_path) == NULL) {
		g_free (parent_path);
		return NULL;
	}
	g_free (parent_path);

	/*
	 * Create the internal representation of the menu item.
	 */
	internal = g_new0 (MenuItemInternal, 1);
	internal->item = menu_copy_item (item);
	internal->uih = uih;
	internal->sensitive = TRUE;
	internal->active = FALSE;

	CORBA_exception_init (&ev);
	internal->uih_corba = CORBA_Object_duplicate (uih_corba, &ev);
	CORBA_exception_free (&ev);

	/*
	 * Place this new menu item structure at the front of the linked
	 * list of menu items.
	 *
	 * We maintain a linked list so that we can handle overriding
	 * properly.  The front-most item in the list is the current
	 * active menu item.  When it is destroyed, it is removed from
	 * the list, and the next item in the list is the current,
	 * active menu item.
	 */
	l = g_hash_table_lookup (uih->top->path_to_menu_item, internal->item->path);

	if (l != NULL) {
		l = g_list_prepend (l, internal);
		g_hash_table_insert (uih->top->path_to_menu_item, internal->item->path, l);
	} else {
		l = g_list_prepend (NULL, internal);
		g_hash_table_insert (uih->top->path_to_menu_item, g_strdup (internal->item->path), l);
	}

	/*
	 * Now insert a child record into this item's parent, if it has
	 * a parent.
	 */
	menu_toplevel_add_parent_entry (uih, item);

	return internal;
}

static void
menu_toplevel_override_notify_recursive (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;
	GList *curr;

	/*
	 * Send out an override notification for this menu item.
	 */
	menu_toplevel_override_notify (uih, path);

	/*
	 * Now send out notifications for its children.
	 */
	internal = menu_toplevel_get_item (uih, path);

	for (curr = internal->children; curr != NULL; curr = curr->next)
		menu_toplevel_override_notify_recursive (uih, (char *) curr->data);
}

static void
menu_toplevel_override_notify (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;
	CORBA_Environment ev;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_overridden (internal->uih_corba, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (internal->uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_overridden (PortableServer_Servant servant,
		     CORBA_char *path,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received override notification for a menu item I don't own [%s]!\n", path);
		return;
	}
	
	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_OVERRIDDEN], path, internal_cb->callback_data);
}

/*
 * This function is called to check if a new menu item is going to
 * override an existing one.  If it does, then the old (overridden)
 * menu item's widgets must be destroyed to make room for the new
 * item. The "menu_item_overridden" signal is propagated down from the
 * top-level UIHandler to the appropriate containee.
 */
static void
menu_toplevel_check_override (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	/*
	 * Check for an existing menu item with the specified path.
	 */
	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL)
		return;

	/*
	 * There is an existing menu item with this path.  We must
	 * override it.
	 *
	 * Remove the item's widgets, the item's childrens' widgets,
	 * and notify the owner of each overridden item that it has
	 * been overridden.
	 */
	menu_toplevel_remove_widgets_recursive (uih, path);

	/*
	 * Notify the owner of each item that it was overridden.
	 */
	menu_toplevel_override_notify_recursive (uih, path);
}

static void
menu_toplevel_create_item (GnomeUIHandler *uih, char *parent_path,
			   GnomeUIHandlerMenuItem *item, GNOME_UIHandler uih_corba)
{
	MenuItemInternal *internal_data;

	/*
	 * Check to see if there is already a menu item by this name,
	 * and if so, override it.
	 */
	menu_toplevel_check_override (uih, item->path);

	/*
	 * Duplicate the GnomeUIHandlerMenuItem structure and store
	 * an internal representation of the menu item.
	 */
	internal_data = menu_toplevel_store_data (uih, uih_corba, item);

	if (internal_data == NULL)
		return;

	/*
	 * Create the menu item's widgets.
	 */
	menu_toplevel_create_widgets (uih, parent_path, internal_data);
}

static void
menu_remote_create_item (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	GNOME_UIHandler_iobuf *pixmap_buf;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	pixmap_buf = pixmap_data_to_corba (item->pixmap_type, item->pixmap_data);

	GNOME_UIHandler_menu_create (uih->top_level_uih,
				     gnome_object_corba_objref (GNOME_OBJECT (uih)),
				     item->path,
				     menu_type_to_corba (item->type),
				     CORBIFY_STRING (item->label),
				     CORBIFY_STRING (item->hint),
				     item->pos,
				     pixmap_type_to_corba (item->pixmap_type),
				     pixmap_buf,
				     (CORBA_unsigned_long) item->accelerator_key,
				     (CORBA_long) item->ac_mods,
				     &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buf);
}

static void
impl_menu_create (PortableServer_Servant servant,
		  GNOME_UIHandler containee_uih,
		  CORBA_char *path,
		  GNOME_UIHandler_MenuType menu_type,
		  CORBA_char *label,
		  CORBA_char *hint,
		  CORBA_long pos,
		  GNOME_UIHandler_PixmapType pixmap_type,
		  GNOME_UIHandler_iobuf *pixmap_data,
		  CORBA_unsigned_long accelerator_key,
		  CORBA_long modifier,
		  CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	GnomeUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	item = menu_make_item (path, menu_corba_to_type (menu_type),
			       UNCORBIFY_STRING (label),
			       UNCORBIFY_STRING (hint),
			       pos,
			       pixmap_corba_to_type (pixmap_type),
			       pixmap_corba_to_data (pixmap_type, pixmap_data),
			       (guint) accelerator_key, (GdkModifierType) modifier,
			       NULL, NULL);

	parent_path = path_get_parent (path);
	g_return_if_fail (parent_path != NULL);

	menu_toplevel_create_item (uih, parent_path, item, containee_uih);

	pixmap_free_data (item->pixmap_type, item->pixmap_data);

	g_free (item);
	g_free (parent_path);
}

/**
 * gnome_ui_handler_menu_add_one:
 */
void
gnome_ui_handler_menu_add_one (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Do any local work that must be done for this menu item,
	 * including storing the local callback data.
	 */
	menu_local_create_item (uih, parent_path, item);

	/*
	 * If we are not a top-level UIHandler, propagate
	 * the menu-item creation to the top-level.
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_create_item (uih, parent_path, item);
		return;
	}

	/*
	 * We are the top-level UIHandler, and so we must create the
	 * menu's widgets and so forth.
	 */
	menu_toplevel_create_item (uih, parent_path, item, gnome_object_corba_objref (GNOME_OBJECT (uih)));
}

/**
 * gnome_ui_handler_menu_add_list:
 */
void
gnome_ui_handler_menu_add_list (GnomeUIHandler *uih, char *parent_path,
				GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; curr->type != GNOME_UI_HANDLER_MENU_END; curr ++)
		gnome_ui_handler_menu_add_tree (uih, parent_path, curr);
}

/**
 * gnome_ui_handler_menu_add_tree:
 */
void
gnome_ui_handler_menu_add_tree (GnomeUIHandler *uih, char *parent_path,
				GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Add this menu item.
	 */
	gnome_ui_handler_menu_add_one (uih, parent_path, item);

	/*
	 * Recursive add its children.
	 */
	if (item->children != NULL)
		gnome_ui_handler_menu_add_list (uih, item->path, item->children);
}

/**
 * gnome_ui_handler_menu_new:
 */
void
gnome_ui_handler_menu_new (GnomeUIHandler *uih, char *path,
			   GnomeUIHandlerMenuItemType type,
			   char *label, char *hint,
			   int pos, GnomeUIHandlerPixmapType pixmap_type,
			   gpointer pixmap_data, guint accelerator_key,
			   GdkModifierType ac_mods, GnomeUIHandlerCallbackFunc callback,
			   gpointer callback_data)
{
	GnomeUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	item = menu_make_item (path, type, label, hint, pos,
			       pixmap_type, pixmap_data, accelerator_key, ac_mods,
			       callback, callback_data);

	parent_path = path_get_parent (path);
	g_return_if_fail (parent_path != NULL);

	gnome_ui_handler_menu_add_one (uih, parent_path, item);

	g_free (item);
	g_free (parent_path);
}

/**
 * gnome_ui_handler_menu_new_item:
 */
void
gnome_ui_handler_menu_new_item (GnomeUIHandler *uih, char *path,
				char *label, char *hint, int pos,
				GnomeUIHandlerPixmapType pixmap_type,
				gpointer pixmap_data, guint accelerator_key,
				GdkModifierType ac_mods,
				GnomeUIHandlerCallbackFunc callback,
				gpointer callback_data)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_ITEM,
				   label, hint, pos, pixmap_type,
				   pixmap_data, accelerator_key, ac_mods,
				   callback, callback_data);
}

/**
 * gnome_ui_handler_menu_new_subtree:
 */
void
gnome_ui_handler_menu_new_subtree (GnomeUIHandler *uih, char *path,
				   char *label, char *hint, int pos,
				   GnomeUIHandlerPixmapType pixmap_type,
				   gpointer pixmap_data, guint accelerator_key,
				   GdkModifierType ac_mods)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_SUBTREE,
				   label, hint, pos, pixmap_type, pixmap_data,
				   accelerator_key, ac_mods, NULL, NULL);
}

/**
 * gnome_ui_handler_menu_new_separator:
 */
void
gnome_ui_handler_menu_new_separator (GnomeUIHandler *uih, char *path, int pos)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_SEPARATOR,
				   NULL, NULL, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

/**
 * gnome_ui_handler_menu_new_radiogroup:
 */
void
gnome_ui_handler_menu_new_radiogroup (GnomeUIHandler *uih, char *path)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_RADIOGROUP,
				   NULL, NULL, -1, GNOME_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

/**
 * gnome_ui_handler_menu_radioitem:
 */
void
gnome_ui_handler_menu_new_radioitem (GnomeUIHandler *uih, char *path,
				     char *label, char *hint, int pos,
				     guint accelerator_key, GdkModifierType ac_mods,
				     GnomeUIHandlerCallbackFunc callback,
				     gpointer callback_data)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_RADIOITEM,
				   label, hint, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

/**
 * gnome_ui_handler_menu_new_toggleitem:
 */
void
gnome_ui_handler_menu_new_toggleitem (GnomeUIHandler *uih, char *path,
				      char *label, char *hint, int pos,
				      guint accelerator_key, GdkModifierType ac_mods,
				      GnomeUIHandlerCallbackFunc callback,
				      gpointer callback_data)
{
	gnome_ui_handler_menu_new (uih, path, GNOME_UI_HANDLER_MENU_TOGGLEITEM,
				   label, hint, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

static void
menu_toplevel_remove_widgets (GnomeUIHandler *uih, char *path)
{
	GtkWidget *menu_widget;
	GtkWidget *menu_shell_widget;
	char *orig_key;

	/*
	 * Destroy the old menu item widget.
	 */
	menu_widget = menu_toplevel_get_widget (uih, path);
	g_return_if_fail (menu_widget != NULL);

	gtk_widget_destroy (menu_widget);

	/* Free the path string that was stored in the hash table. */
	g_hash_table_lookup_extended (uih->top->path_to_menu_widget, path,
				      (gpointer *) &orig_key, NULL);
	g_hash_table_remove (uih->top->path_to_menu_widget, path);
	g_free (orig_key);

	/*
	 * If this item is a subtree, then its menu shell was already
	 * destroyed (the GtkMenuItem destroy function handles that).
	 * We just need to remove the menu shell widget from our hash
	 * table.
	 */
	menu_shell_widget = menu_toplevel_get_shell (uih, path);
	if (menu_shell_widget != NULL) {
		g_hash_table_lookup_extended (uih->top->path_to_menu_shell, path,
					      (gpointer *) &orig_key, NULL);
		g_hash_table_remove (uih->top->path_to_menu_shell, path);
		g_free (orig_key);
	}
}

static void
menu_toplevel_remove_widgets_recursive (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	/*
	 * If this menu item is a subtree, recurse out to the leaves.
	 */
	internal = menu_toplevel_get_item (uih, path);
	if (internal->children != NULL) {
		GList *curr;

		for (curr = internal->children; curr != NULL; curr = curr->next)
			menu_toplevel_remove_widgets_recursive (uih, (char *) curr->data);
	}

	/*
	 * Remove the widgets for this menu item.
	 */
	menu_toplevel_remove_widgets (uih, path);
}

/*
 * Adds an entry to an item's parent's list of children.
 */
static void
menu_toplevel_add_parent_entry (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *parent;
	char *parent_path;
	int max_pos;

	parent_path = path_get_parent (item->path);
	parent = menu_toplevel_get_item (uih, parent_path);

	if (parent == NULL) {
		g_warning ("menu_toplevel_store_data: Parent [%s] does not exist for path [%s]!\n",
			   parent_path, item->path);
		g_free (parent_path);
		return;
	}

	g_free (parent_path);

	/*
	 * If there is already an entry for this child in the parent's
	 * list of children, we remove it.
	 */
	menu_toplevel_remove_parent_entry (uih, item->path, FALSE);

	/*
	 * Now insert an entry into the parent in the proper place,
	 * depending on the item's position.
	 */
	max_pos = g_list_length (parent->children);

	if ((item->pos > max_pos) || (item->pos == -1))
		item->pos = max_pos;

	parent->children = g_list_insert (parent->children, g_strdup (item->path), item->pos);
}

/*
 * The job of this function is to remove a child's entry from its
 * parent's list of children.
 */
static void
menu_toplevel_remove_parent_entry (GnomeUIHandler *uih, char *path, gboolean warn)
{
	MenuItemInternal *parent;
	char *parent_path;
	gboolean child_found;
	GList *curr, *next;
	int pos;

	parent_path = path_get_parent (path);
	parent = menu_toplevel_get_item (uih, parent_path);
	g_free (parent_path);

	/*
	 * Remove the entry from the parent list.
	 */
	pos = 0;
	child_found = FALSE;
	for (curr = parent->children; curr != NULL; curr = next) {

		next = curr->next;

		if (! strcmp (path, (char *) curr->data)) {
			parent->children = g_list_remove_link (parent->children, curr);
			g_free (curr->data);
			g_list_free_1 (curr);
			child_found = TRUE;
		} else {
			MenuItemInternal *child;

			/*
			 * Update the position for this child.
			 */
			child = menu_toplevel_get_item (uih, (char *) curr->data);
			child->item->pos = pos;
			 
			pos ++;
		}
			 
	}

	if ((! child_found) && warn)
		g_warning ("menu_toplevel_remove_parent_entry: No entry in parent for child path [%s]!\n", path);
}

static void
menu_toplevel_remove_data (GnomeUIHandler *uih, MenuItemInternal *internal)
{
	CORBA_Environment ev;
	char *orig_key;
	char *path;
	GList *curr;
	GList *l;

	g_assert (internal != NULL);

	path = g_strdup (internal->item->path);

	/*
	 * Get the list of menu items which match this path.  There
	 * may be several menu items matching the same path because
	 * each containee can create a menu item with the same path.
	 * The newest item overrides the older ones.
	 */
	g_hash_table_lookup_extended (uih->top->path_to_menu_item, path,
				      (gpointer *) &orig_key, (gpointer *) &l);
	g_hash_table_remove (uih->top->path_to_menu_item, path);
	g_free (orig_key);

	/*
	 * Remove this item from the list, and reinsert the list into
	 * the hash table, if there are remaining items.
	 */
	l = g_list_remove (l, internal);

	if (l != NULL) {
		g_hash_table_insert (uih->top->path_to_menu_item, g_strdup (path), l);
	} else {
		/*
		 * Remove this path entry from the parent's child
		 * list.
		 */
		menu_toplevel_remove_parent_entry (uih, path, TRUE);
	}

	/*
	 * Free the internal data structures associated with this
	 * item.
	 */
	CORBA_exception_init (&ev);
	CORBA_Object_release (internal->uih_corba, &ev);
	CORBA_exception_free (&ev);

	menu_free (internal->item);

	for (curr = internal->children; curr != NULL; curr = curr->next)
		g_free ((char *) curr->data);
	g_list_free (internal->children);

	g_slist_free (internal->radio_items);
	g_free (internal);
	g_free (path);
}

static void
menu_toplevel_remove_notify (GnomeUIHandler *uih, MenuItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_removed (internal->uih_corba, internal->item->path, &ev);

	CORBA_exception_free (&ev);
}

static void
impl_menu_removed (PortableServer_Servant servant,
		   CORBA_char *path,
		   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received removal notification for menu item I don't own [%s]!\n", path);
		return;
	}

	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_REMOVED],
			 path, internal_cb->callback_data);

	menu_local_remove_item (uih, path);
}

static void
menu_toplevel_reinstate_notify (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;
	CORBA_Environment ev;

	internal = menu_toplevel_get_item (uih, path);

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_reinstated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
menu_toplevel_reinstate_notify_recursive (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;
	GList *curr;

	/*
	 * Send out a reinstatement notification for this menu item.
	 */
	menu_toplevel_reinstate_notify (uih, path);

	/*
	 * Now send out notifications to the kids.
	 */
	internal = menu_toplevel_get_item (uih, path);

	for (curr = internal->children; curr != NULL; curr = curr->next)
		menu_toplevel_reinstate_notify_recursive (uih, (char *) curr->data);
}

static void
impl_menu_reinstated (PortableServer_Servant servant,
		      CORBA_char *path,
		      CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received reinstatement notification for menu item I don't own [%s]!\n", path);
		return;
	}

	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_REINSTATED],
			 path, internal_cb->callback_data);

}

static void
menu_toplevel_remove_item_internal (GnomeUIHandler *uih, MenuItemInternal *internal, gboolean replace)
{
	gboolean is_head;
	char *path;

	/*
	 * Don't remove "/" or the user will have to recreate it.  And
	 * we don't want the poor user to go to so much work.  Because
	 * the user is our boss.  We love the user.  Everything for
	 * the user.  Here I am, sacrificing my youth, my hands, and
	 * my very SOUL on the high-altar of this fucking USER I've
	 * never even fucking met!  FUCK THE USER!  FUCK HIM!  What
	 * did he ever do for me?
	 */
	if (! strcmp (internal->item->path, "/"))
		return;

	/*
	 * Check to see if this is the current active menu item.
	 */
	is_head = menu_toplevel_item_is_head (uih, internal);

	/*
	 * If this item has children, recurse to the leaves and remove
	 * all descendents.
	 */
	if (internal->children != NULL) {

		while (internal->children != NULL) {
			MenuItemInternal *child_internal;

			child_internal = menu_toplevel_get_item (uih, (char *) internal->children->data);
			menu_toplevel_remove_item_internal (uih, child_internal, FALSE);
		}
	}

	/*
	 * Keep a copy of the path around; the one in internal->item
	 * won't be valid in a moment.
	 */
	path = g_strdup (internal->item->path);

	/*
	 * Destroy the widgets associated with this item.
	 */
	if (is_head &&
	    internal->item->type != GNOME_UI_HANDLER_MENU_RADIOGROUP)
		menu_toplevel_remove_widgets (uih, path);

	/*
	 * Notify the item's owner that the item is being removed.
	 * This is important in the case where one containee removes a
	 * subtree containing a menu item created by another
	 * containee.
	 */
	menu_toplevel_remove_notify (uih, internal);

	/*
	 * Free the internal data structures associated with this
	 * item.
	 */
	menu_toplevel_remove_data (uih, internal);

	/*
	 * If we have just destroyed the currently active menu item
	 * (the one at the head of the queue), and there is a
	 * replacement, and we have been instructed to activate the
	 * replacement, then we create the replacement here.
	 */
	if (is_head && replace) {
		MenuItemInternal *replacement;
		
		replacement = menu_toplevel_get_item (uih, path);

		if (replacement == NULL) {
			g_free (path);
			return;
		}

		/*
		 * Notify this item's owner that the item is being
		 * reinstated.
		 */
		menu_toplevel_reinstate_notify_recursive (uih, replacement->item->path);

		menu_toplevel_create_widgets_recursive (uih, replacement);
	}

	g_free (path);
}

static void
menu_toplevel_remove_item (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	menu_toplevel_remove_item_internal (uih, internal, TRUE);
}

static void
menu_remote_remove_item (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_remove (uih->top_level_uih,
				     gnome_object_corba_objref (GNOME_OBJECT (uih)), path,
				     &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_remove (PortableServer_Servant servant,
		  GNOME_UIHandler containee_uih,
		  CORBA_char *path,
		  CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	/*
	 * Find the menu item belonging to this containee.
	 */
	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);
	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	/*
	 * Remove it.
	 */
	menu_toplevel_remove_item_internal (uih, internal, TRUE);
}

/**
 * gnome_ui_handler_menu_remove:
 */
void
gnome_ui_handler_menu_remove (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are not a top-level UIHandler, propagate
	 * the menu-item removal to the toplevel.
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_remove_item (uih, path);
		return;
	}

	/*
	 * We are the top-level UIHandler, and so must remove the menu
	 * item's widgets and associated internal data.
	 */
	menu_toplevel_remove_item (uih, path);
}

static GnomeUIHandlerMenuItem *
menu_toplevel_fetch (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL)
		return NULL;

	return menu_copy_item (internal->item);
}

static GnomeUIHandlerMenuItem *
menu_remote_fetch (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *item;
	CORBA_Environment ev;
	CORBA_boolean exists;

	GNOME_UIHandler_MenuType corba_menu_type;
	CORBA_char *corba_label;
	CORBA_char *corba_hint;
	CORBA_long corba_pos;
	GNOME_UIHandler_PixmapType corba_pixmap_type;
	GNOME_UIHandler_iobuf *corba_pixmap_iobuf;
	CORBA_long corba_accelerator_key;
	CORBA_long corba_modifier_type;

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	CORBA_exception_init (&ev);

	exists = GNOME_UIHandler_menu_fetch (uih->top_level_uih,
					     path, &corba_menu_type, &corba_label,
					     &corba_hint, &corba_pos,
					     &corba_pixmap_type, &corba_pixmap_iobuf,
					     &corba_accelerator_key, &corba_modifier_type,
					     &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	if (exists == FALSE) {
		g_free (item);
		return NULL;
	}

	item->path = g_strdup (path);
	item->label = g_strdup (corba_label);
	item->hint = g_strdup (corba_hint);
	item->pos = corba_pos;
	item->type = menu_corba_to_type (corba_menu_type);
	item->pixmap_type = pixmap_corba_to_type (corba_pixmap_type),
	item->pixmap_data = g_memdup (corba_pixmap_iobuf->_buffer, corba_pixmap_iobuf->_length);
	item->accelerator_key = corba_accelerator_key;
	item->ac_mods = (GdkModifierType) (corba_modifier_type);

	CORBA_free (corba_hint);
	CORBA_free (corba_label);
	CORBA_free (corba_pixmap_iobuf);

	return item;
}

static CORBA_boolean
impl_menu_fetch (PortableServer_Servant servant,
		 CORBA_char *path,
		 GNOME_UIHandler_MenuType *type,
		 CORBA_char **label,
		 CORBA_char **hint,
		 CORBA_long *pos,
		 GNOME_UIHandler_PixmapType *pixmap_type,
		 GNOME_UIHandler_iobuf **pixmap_data,
		 CORBA_unsigned_long *accelerator_key,
		 CORBA_long *modifier,
		 CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	GnomeUIHandlerMenuItem *item;
	MenuItemInternal *internal;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), FALSE);

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL)
		return FALSE;

	item = internal->item;

	*type = menu_type_to_corba (item->type);
	*label = CORBA_string_dup (CORBIFY_STRING (item->label));
	*hint = CORBA_string_dup (CORBIFY_STRING (item->hint));
	*pos = item->pos;
	*accelerator_key = (CORBA_unsigned_long) item->accelerator_key;
	*modifier = (CORBA_long) item->ac_mods;

	return TRUE;
}

/**
 * gnome_ui_handler_menu_fetch_one:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_fetch_one (GnomeUIHandler *uih, char *path)
{
	MenuItemLocalInternal *internal_cb;
	GnomeUIHandlerMenuItem *item;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	/*
	 * If we are not the top-level UIHandler, ask the top-level
	 * for the information.
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		item = menu_remote_fetch (uih, path);
	else
		item = menu_toplevel_fetch (uih, path);

	if (item == NULL)
		return NULL;

	/*
	 * Fetch the callback data locally.
	 */
	internal_cb = menu_local_get_item (uih, path);
	item->callback = internal_cb->callback;
	item->callback_data = internal_cb->callback_data;

	return item;
}

/**
 * gnome_ui_handler_menu_fetch_tree:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_fetch_tree (GnomeUIHandler *uih, char *path)
{
	/* FIXME: Implement me */
	g_error ("Implement fetch tree\n");
	return NULL;
}

/**
 * gnome_ui_handler_menu_free_one:
 */
void
gnome_ui_handler_menu_free_one (GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (item != NULL);

	menu_free (item);
}

/**
 * gnome_ui_handler_menu_free_list:
 * @array: A NULL-terminated array of GnomeUIHandlerMenuItem structures to be freed.
 *
 * Frees the list of menu items in @array and frees the array itself.
 */
void
gnome_ui_handler_menu_free_list (GnomeUIHandlerMenuItem *array)
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (array != NULL);

	for (curr = array; curr->type != GNOME_UI_HANDLER_MENU_END; curr ++) {
		if (curr->children != NULL)
			gnome_ui_handler_menu_free_list (curr->children);

		menu_free_data (curr);
	}

	g_free (array);
}

/**
 * gnome_ui_handler_menu_free_tree:
 */
void
gnome_ui_handler_menu_free_tree (GnomeUIHandlerMenuItem *tree)
{
	g_return_if_fail (tree != NULL);

	if (tree->type == GNOME_UI_HANDLER_MENU_END)
		return;

	if (tree->children != NULL)
		gnome_ui_handler_menu_free_list (tree->children);

	menu_free (tree);
}

static GList *
menu_toplevel_get_children (GnomeUIHandler *uih, char *parent_path)
{
	MenuItemInternal *internal;
	GList *children, *curr;

	internal = menu_toplevel_get_item (uih, parent_path);

	if (internal == NULL)
		return NULL;

	/*
	 * Copy the list and the strings inside it.
	 */
	children = g_list_copy (internal->children);
	for (curr = children; curr != NULL; curr = curr->next)
		curr->data = g_strdup ((char *) curr->data);

	return children;
}

static GList *
menu_remote_get_children (GnomeUIHandler *uih, char *parent_path)
{
	GNOME_UIHandler_StringSeq *childseq;
	CORBA_Environment ev;
	GList *children;
	int i;
	gboolean fail = FALSE;
	
	CORBA_exception_init (&ev);
	if (! GNOME_UIHandler_menu_get_children (uih->top_level_uih,
						 (CORBA_char *) parent_path, &childseq,
						 &ev))
		return NULL;
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (GNOME_OBJECT (uih), uih->top_level_uih, &ev);
		fail = TRUE;
	}
	
	CORBA_exception_free (&ev);
	if (fail)
		return NULL;
	
	/* FIXME: Free the sequence */
	
	children = NULL;
	for (i = 0; i < childseq->_length; i ++) {
		children = g_list_prepend (children, g_strdup (childseq->_buffer [i]));
	}
	
	return children;
}

static CORBA_boolean
impl_menu_get_children (PortableServer_Servant servant,
			CORBA_char *parent_path,
			GNOME_UIHandler_StringSeq **child_paths,
			CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;
	int num_children, i;
	GList *curr;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), FALSE);

	internal = menu_toplevel_get_item (uih, parent_path);

	if (internal == NULL)
		return FALSE;

	num_children = g_list_length (internal->children);

	*child_paths = GNOME_UIHandler_StringSeq__alloc ();
	(*child_paths)->_buffer = CORBA_sequence_CORBA_string_allocbuf (num_children);
	(*child_paths)->_length = num_children;

	for (curr = internal->children, i = 0; curr != NULL; curr = curr->next, i ++)
		(*child_paths)->_buffer [i] = CORBA_string_dup ((char *) curr->data);

	return TRUE;
}

/**
 * gnome_ui_handler_menu_get_child_paths:
 */
GList *
gnome_ui_handler_menu_get_child_paths (GnomeUIHandler *uih, char *parent_path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (parent_path != NULL, NULL);

	/*
	 * If we are not the top-level UIHandler, ask the top-level
	 * for a list of children.
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_children (uih, parent_path);

	/*
	 * We are the top-level, so we grab the list from our internal
	 * data.
	 */
	return menu_toplevel_get_children (uih, parent_path);
}

static GnomeUIHandlerMenuItemType
menu_uiinfo_type_to_uih (GnomeUIInfoType uii_type)
{
	switch (uii_type) {
	case GNOME_APP_UI_ENDOFINFO:
		return GNOME_UI_HANDLER_MENU_END;

	case GNOME_APP_UI_ITEM:
		return GNOME_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_TOGGLEITEM:
		return GNOME_UI_HANDLER_MENU_TOGGLEITEM;

	case GNOME_APP_UI_RADIOITEMS:
		return GNOME_UI_HANDLER_MENU_RADIOGROUP;

	case GNOME_APP_UI_SUBTREE:
		return GNOME_UI_HANDLER_MENU_SUBTREE;

	case GNOME_APP_UI_SEPARATOR:
		return GNOME_UI_HANDLER_MENU_SEPARATOR;

	case GNOME_APP_UI_HELP:
		g_error ("Help unimplemented."); /* FIXME */

	case GNOME_APP_UI_BUILDER_DATA:
		g_error ("Builder data - what to do?"); /* FIXME */

	case GNOME_APP_UI_ITEM_CONFIGURABLE:
		g_warning ("Configurable item!");
		return GNOME_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_SUBTREE_STOCK:
		return GNOME_UI_HANDLER_MENU_SUBTREE;

	default:
		g_warning ("Unknown UIInfo Type: %d", uii_type);
		return GNOME_UI_HANDLER_MENU_ITEM;
	}
}

static void
menu_parse_uiinfo_one (GnomeUIHandlerMenuItem *item, GnomeUIInfo *uii)
{
	item->path = NULL;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	item->type = menu_uiinfo_type_to_uih (uii->type);

	item->label = COPY_STRING (uii->label);
	item->hint = COPY_STRING (uii->hint);

	item->pos = -1;

	if (item->type == GNOME_UI_HANDLER_MENU_ITEM ||
	    item->type == GNOME_UI_HANDLER_MENU_RADIOITEM ||
	    item->type == GNOME_UI_HANDLER_MENU_TOGGLEITEM)
		item->callback = uii->moreinfo;
	item->callback_data = uii->user_data;

	item->pixmap_type = uiinfo_pixmap_type_to_uih (uii->pixmap_type);
	item->pixmap_data = pixmap_copy_data (item->pixmap_type, uii->pixmap_info);
	item->accelerator_key = uii->accelerator_key;
	item->ac_mods = uii->ac_mods;
}

static void
menu_parse_uiinfo_tree (GnomeUIHandlerMenuItem *tree, GnomeUIInfo *uii)
{
	menu_parse_uiinfo_one (tree, uii);

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE ||
	    tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		tree->children = gnome_ui_handler_menu_parse_uiinfo_list (uii->moreinfo);
	}
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_one:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_one (GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	menu_parse_uiinfo_one (item, uii);
	
	return item;
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_list:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_list (GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *list;
	GnomeUIHandlerMenuItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the GnomeUIHandlerMenuItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (GnomeUIHandlerMenuItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		menu_parse_uiinfo_tree (curr_uih, curr_uii);

	/* Parse the terminal entry. */
	menu_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_tree:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_tree (GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (GnomeUIHandlerMenuItem, 1);

	menu_parse_uiinfo_tree (item_tree, uii);

	return item_tree;
}

static void
menu_parse_uiinfo_one_with_data (GnomeUIHandlerMenuItem *item, GnomeUIInfo *uii, void *data)
{
	menu_parse_uiinfo_one (item, uii);

	item->callback_data = data;
}

static void
menu_parse_uiinfo_tree_with_data (GnomeUIHandlerMenuItem *tree, GnomeUIInfo *uii, void *data)
{
	menu_parse_uiinfo_one_with_data (tree, uii, data);

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE ||
	    tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		tree->children = gnome_ui_handler_menu_parse_uiinfo_list_with_data (uii->moreinfo, data);
	}
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_one_with_data:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_one_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerMenuItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	menu_parse_uiinfo_one_with_data (item, uii, data);

	return item;
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_list_with_data:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_list_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerMenuItem *list;
	GnomeUIHandlerMenuItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the GnomeUIHandlerMenuItem list.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (GnomeUIHandlerMenuItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		menu_parse_uiinfo_tree_with_data (curr_uih, curr_uii, data);

	/* Parse the terminal entry. */
	menu_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_tree_with_data:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_tree_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (GnomeUIHandlerMenuItem, 1);

	menu_parse_uiinfo_tree_with_data (item_tree, uii, data);

	return item_tree;
}

static gint
menu_toplevel_get_pos (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	if (internal == NULL) {
		g_warning ("menu_toplevel_get_item: No such item for path [%s]!\n", path);
		return -1;
	}

	return internal->item->pos;
}

static gint
menu_remote_get_pos (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;
	CORBA_long retval;
	gboolean fail = FALSE;
	
	CORBA_exception_init (&ev);

	retval = GNOME_UIHandler_menu_get_pos (uih->top_level_uih,
					       path,
					       &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		fail = TRUE;
	}

	CORBA_exception_free (&ev);

	if (fail)
		return -1;
	
	return (gint) retval;
}

static CORBA_long
impl_menu_get_pos (PortableServer_Servant servant,
		   CORBA_char *path,
		   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), -1);

	return (CORBA_long) menu_toplevel_get_pos (uih, (char *) path);
}


/**
 * gnome_ui_handler_menu_get_pos:
 */
int
gnome_ui_handler_menu_get_pos (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_pos (uih, path);

	return menu_toplevel_get_pos (uih, path);
}

static void
menu_toplevel_set_sensitivity_internal (GnomeUIHandler *uih, MenuItemInternal *internal, gboolean sensitivity)
{
	GtkWidget *menu_widget;

	internal->sensitive = sensitivity;

	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);
	g_return_if_fail (menu_widget != NULL);

	gtk_widget_set_sensitive (menu_widget, sensitivity);
}

static void
menu_toplevel_set_sensitivity (GnomeUIHandler *uih, char *path, gboolean sensitive)
{
	MenuItemInternal *internal;

	/* Update the internal state */
	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);


	menu_toplevel_set_sensitivity_internal (uih, internal, sensitive);
}

static void
menu_remote_set_sensitivity (GnomeUIHandler *uih, char *path, gboolean sensitive)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_sensitivity (uih->top_level_uih,
					      gnome_object_corba_objref (GNOME_OBJECT (uih)),
					      path, sensitive,
					      &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_set_sensitivity (PortableServer_Servant servant,
			   GNOME_UIHandler containee_uih,
			   CORBA_char *path,
			   CORBA_boolean sensitive,
			   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	/*
	 * Get the menu item matching this path belonging to this
	 * containee.
	 */
	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	menu_toplevel_set_sensitivity_internal (uih, internal, sensitive);
}

/**
 * gnome_ui_handler_menu_set_sensitivity:
 */
void
gnome_ui_handler_menu_set_sensitivity (GnomeUIHandler *uih, char *path, gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_sensitivity (uih, path, sensitive);
		return;
	}

	menu_toplevel_set_sensitivity (uih, path, sensitive);
}

static gboolean
menu_toplevel_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	return internal->sensitive;
}

static gboolean
menu_remote_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;
	gboolean retval;

	CORBA_exception_init (&ev);

	retval = (gboolean) GNOME_UIHandler_menu_get_sensitivity (uih->top_level_uih, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	return retval;
}

static CORBA_boolean
impl_menu_get_sensitivity (PortableServer_Servant servant,
			   CORBA_char *path,
			   CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), FALSE);

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return FALSE;
	}

	return (CORBA_boolean) internal->sensitive;
}

/**
 * gnome_ui_handler_menu_get_sensitivity:
 */
gboolean
gnome_ui_handler_menu_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_sensitivity (uih, path);

	return menu_toplevel_get_sensitivity (uih, path);
}

static void
menu_toplevel_set_label_internal (GnomeUIHandler *uih, MenuItemInternal *internal, gchar *label_text)
{
	GtkWidget *parent_shell;
	GtkWidget *menu_widget;
	GtkWidget *child;
	char *parent_path;
	guint keyval;

	/*
	 * Modify the internal data.
	 */
	g_free (internal->item->label);
	internal->item->label = g_strdup (label_text);

	/*
	 * Now modify the widget.
	 */
	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);

	parent_path = path_get_parent (internal->item->path);
	parent_shell = menu_toplevel_get_shell (uih, parent_path);
	g_free (parent_path);

	/*
	 * Remove the old label widget.
	 */
	child = GTK_BIN (menu_widget)->child;

	if (child != NULL) {
		/*
		 * Grab the keyval for the label, if appropriate, so the
		 * corresponding accelerator can be removed.
		 */
		keyval = gtk_label_parse_uline (GTK_LABEL (child), internal->item->label);
		if (uih->top->accelgroup != NULL) {
			if (parent_shell != uih->top->menubar)
				gtk_widget_remove_accelerator (GTK_WIDGET (menu_widget), uih->top->accelgroup, keyval, 0);
			else
				gtk_widget_remove_accelerator (GTK_WIDGET (menu_widget), uih->top->accelgroup,
							       keyval, GDK_MOD1_MASK);
		}
			
		/*
		 * Now remove the old label widget.
		 */
		gtk_container_remove (GTK_CONTAINER (menu_widget), child);
	}
		
	/*
	 * Create the new label widget.
	 */
	menu_toplevel_create_label (uih, internal->item, parent_shell, menu_widget);
}

static void
menu_toplevel_set_label (GnomeUIHandler *uih, char *path, gchar *label_text)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_label_internal (uih, internal, label_text);
}

static void
menu_remote_set_label (GnomeUIHandler *uih, char *path, gchar *label_text)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_label (uih->top_level_uih,
					gnome_object_corba_objref (GNOME_OBJECT (uih)),
					path, label_text,
					&ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}
	CORBA_exception_free (&ev);
}

static void
impl_menu_set_label (PortableServer_Servant servant,
		     GNOME_UIHandler containee_uih,
		     CORBA_char *path,
		     CORBA_char *label,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	menu_toplevel_set_label_internal (uih, internal, label);
}

/**
 * gnome_ui_handler_menu_set_label:
 */
void
gnome_ui_handler_menu_set_label (GnomeUIHandler *uih, char *path,
				 gchar *label_text)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_label (uih, path, label_text);
		return;
	}

	menu_toplevel_set_label (uih, path, label_text);
}

static gchar *
menu_toplevel_get_label (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, NULL);

	if (internal->item->label == NULL)
		return NULL;

	return g_strdup (internal->item->label);
}

static gchar *
menu_remote_get_label (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;
	CORBA_char *label_text;
	char *retval;

	CORBA_exception_init (&ev);

	label_text = GNOME_UIHandler_menu_get_label (uih->top_level_uih, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	retval = g_strdup (label_text);

	CORBA_free (label_text);

	return retval;
}

static CORBA_char *
impl_menu_get_label (PortableServer_Servant servant,
		     CORBA_char *path,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), CORBA_string_dup (""));

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return CORBA_string_dup (CORBIFY_STRING (NULL));
	}

	return CORBA_string_dup (CORBIFY_STRING (internal->item->label));
}


/**
 * gnome_ui_handler_menu_get_label:
 */
gchar *
gnome_ui_handler_menu_get_label (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_label (uih, path);

	return menu_toplevel_get_label (uih, path);
}

static void
menu_toplevel_set_hint_internal (GnomeUIHandler *uih, MenuItemInternal *internal, char *hint)
{
	GtkWidget *menu_widget;

	/*
	 * Update the internal data.
	 */
	g_free (internal->item->hint);
	internal->item->hint = g_strdup (hint);

	/*
	 * Update the hint on the widget.
	 */
	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);
	g_return_if_fail (menu_widget != NULL);

	menu_toplevel_create_hint (uih, internal->item, menu_widget);
}

static void
menu_toplevel_set_hint (GnomeUIHandler *uih, char *path, char *hint)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_hint_internal (uih, internal, hint);
}

static void
menu_remote_set_hint (GnomeUIHandler *uih, char *path, char *hint)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_hint (uih->top_level_uih,
				       gnome_object_corba_objref (GNOME_OBJECT (uih)),
				       path, hint,
				       &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_set_hint (PortableServer_Servant servant,
		    GNOME_UIHandler containee_uih,
		    CORBA_char *path,
		    CORBA_char *hint,
		    CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);

	if (internal == NULL) {
		/* FIXME: Set exception */
		return;
	}

	menu_toplevel_set_hint_internal (uih, internal, hint);
}

/**
 * gnome_ui_handler_menu_set_hint:
 */
void
gnome_ui_handler_menu_set_hint (GnomeUIHandler *uih, char *path, char *hint)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_hint (uih, path, hint);
		return;
	}

	menu_toplevel_set_hint (uih, path, hint);
}

static gchar *
menu_toplevel_get_hint (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, NULL);

	return g_strdup (internal->item->hint);
}

static gchar *
menu_remote_get_hint (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;
	CORBA_char *hint;
	char *retval;

	CORBA_exception_init (&ev);

	hint = GNOME_UIHandler_menu_get_hint (uih->top_level_uih, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	retval = g_strdup (hint);

	CORBA_free (hint);

	return retval;
}

static CORBA_char *
impl_menu_get_hint (PortableServer_Servant servant,
		    CORBA_char *path,
		    CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), CORBA_string_dup (""));

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception */
		return CORBA_string_dup (CORBIFY_STRING (NULL));
	}

	return CORBA_string_dup (internal->item->hint);
}

/**
 * gnome_ui_handler_menu_get_hint:
 */
gchar *
gnome_ui_handler_menu_get_hint (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_hint (uih, path);

	return menu_toplevel_get_hint (uih, path);
}

static void
menu_toplevel_set_pixmap_internal (GnomeUIHandler *uih, MenuItemInternal *internal,
				   GnomeUIHandlerPixmapType type, gpointer data)
{
	GtkWidget *menu_widget;
	GtkWidget *pixmap_widget;

	/*
	 * Update the internal data for this menu item.
	 */
	pixmap_free_data (internal->item->pixmap_type, internal->item->pixmap_data);

	internal->item->pixmap_type = type;
	internal->item->pixmap_data = pixmap_copy_data (type, data);

	/*
	 * Update the widgets.
	 */
	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);

	/* Destroy the old pixmap */
	if (GTK_PIXMAP_MENU_ITEM (menu_widget)->pixmap != NULL)
		gtk_widget_destroy (GTK_PIXMAP_MENU_ITEM (menu_widget)->pixmap);

	/* Create and insert the new one. */
	pixmap_widget = GTK_WIDGET (uih_toplevel_create_pixmap (menu_widget, type, data));
	gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_widget),
					 GTK_WIDGET (pixmap_widget));
}

static void
menu_toplevel_set_pixmap (GnomeUIHandler *uih, char *path,
			  GnomeUIHandlerPixmapType type, gpointer data)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_pixmap_internal (uih, internal, type, data);
}

static void
menu_remote_set_pixmap (GnomeUIHandler *uih, char *path,
			GnomeUIHandlerPixmapType type, gpointer data)
{
	GNOME_UIHandler_iobuf *pixmap_buff;
	CORBA_Environment ev;

	pixmap_buff = pixmap_data_to_corba (type, data);
	
	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_pixmap (uih->top_level_uih,
					 gnome_object_corba_objref (GNOME_OBJECT (uih)),
					 path,
					 pixmap_type_to_corba (type),
					 pixmap_buff,
					 &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buff);
}

static void
impl_menu_set_pixmap (PortableServer_Servant servant,
		      GNOME_UIHandler containee_uih,
		      CORBA_char *path,
		      GNOME_UIHandler_PixmapType corba_pixmap_type,
		      GNOME_UIHandler_iobuf *corba_pixmap_data,
		      CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	menu_toplevel_set_pixmap_internal (uih, internal,
					   pixmap_corba_to_type (corba_pixmap_type),
					   pixmap_corba_to_data (corba_pixmap_type,
								 corba_pixmap_data));
}

/**
 * gnome_ui_handler_menu_set_pixmap:
 */
void
gnome_ui_handler_menu_set_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (data != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_pixmap (uih, path, type, data);
		return;
	}

	menu_toplevel_set_pixmap (uih, path, type, data);
}

static void
menu_toplevel_get_pixmap (GnomeUIHandler *uih, char *path,
			  GnomeUIHandlerPixmapType *type, gpointer *data)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	*data = pixmap_copy_data (internal->item->pixmap_type, internal->item->pixmap_data);
	*type = internal->item->pixmap_type;
}

static void
menu_remote_get_pixmap (GnomeUIHandler *uih, char *path,
			GnomeUIHandlerPixmapType *type, gpointer *data)
{
	/* FIXME: Implement me! */
}

static void
impl_menu_get_pixmap (PortableServer_Servant servant,
		      CORBA_char *path,
		      GNOME_UIHandler_PixmapType *corba_pixmap_type,
		      GNOME_UIHandler_iobuf **corba_pixmap_data,
		      CORBA_Environment *ev)
{
	/* FIXME: Implement me! */
}

/**
 * gnome_ui_handler_get_pixmap:
 */
void
gnome_ui_handler_menu_get_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (type != NULL);
	g_return_if_fail (data != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_get_pixmap (uih, path, type, data);
		return;
	}

	menu_toplevel_get_pixmap (uih, path, type, data);
}

static void
menu_toplevel_set_accel_internal (GnomeUIHandler *uih, MenuItemInternal *internal,
				  guint accelerator_key, GdkModifierType ac_mods)
{
	GtkWidget *menu_widget;

	internal->item->accelerator_key = accelerator_key;
	internal->item->ac_mods = ac_mods;

	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);
	menu_toplevel_install_global_accelerators (uih, internal->item, menu_widget);
}

static void
menu_toplevel_set_accel (GnomeUIHandler *uih, char *path,
			 guint accelerator_key, GdkModifierType ac_mods)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_accel_internal (uih, internal, accelerator_key, ac_mods);
}

static void
menu_remote_set_accel (GnomeUIHandler *uih, char *path,
		       guint accelerator_key, GdkModifierType ac_mods)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_accel (uih->top_level_uih,
					gnome_object_corba_objref (GNOME_OBJECT (uih)),
					path,
					(CORBA_long) accelerator_key,
					(CORBA_long) ac_mods,
					&ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_set_accel (PortableServer_Servant servant,
		     GNOME_UIHandler containee_uih,
		     CORBA_char *path,
		     CORBA_long accelerator_key,
		     CORBA_long ac_mods,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item_for_containee (uih, path, containee_uih);

	if (internal == NULL) {
		/* FIXME: Set exception */
		return;
	}

	menu_toplevel_set_accel_internal (uih, internal, (guint) accelerator_key, (GdkModifierType) ac_mods);
}

/**
 * gnome_ui_handler_menu_set_accel:
 */
void
gnome_ui_handler_menu_set_accel (GnomeUIHandler *uih, char *path,
				 guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_accel (uih, path, accelerator_key, ac_mods);
		return;
	}

	menu_toplevel_set_accel (uih, path, accelerator_key, ac_mods);
}

static void
menu_toplevel_get_accel (GnomeUIHandler *uih, char *path,
			 guint *accelerator_key, GdkModifierType *ac_mods)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	*accelerator_key = internal->item->accelerator_key;
	*ac_mods = internal->item->ac_mods;
}

static void
menu_remote_get_accel (GnomeUIHandler *uih, char *path,
		       guint *accelerator_key, GdkModifierType *ac_mods)
{
	CORBA_Environment ev;
	CORBA_long corba_accelerator_key;
	CORBA_long corba_ac_mods;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_get_accel (uih->top_level_uih, path, &corba_accelerator_key, &corba_ac_mods, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	*accelerator_key = (guint) corba_accelerator_key;
	*ac_mods = (GdkModifierType) corba_ac_mods;
}

static void
impl_menu_get_accel (PortableServer_Servant servant,
		     CORBA_char *path,
		     CORBA_long *accelerator_key,
		     CORBA_long *ac_mods,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	*accelerator_key = (CORBA_long) internal->item->accelerator_key;
	*ac_mods = (CORBA_long) internal->item->ac_mods;
}

/**
 * gnome_ui_handler_menu_get_accel:
 */
void
gnome_ui_handler_menu_get_accel (GnomeUIHandler *uih, char *path,
				 guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (accelerator_key != NULL);
	g_return_if_fail (ac_mods != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_get_accel (uih, path, accelerator_key, ac_mods);
		return;
	}

	menu_toplevel_get_accel (uih, path, accelerator_key, ac_mods);
}

static void
menu_local_set_callback (GnomeUIHandler *uih, char *path, GnomeUIHandlerCallbackFunc callback, gpointer callback_data)
{
	MenuItemLocalInternal *internal_cb;

	/*
	 * Get the local data for this item.
	 */
	internal_cb = menu_local_get_item (uih, path);

	/*
	 * Modify the existing callback data.
	 */
	internal_cb->callback = callback;
	internal_cb->callback_data = callback_data;
}


/**
 * gnome_ui_handler_menu_set_callback:
 */
void
gnome_ui_handler_menu_set_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc callback,
				    gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	menu_local_set_callback (uih, path, callback, callback_data);
}

static void
menu_local_get_callback (GnomeUIHandler *uih, char *path,
			 GnomeUIHandlerCallbackFunc *callback,
			 gpointer *callback_data)
{
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	*callback = internal_cb->callback;
	*callback_data = internal_cb->callback_data;
}

/**
 * gnome_ui_handler_menu_get_callback:
 */
void
gnome_ui_handler_menu_get_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc *callback,
				    gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (callback_data != NULL);

	menu_local_get_callback (uih, path, callback, callback_data);
}

static void
menu_toplevel_set_toggle_state_internal (GnomeUIHandler *uih, MenuItemInternal *internal, gboolean state)
{
	GtkWidget *menu_widget;

	/*
	 * Update the internal data.
	 */
	internal->active = state;

	/*
	 * Update the widget.
	 */
	if (! menu_toplevel_item_is_head (uih, internal))
		return;

	menu_widget = menu_toplevel_get_widget (uih, internal->item->path);
	g_return_if_fail (menu_widget != NULL);

	if (GTK_IS_CHECK_MENU_ITEM (menu_widget))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_widget), state);
}

static void
menu_toplevel_set_toggle_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_toggle_state_internal (uih, internal, state);
}

static void
menu_remote_set_toggle_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_set_toggle_state (uih->top_level_uih,
					       gnome_object_corba_objref (GNOME_OBJECT (uih)),
					       path, state,
					       &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_menu_set_toggle_state (PortableServer_Servant servant,
			    GNOME_UIHandler containee,
			    CORBA_char *path,
			    CORBA_boolean state,
			    CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item_for_containee (uih, path, containee);

	if (internal == NULL) {
		/* FIXME: Set exception */
		return;
	}

	menu_toplevel_set_toggle_state_internal (uih, internal, state);
}

/**
 * gnome_ui_handler_menu_set_toggle_state:
 */
void
gnome_ui_handler_menu_set_toggle_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_toggle_state (uih, path, state);
		return;
	}

	menu_toplevel_set_toggle_state (uih, path, state);
}

static gboolean
menu_toplevel_get_toggle_state (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, FALSE);

	return internal->active;
}

static gboolean
menu_remote_get_toggle_state (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;
	CORBA_boolean retval;

	CORBA_exception_init (&ev);

	retval = GNOME_UIHandler_menu_get_toggle_state (uih->top_level_uih, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	return (gboolean) retval;
}

static CORBA_boolean
impl_menu_get_toggle_state (PortableServer_Servant servant,
			    CORBA_char *path,
			    CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), FALSE);

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception */
		return FALSE;
	}
	
	return (CORBA_boolean) menu_toplevel_get_toggle_state (uih, path);
}

/**
 * gnome_ui_handler_menu_get_toggle_state:
 */
gboolean
gnome_ui_handler_menu_get_toggle_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_toggle_state (uih, path);

	return menu_toplevel_get_toggle_state (uih, path);
}

static void
menu_toplevel_set_radio_state_internal (GnomeUIHandler *uih, MenuItemInternal *internal, gboolean state)
{
	menu_toplevel_set_toggle_state_internal (uih, internal, state);
}

static void
menu_toplevel_set_radio_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 gnome_object_corba_objref (GNOME_OBJECT (uih)));
	g_return_if_fail (internal != NULL);
	
	menu_toplevel_set_radio_state_internal (uih, internal, state);
}

static void
menu_remote_set_radio_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	menu_remote_set_toggle_state (uih, path, state);
}

/**
 * gnome_ui_handler_menu_set_radio_state:
 */
void
gnome_ui_handler_menu_set_radio_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_radio_state (uih, path, state);
		return;
	}

	menu_toplevel_set_radio_state (uih, path, state);
}

static gboolean
menu_toplevel_get_radio_state (GnomeUIHandler *uih, char *path)
{
	return menu_toplevel_get_toggle_state (uih, path);
}

static gboolean
menu_remote_get_radio_state (GnomeUIHandler *uih, char *path)
{
	return menu_remote_get_toggle_state (uih, path);
}

/**
 * gnome_ui_handler_menu_get_radio_state:
 */
gboolean
gnome_ui_handler_menu_get_radio_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_radio_state (uih, path);

	return menu_toplevel_get_radio_state (uih, path);
}

/*
 *
 * Toolbars.
 *
 * The name of the toolbar is the first element in the toolbar path.
 * For example:
 *		"Common/Save"
 *		"Graphics Tools/Erase"
 * 
 * Where both "Common" and "Graphics Tools" are the names of 
 * toolbars.
 */

/*
 * This structure is used to maintain an internal representation of a
 * toolbar item.
 */

struct _ToolbarItemInternal {
	/*
	 * The GnomeUIHandler.
	 */
	GnomeUIHandler *uih;

	/*
	 * A copy of the GnomeUIHandlerToolbarItem for this toolbar item.
	 */
	GnomeUIHandlerToolbarItem *item;

	/*
	 * The UIHandler CORBA interface for the containee which owns
	 * this particular menu item.
	 */
	GNOME_UIHandler uih_corba;

	/*
	 * If this item is a radio group, then this list points to the
	 * ToolbarItemInternal structures for the members of the group.
	 */
	GSList *radio_items;

	gboolean sensitive;
	gboolean active;
};

/*
 * The internal data for a toolbar.
 */
struct _ToolbarToolbarInternal {

	/*
	 * This toolbar's name. e.g. "Common"
	 * The path for a toolbar item will be something like "/Common/Help"
	 */
	char			 *name;

	/*
	 * A list of paths for the items in this toolbar.
	 */
	GList			 *children;

	/*
	 * The owner for this toolbar.
	 */
	GNOME_UIHandler		 uih_corba;

	GtkOrientation		 orientation;
	GtkToolbarStyle		 style;
	GtkToolbarSpaceStyle	 space_style;
	int			 space_size;
	GtkReliefStyle		 relief;
};

struct _ToolbarItemLocalInternal {
	gpointer	 callback;
	gpointer	 callback_data;
};

struct _ToolbarToolbarLocalInternal {
	GList		*children;
};

static GnomeUIHandlerToolbarItem *
toolbar_make_item (char *path, GnomeUIHandlerToolbarItemType type,
		   char *label, char *hint, int pos,
		   GnomeUIHandlerPixmapType pixmap_type,
		   gpointer pixmap_data, guint accelerator_key,
		   GdkModifierType ac_mods,
		   GnomeUIHandlerCallbackFunc callback,
		   gpointer callback_data)
{
	GnomeUIHandlerToolbarItem *item;

	item = g_new0 (GnomeUIHandlerToolbarItem, 1);

	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->pos = pos;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods = ac_mods;
	item->callback = callback;
	item->callback_data = callback_data;

	return item;
}

static GnomeUIHandlerToolbarItem *
toolbar_copy_item (GnomeUIHandlerToolbarItem *item)
{
	GnomeUIHandlerToolbarItem *copy;

	copy = g_new0 (GnomeUIHandlerToolbarItem, 1);

	copy->path = COPY_STRING (item->path);
	copy->type = item->type;
	copy->hint = COPY_STRING (item->hint);
	copy->pos = item->pos;
	copy->label = COPY_STRING (item->label);
	copy->children = NULL;

	copy->pixmap_data = pixmap_copy_data (item->pixmap_type, item->pixmap_data);
	copy->pixmap_type = item->pixmap_type;

	copy->accelerator_key = item->accelerator_key;
	copy->ac_mods = item->ac_mods;

	copy->callback = item->callback;
	copy->callback_data = item->callback_data;

	return copy;
}

static void
toolbar_free_data (GnomeUIHandlerToolbarItem *item)
{
	g_free (item->path);
	item->path = NULL;

	g_free (item->label);
	item->label = NULL;

	g_free (item->hint);
	item->hint = NULL;

	pixmap_free_data (item->pixmap_type, item->pixmap_data);
}

static void
toolbar_free (GnomeUIHandlerToolbarItem *item)
{
	toolbar_free_data (item);
	g_free (item);
}

static GNOME_UIHandler_ToolbarType
toolbar_type_to_corba (GnomeUIHandlerToolbarItemType type)
{
	switch (type) {
	case GNOME_UI_HANDLER_TOOLBAR_END:
		g_warning ("Passing ToolbarTypeEnd through CORBA!\n");
		return GNOME_UIHandler_ToolbarTypeEnd;

	case GNOME_UI_HANDLER_TOOLBAR_ITEM:
		return GNOME_UIHandler_ToolbarTypeItem;

	case GNOME_UI_HANDLER_TOOLBAR_RADIOITEM:
		return GNOME_UIHandler_ToolbarTypeRadioItem;

	case GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP:
		return GNOME_UIHandler_ToolbarTypeRadioGroup;

	case GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM:
		return GNOME_UIHandler_ToolbarTypeToggleItem;

	case GNOME_UI_HANDLER_TOOLBAR_SEPARATOR:
		return GNOME_UIHandler_ToolbarTypeSeparator;

	default:
		g_warning ("toolbar_type_to_corba: Unknown toolbar type [%d]!\n", (gint) type);
	}

	return GNOME_UI_HANDLER_TOOLBAR_ITEM;
}

static GnomeUIHandlerToolbarItemType
toolbar_corba_to_type (GNOME_UIHandler_ToolbarType type)
{
	switch (type) {
	case GNOME_UIHandler_ToolbarTypeEnd:
		g_warning ("Passing ToolbarTypeEnd through CORBA!\n");
		return GNOME_UI_HANDLER_TOOLBAR_END;

	case GNOME_UIHandler_ToolbarTypeItem:
		return GNOME_UI_HANDLER_TOOLBAR_ITEM;

	case GNOME_UIHandler_ToolbarTypeRadioItem:
		return GNOME_UI_HANDLER_TOOLBAR_RADIOITEM;

	case GNOME_UIHandler_ToolbarTypeRadioGroup:
		return GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP;

	case GNOME_UIHandler_ToolbarTypeSeparator:
		return GNOME_UI_HANDLER_TOOLBAR_SEPARATOR;

	case GNOME_UIHandler_ToolbarTypeToggleItem:
		return GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM;

	default:
		g_warning ("toolbar_corba_to_type: Unknown toolbar type [%d]!\n", (gint) type);
			
	}

	return GNOME_UI_HANDLER_TOOLBAR_ITEM;
}


static ToolbarItemLocalInternal *
toolbar_local_get_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_toolbar_callback, path);

	if (l == NULL)
		return NULL;

	return (ToolbarItemLocalInternal *) l->data;
}

static ToolbarToolbarLocalInternal *
toolbar_local_get_toolbar (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_toolbar_toolbar, path);

	if (l == NULL)
		return NULL;

	return (ToolbarToolbarLocalInternal *) l->data;
}

static ToolbarToolbarInternal *
toolbar_toplevel_get_toolbar (GnomeUIHandler *uih, char *name)
{
	GList *l;
	
	l = g_hash_table_lookup (uih->top->name_to_toolbar, name);

	if (l == NULL)
		return NULL;

	return (ToolbarToolbarInternal *) l->data;
}

static ToolbarItemInternal *
toolbar_toplevel_get_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, path);

	if (l == NULL)
		return NULL;

	return (ToolbarItemInternal *) l->data;
}

static ToolbarItemInternal *
toolbar_toplevel_get_item_for_containee (GnomeUIHandler *uih, char *path,
					 GNOME_UIHandler uih_corba)
{
	GList *l, *curr;

	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, path);

	for (curr = l; curr != NULL; curr = curr->next) {
		ToolbarItemInternal *internal;
		CORBA_Environment ev;

		internal = (ToolbarItemInternal *) curr->data;

		CORBA_exception_init (&ev);

		if (CORBA_Object_is_equivalent (uih_corba, internal->uih_corba, &ev)) {
			CORBA_exception_free (&ev);

			return internal;
		}

		CORBA_exception_free (&ev);
	}

	return NULL;
}

static gboolean
toolbar_toplevel_item_is_head (GnomeUIHandler *uih, ToolbarItemInternal *internal)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, internal->item->path);

	if (l->data == internal)
		return TRUE;

	return FALSE;
}

static char *
toolbar_get_toolbar_name (char *path)
{
	char *parent_name;
	char **toks;
	int i;

	toks = path_tokenize (path);

	parent_name = g_strdup (toks [1]);

	for (i = 0; toks [i] != NULL; i ++) {
		g_free (toks [i]);
	}
	g_free (toks);

	return parent_name;
}

static void
toolbar_toplevel_toolbar_create_widget (GnomeUIHandler *uih, ToolbarToolbarInternal *internal)
{
	GtkWidget *toolbar_widget;

	/*
	 * Create the toolbar widget.
	 */
	toolbar_widget = gtk_toolbar_new (internal->orientation, internal->style);

	/*
	 * Store it in the widget hash table.
	 */
	g_hash_table_insert (uih->top->name_to_toolbar_widget, g_strdup (internal->name), toolbar_widget);

	/*
	 * Stuff it into the application.
	 */
	gnome_app_set_toolbar (uih->top->app, GTK_TOOLBAR (toolbar_widget));

	gtk_widget_show (toolbar_widget);
}

static void
toolbar_toplevel_item_remove_widgets (GnomeUIHandler *uih, char *path)
{
	GtkWidget *toolbar_item_widget;
	gboolean found;
	char *orig_key;

	/*
	 * Get the toolbar item widget and remove its entry from the
	 * hash table.
	 */
	found = g_hash_table_lookup_extended (uih->top->path_to_toolbar_item_widget, path,
					      (gpointer *) &orig_key, (gpointer *) &toolbar_item_widget);
	g_hash_table_remove (uih->top->path_to_toolbar_item_widget, path);
	g_free (orig_key);

	/*
	 * Destroy the widget.
	 */
	if (found && toolbar_item_widget != NULL)
		gtk_widget_destroy (toolbar_item_widget);
}

static void
toolbar_local_toolbar_create (GnomeUIHandler *uih, char *name)
{
	ToolbarToolbarLocalInternal *internal;
	GList *l;

	internal = g_new0 (ToolbarToolbarLocalInternal, 1);

	l = g_hash_table_lookup (uih->path_to_toolbar_toolbar, name);

	if (l == NULL) {
		l = g_list_prepend (l, internal);
		g_hash_table_insert (uih->path_to_toolbar_toolbar, g_strdup (name), l);
	} else {
		l = g_list_prepend (l, internal);
		g_hash_table_insert (uih->path_to_toolbar_toolbar, name, l);
	}
}

static void
toolbar_toplevel_toolbar_remove_widgets_recursive (GnomeUIHandler *uih, char *name)
{
	ToolbarToolbarInternal *internal;
	GtkWidget *toolbar_widget;
	char *orig_key;
	GList *curr;

	internal = toolbar_toplevel_get_toolbar (uih, name);
	g_return_if_fail (internal != NULL);

	/*
	 * First destroy each of the toolbar's children.
	 */
	for (curr = internal->children; curr != NULL; curr = curr->next)
		toolbar_toplevel_item_remove_widgets (uih, (char *) curr->data);

	/*
	 * Get the toolbar widget and remove its entry from the hash table.
	 */
	g_hash_table_lookup_extended (uih->top->name_to_toolbar_widget, name,
				      (gpointer *) &orig_key, (gpointer *) &toolbar_widget);
	g_hash_table_remove (uih->top->name_to_toolbar_widget, name);
	g_free (orig_key);

	/*
	 * Now destroy the toolbar widget.
	 */
	gtk_widget_destroy (toolbar_widget);
}

static void
toolbar_toplevel_toolbar_override_notify_recursive (GnomeUIHandler *uih, char *name)
{
	ToolbarToolbarInternal *internal;
	CORBA_Environment ev;
	char *toolbar_path;
	GList *curr;

	internal = toolbar_toplevel_get_toolbar (uih, name);

	toolbar_path = g_strconcat ("/", name, NULL);

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_overridden (internal->uih_corba, toolbar_path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);

	g_free (toolbar_path);

	for (curr = internal->children; curr != NULL; curr = curr->next)
		toolbar_toplevel_item_override_notify (uih, (char *) curr->data);
}

static void
toolbar_toplevel_toolbar_check_override (GnomeUIHandler *uih, char *name)
{
	ToolbarToolbarInternal *internal;

	/*
	 * Check for an existing toolbar by this name.
	 */
        internal = toolbar_toplevel_get_toolbar (uih, name);

	if (internal == NULL)
		return;

	/*
	 * There is a toolbar by this name, and so it must
	 * be overridden.
	 *
	 * We remove its widgets, its children's widgets, and notify
	 * the owner of each overridden item that it has been
	 * overridden.
	 */
	toolbar_toplevel_toolbar_remove_widgets_recursive (uih, name);

	/*
	 * Notification.
	 */
	toolbar_toplevel_toolbar_override_notify_recursive (uih, name);
}

static ToolbarToolbarInternal *
toolbar_toplevel_toolbar_store_data (GnomeUIHandler *uih, char *name, GNOME_UIHandler uih_corba)
{
	ToolbarToolbarInternal *internal;
	CORBA_Environment ev;
	GList *l;

	internal = g_new0 (ToolbarToolbarInternal, 1);
	internal->name = g_strdup (name);

	internal->orientation = GTK_ORIENTATION_HORIZONTAL;
	internal->style = GTK_TOOLBAR_BOTH;

	if (gnome_preferences_get_toolbar_lines ()) {
		internal->space_style = GTK_TOOLBAR_SPACE_LINE;
		internal->space_size = GNOME_PAD * 2;
	} else {
		internal->space_style = GTK_TOOLBAR_SPACE_EMPTY;
		internal->space_size = GNOME_PAD;
	}

	if (gnome_preferences_get_toolbar_relief_btn ())
		internal->relief = GTK_RELIEF_NORMAL;
	else
		internal->relief = GTK_RELIEF_NONE;

	if (gnome_preferences_get_toolbar_labels ())
		internal->style = GTK_TOOLBAR_BOTH;
	else
		internal->style = GTK_TOOLBAR_ICONS;

	CORBA_exception_init (&ev);
	internal->uih_corba = CORBA_Object_duplicate (uih_corba, &ev);
	CORBA_exception_free (&ev);

	/*
	 * Put this new toolbar at the front of the list of toolbars.
	 */
	l = g_hash_table_lookup (uih->top->name_to_toolbar, name);

	if (l != NULL) {
		l = g_list_prepend (l, internal);
		g_hash_table_insert (uih->top->name_to_toolbar, name, l);
	} else {
		l = g_list_prepend (NULL, internal);
		g_hash_table_insert (uih->top->name_to_toolbar, g_strdup (name), l);
	}

	return internal;
}

static void
toolbar_toplevel_toolbar_create (GnomeUIHandler *uih, char *name, GNOME_UIHandler uih_corba)
{
	ToolbarToolbarInternal *internal;

	/*
	 * If there is already a toolbar by this name, notify its
	 * owner that it is being overridden.
	 */
	toolbar_toplevel_toolbar_check_override (uih, name);

	/*
	 * Store the internal data for this toolbar.
	 */
	internal = toolbar_toplevel_toolbar_store_data (uih, name, uih_corba);

	/*
	 * Create the toolbar widget.
	 */
	toolbar_toplevel_toolbar_create_widget (uih, internal);
}

static void
toolbar_remote_toolbar_create (GnomeUIHandler *uih, char *name)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_create (uih->top_level_uih,
					gnome_object_corba_objref (GNOME_OBJECT (uih)), name,
					&ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_create (PortableServer_Servant servant,
		     GNOME_UIHandler containee,
		     CORBA_char *name,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));

	toolbar_toplevel_toolbar_create (uih, name, containee);
}

/**
 * gnome_ui_handler_toolbar_create:
 */
void
gnome_ui_handler_create_toolbar (GnomeUIHandler *uih, char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);

	toolbar_local_toolbar_create (uih, name);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		toolbar_remote_toolbar_create (uih, name);
		return;
	}

	toolbar_toplevel_toolbar_create (uih, name, gnome_object_corba_objref (GNOME_OBJECT (uih)));
}

static void
toolbar_toplevel_toolbar_remove (GnomeUIHandler *uih, char *name)
{
}

static void
toolbar_remote_toolbar_remove (GnomeUIHandler *uih, char *name)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_remove (uih->top_level_uih,
					gnome_object_corba_objref (GNOME_OBJECT (uih)),
					name,
					&ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_remove (PortableServer_Servant servant,
		     GNOME_UIHandler containee,
		     CORBA_char *name,
		     CORBA_Environment *ev)

{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
}

/**
 * gnome_ui_handler_remove_toolbar:
 */
void
gnome_ui_handler_remove_toolbar (GnomeUIHandler *uih, char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_remote_toolbar_remove (uih, name);
	else
		toolbar_toplevel_toolbar_remove (uih, name);
}

void
gnome_ui_handler_set_toolbar (GnomeUIHandler *uih, char *name, GtkWidget *toolbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
}

/**
 * gnome_ui_handler_get_toolbar_list:
 */
GList *
gnome_ui_handler_get_toolbar_list (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return NULL;
}

static void
toolbar_local_do_path (char *parent_path, GnomeUIHandlerToolbarItem *item)
{
	uih_local_do_path (parent_path, item->label, & item->path);
}

static void
toolbar_local_remove_parent_entry (GnomeUIHandler *uih, char *path, gboolean warn)
{
	ToolbarToolbarLocalInternal *parent;
	char *parent_path;
	GList *curr;

	parent_path = toolbar_get_toolbar_name (path);
	parent = toolbar_local_get_toolbar (uih, parent_path);
	g_free (parent_path);

	if (parent == NULL)
		return;

	for (curr = parent->children; curr != NULL; curr = curr->next) {
		if (! strcmp (path, (char *) curr->data)) {
			parent->children = g_list_remove_link (parent->children, curr);
			g_free (curr->data);
			g_list_free_1 (curr);
			return;
		}
	}

	if (warn)
		g_warning ("toolbar_local_remove_parent_entry: No entry in toolbar for item path [%s]!\n", path);
}

static void
toolbar_local_add_parent_entry (GnomeUIHandler *uih, char *path)
{
	ToolbarToolbarLocalInternal *parent_internal_cb;
	char *parent_name;

	toolbar_local_remove_parent_entry (uih, path, FALSE);

	parent_name = toolbar_get_toolbar_name (path);
	parent_internal_cb = toolbar_local_get_toolbar (uih, parent_name);
	g_free (parent_name);

	if (parent_internal_cb == NULL) {
		/*
		 * If we don't have an entry for the parent,
		 * it doesn't matter.
		 */
		return;
	}

	parent_internal_cb->children = g_list_prepend (parent_internal_cb->children, g_strdup (path));
}

static void
toolbar_local_create_item (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerToolbarItem *item)
{
	ToolbarItemLocalInternal *internal_cb;
	GList *l, *new_list;

	/*
	 * Setup this item's path.
	 */
	toolbar_local_do_path (parent_path, item);

	/*
	 * Store it.
	 */
	internal_cb = g_new0 (ToolbarItemLocalInternal, 1);
	internal_cb->callback = item->callback;
	internal_cb->callback_data = item->callback_data;

	l = g_hash_table_lookup (uih->path_to_toolbar_callback, item->path);
	new_list = g_list_prepend (l, internal_cb);

	if (l == NULL)
		g_hash_table_insert (uih->path_to_toolbar_callback, g_strdup (item->path), new_list);
	else
		g_hash_table_insert (uih->path_to_toolbar_callback, item->path, new_list);

	/*
	 * Update the toolbar's child list.
	 */
	toolbar_local_add_parent_entry (uih, item->path);
}

static void
toolbar_toplevel_item_override_notify (GnomeUIHandler *uih, char *path)
{
	ToolbarItemInternal *internal;
	CORBA_Environment ev;

	internal = toolbar_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_overridden (internal->uih_corba, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_overridden (PortableServer_Servant servant,
			 CORBA_char *path,
			 CORBA_Environment *ev)
{
}

static void
toolbar_toplevel_item_check_override (GnomeUIHandler *uih, char *path)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);

	if (internal == NULL)
		return;

	toolbar_toplevel_item_remove_widgets (uih, path);

	toolbar_toplevel_item_override_notify (uih, path);
}

static gint
toolbar_toplevel_item_activated (GtkWidget *widget, gpointer user_data)
{
	/* FIXME: implement me */
	return FALSE;
}

static GtkToolbarChildType
toolbar_type_to_gtk_type (GnomeUIHandlerToolbarItemType type)
{
	switch (type) {
	case GNOME_UI_HANDLER_TOOLBAR_ITEM:
		return GTK_TOOLBAR_CHILD_BUTTON;

	case GNOME_UI_HANDLER_TOOLBAR_RADIOITEM:
		return GTK_TOOLBAR_CHILD_RADIOBUTTON;

	case GNOME_UI_HANDLER_TOOLBAR_SEPARATOR:
		return GTK_TOOLBAR_CHILD_SPACE;

	case GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM:
		return GTK_TOOLBAR_CHILD_TOGGLEBUTTON;

	case GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP:
		g_warning ("toolbar_type_to_gtk_type: Trying to convert UIHandler radiogroup "
			   "to Gtk toolbar type!\n");
		return GTK_TOOLBAR_CHILD_SPACE;
		
	default:
		g_warning ("toolbar_type_to_gtk_type: Unkonwn UIHandler toolbar "
			   "item type [%d]!\n", type);
	}

	return GTK_TOOLBAR_CHILD_BUTTON;
}

static void
toolbar_toplevel_item_create_widgets (GnomeUIHandler *uih,
				      ToolbarItemInternal *internal)
{
	GtkWidget *toolbar_item;
	GtkWidget *toolbar;
	GtkWidget *pixmap;
	char *parent_name;

	g_return_if_fail (internal != NULL);
	g_return_if_fail (internal->item != NULL);

	parent_name = toolbar_get_toolbar_name (internal->item->path);
	toolbar = g_hash_table_lookup (uih->top->name_to_toolbar_widget, parent_name);
	g_free (parent_name);

	g_return_if_fail (toolbar != NULL);

	toolbar_item = NULL;
	switch (internal->item->type) {

	case GNOME_UI_HANDLER_TOOLBAR_SEPARATOR:
		gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), internal->item->pos);
		break;

	case GNOME_UI_HANDLER_TOOLBAR_ITEM:
		pixmap = NULL;
		if (internal->item->pixmap_data != NULL && internal->item->pixmap_type != GNOME_UI_HANDLER_PIXMAP_NONE)
			pixmap = uih_toplevel_create_pixmap (toolbar, internal->item->pixmap_type,
							     internal->item->pixmap_data);


		if (internal->item->pos > 0)
			toolbar_item = gtk_toolbar_insert_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, pixmap,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal, internal->item->pos);
		else
			toolbar_item = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, pixmap,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal);

		break;

	case GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM:

		if (internal->item->pos > 0)
			toolbar_item = gtk_toolbar_insert_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, NULL,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal, internal->item->pos);
		else
			toolbar_item = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, NULL,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal);

		break;

	case GNOME_UI_HANDLER_TOOLBAR_RADIOITEM:
		g_warning ("Toolbar radio items/groups not implemented\n");
		return;
		
	default:

		g_warning ("toolbar_toplevel_item_create_widgets: Unkonwn toolbar item type [%d]!\n",
			   (gint) internal->item->type);
		return;
	}

	/* FIXME: Connect to signals and gtk_widget_add_accelerator */

	g_hash_table_insert (uih->top->path_to_toolbar_item_widget, g_strdup (internal->item->path), toolbar_item);
}

static void
toolbar_toplevel_item_remove_parent_entry (GnomeUIHandler *uih, char *path, gboolean warn)
{
	ToolbarToolbarInternal *parent;
	char *parent_name;
	GList *curr;

	parent_name = toolbar_get_toolbar_name (path);
	parent = toolbar_toplevel_get_toolbar (uih, parent_name);

	if (parent == NULL) {
		g_warning ("toolbar_toplevel_remove_parent_entry: Cannot find toolbar [%s] for path [%s]!\n",
			   path, parent_name);
		g_free (parent_name);
		return;
	}
	g_free (parent_name);

	for (curr = parent->children; curr != NULL; curr = curr->next) {
		if (! strcmp (path, (char *) curr->data)) {
			parent->children = g_list_remove_link (parent->children, curr);
			g_free (curr->data);
			g_list_free_1 (curr);
			return;
		}
	}

	if (warn)
		g_warning ("toolbar_toplevel_remove_parent_entry: No entry in parent for child path [%s]!\n", path);
}

static void
toolbar_toplevel_item_add_parent_entry (GnomeUIHandler *uih, char *path)
{
	ToolbarToolbarInternal *internal;
	char *parent_name;

	toolbar_toplevel_item_remove_parent_entry (uih, path, FALSE);
	
	parent_name = toolbar_get_toolbar_name (path);
	internal = toolbar_toplevel_get_toolbar (uih, parent_name);
	g_free (parent_name);

	if (!internal)
		return;

	internal->children = g_list_prepend (internal->children, g_strdup (path));
}

static ToolbarItemInternal *
toolbar_toplevel_item_store_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba,
				  GnomeUIHandlerToolbarItem *item)
{
	ToolbarItemInternal *internal;
	CORBA_Environment ev;
	GList *l;

	/*
	 * Create the internal representation of the toolbar item.
	 */
	internal = g_new0 (ToolbarItemInternal, 1);
	internal->item = toolbar_copy_item (item);
	internal->uih = uih;
	internal->sensitive = TRUE;
	internal->active = FALSE;

	CORBA_exception_init (&ev);
	internal->uih_corba = CORBA_Object_duplicate (uih_corba, &ev);
	CORBA_exception_free (&ev);

	/*
	 * Store it.
	 */
	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, internal->item->path);

	if (l != NULL) {
		l = g_list_prepend (l, internal);
		g_hash_table_insert (uih->top->path_to_toolbar_item, internal->item->path, l);
	} else {
		l = g_list_prepend (NULL, internal);
		g_hash_table_insert (uih->top->path_to_toolbar_item, g_strdup (internal->item->path), l);
	}

	toolbar_toplevel_item_add_parent_entry (uih, item->path);

	return internal;
}

static void
toolbar_toplevel_create_item (GnomeUIHandler *uih, char *parent_path,
			      GnomeUIHandlerToolbarItem *item, GNOME_UIHandler uih_corba)
{
	ToolbarItemInternal *internal;

	/*
	 * Check to see if this item is overriding an existing toolbar
	 * item.
	 */
	toolbar_toplevel_item_check_override (uih, item->path);

	/*
	 * Store an internal representation of the item.
	 */
	internal = toolbar_toplevel_item_store_data (uih, uih_corba, item);

	/*
	 * Create the toolbar item widgets.
	 */
	toolbar_toplevel_item_create_widgets (uih, internal);
}

static void
toolbar_remote_create_item (GnomeUIHandler *uih, char *parent_path,
			    GnomeUIHandlerToolbarItem *item)
{
	GNOME_UIHandler_iobuf *pixmap_buf;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	pixmap_buf = pixmap_data_to_corba (item->pixmap_type, item->pixmap_data);

	GNOME_UIHandler_toolbar_create_item (uih->top_level_uih,
					     gnome_object_corba_objref (GNOME_OBJECT (uih)),
					     item->path,
					     toolbar_type_to_corba (item->type),
					     CORBIFY_STRING (item->label),
					     CORBIFY_STRING (item->hint),
					     item->pos,
					     pixmap_type_to_corba (item->pixmap_type),
					     pixmap_buf,
					     (CORBA_unsigned_long) item->accelerator_key,
					     (CORBA_long) item->ac_mods,
					     &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buf);
}

static void
impl_toolbar_create_item (PortableServer_Servant servant,
			  GNOME_UIHandler containee_uih,
			  CORBA_char *path,
			  GNOME_UIHandler_ToolbarType toolbar_type,
			  CORBA_char *label,
			  CORBA_char *hint,
			  CORBA_long pos,
			  GNOME_UIHandler_PixmapType pixmap_type,
			  GNOME_UIHandler_iobuf *pixmap_data,
			  CORBA_unsigned_long accelerator_key,
			  CORBA_long modifier,
			  CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	GnomeUIHandlerToolbarItem *item;
	char *parent_path;


	item = toolbar_make_item (path, toolbar_corba_to_type (toolbar_type),
				  UNCORBIFY_STRING (label),
				  UNCORBIFY_STRING (hint),
				  pos,
				  pixmap_corba_to_type (pixmap_type),
				  pixmap_corba_to_data (pixmap_type, pixmap_data),
				  (guint) accelerator_key, (GdkModifierType) modifier,
				  NULL, NULL);

	parent_path = path_get_parent (item->path);
	g_return_if_fail (parent_path != NULL);

	toolbar_toplevel_create_item (uih, parent_path, item, containee_uih);

	pixmap_free_data (item->pixmap_type, item->pixmap_data);

	g_free (item);
	g_free (parent_path);
}

/**
 * gnome_ui_handler_toolbar_add_one:
 */
void
gnome_ui_handler_toolbar_add_one (GnomeUIHandler *uih, char *parent_path,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Store the toolbar item's local callback data.
	 */
	toolbar_local_create_item (uih, parent_path, item);

	/*
	 * Create the item.
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_remote_create_item  (uih, parent_path, item);
	else
		toolbar_toplevel_create_item  (uih, parent_path, item,
					       gnome_object_corba_objref (GNOME_OBJECT (uih)));
}

void
gnome_ui_handler_toolbar_add_list (GnomeUIHandler *uih, char *parent_path,
				   GnomeUIHandlerToolbarItem *item)
{
	GnomeUIHandlerToolbarItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; curr->type != GNOME_UI_HANDLER_TOOLBAR_END; curr ++)
		gnome_ui_handler_toolbar_add_tree (uih, parent_path, curr);
}

void
gnome_ui_handler_toolbar_add_tree (GnomeUIHandler *uih, char *parent_path,
				   GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Add this toolbar item.
	 */
	gnome_ui_handler_toolbar_add_one (uih, parent_path, item);

	/*
	 * Recursive add its children.
	 */
	if (item->children != NULL)
		gnome_ui_handler_toolbar_add_list (uih, item->path, item->children);
}

void
gnome_ui_handler_toolbar_new (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItemType type,
			      char *label, char *hint,
			      int pos, GnomeUIHandlerPixmapType pixmap_type,
			      gpointer pixmap_data, guint accelerator_key,
			      GdkModifierType ac_mods,
			      GnomeUIHandlerCallbackFunc callback,
			      gpointer callback_data)
{
	GnomeUIHandlerToolbarItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	item = toolbar_make_item (path, type, label, hint, pos, pixmap_type, pixmap_data,
				  accelerator_key, ac_mods, callback, callback_data);

	parent_path = path_get_parent (path);
	g_return_if_fail (parent_path != NULL);

	gnome_ui_handler_toolbar_add_one (uih, parent_path, item);

	g_free (item);
	g_free (parent_path);
}

/**
 * gnome_ui_handler_toolbar_new_item
 */
void
gnome_ui_handler_toolbar_new_item (GnomeUIHandler *uih, char *path,
				   char *label, char *hint, int pos,
				   GnomeUIHandlerPixmapType pixmap_type,
				   gpointer pixmap_data,
				   guint accelerator_key, GdkModifierType ac_mods,
				   GnomeUIHandlerCallbackFunc callback,
				   gpointer callback_data)
{
	gnome_ui_handler_toolbar_new (uih, path,
				      GNOME_UI_HANDLER_TOOLBAR_ITEM,
				      label, hint, pos, pixmap_type,
				      pixmap_data, accelerator_key,
				      ac_mods, callback, callback_data);
}

/**
 * gnome_ui_handler_toolbar_new_separator:
 */
void
gnome_ui_handler_toolbar_new_separator (GnomeUIHandler *uih, char *path, int pos)
{
	gnome_ui_handler_toolbar_new (uih, path,
				      GNOME_UI_HANDLER_TOOLBAR_SEPARATOR,
				      NULL, NULL, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * gnome_ui_handler_toolbar_new_radiogroup:
 */
void
gnome_ui_handler_toolbar_new_radiogroup (GnomeUIHandler *uih, char *path)
{
	gnome_ui_handler_toolbar_new (uih, path, GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP,
				      NULL, NULL, -1, GNOME_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * gnome_ui_handler_toolbar_new_radioitem:
 */
void
gnome_ui_handler_toolbar_new_radioitem (GnomeUIHandler *uih, char *path,
					char *label, char *hint, int pos,
					guint accelerator_key, GdkModifierType ac_mods,
					GnomeUIHandlerCallbackFunc callback,
					gpointer callback_data)
{
	gnome_ui_handler_toolbar_new (uih, path, GNOME_UI_HANDLER_TOOLBAR_RADIOITEM,
				      label, hint, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				      NULL, accelerator_key, ac_mods, callback, callback_data);
}

/**
 * gnome_ui_handler_toolbar_new_toggleitem:
 */
void
gnome_ui_handler_toolbar_new_toggleitem (GnomeUIHandler *uih, char *path,
					 char *label, char *hint, int pos,
					 guint accelerator_key, GdkModifierType ac_mods,
					 GnomeUIHandlerCallbackFunc callback,
					 gpointer callback_data)
{
	gnome_ui_handler_toolbar_new (uih, path, GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM,
				      label, hint, pos, GNOME_UI_HANDLER_PIXMAP_NONE,
				      NULL, accelerator_key, ac_mods, callback,
				      callback_data);
}

static void
toolbar_local_remove_item (GnomeUIHandler *uih, char *path)
{
	GList *l, *new_list;

	/*
	 * Remove the local data at the front of the list for this
	 * guy.
	 */
	l = g_hash_table_lookup (uih->path_to_toolbar_callback, path);

	if (l == NULL)
		return;

	/*
	 * Remove the list link.
	 */
	new_list = g_list_remove_link (l, l);
	g_list_free_1 (l);

	/*
	 * If there are no items left in the list, remove the hash
	 * table entry.
	 */
	if (new_list == NULL) {
		char *orig_key;

		g_hash_table_lookup_extended (uih->path_to_toolbar_callback, path, (gpointer *) &orig_key, NULL);
		g_hash_table_remove (uih->path_to_toolbar_callback, path);
		g_free (orig_key);

		toolbar_local_remove_parent_entry (uih, path, TRUE);
	} else
		g_hash_table_insert (uih->path_to_toolbar_callback, path, new_list);
	
}

static void
toolbar_toplevel_item_reinstate_notify (GnomeUIHandler *uih, char *path)
{
	ToolbarItemInternal *internal;
	CORBA_Environment ev;

	internal = toolbar_toplevel_get_item (uih, path);

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_reinstated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			GNOME_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_reinstated (PortableServer_Servant servant,
			 CORBA_char *path,
			 CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	ToolbarItemLocalInternal *internal_cb;

	internal_cb = toolbar_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received reinstatement notification for toolbar item I don't own [%s]!\n", path);
		return;
	}

	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [TOOLBAR_ITEM_REINSTATED],
			 path, internal_cb->callback_data);

}

static void
toolbar_toplevel_item_remove_notify (GnomeUIHandler *uih, ToolbarItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_removed (internal->uih_corba, internal->item->path, &ev);

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_removed (PortableServer_Servant servant,
		      CORBA_char *path,
		      CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	ToolbarItemLocalInternal *internal_cb;

	internal_cb = toolbar_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received removal notification for toolbar item I don't own [%s]!\n", path);
		return;
	}

	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [TOOLBAR_ITEM_REMOVED],
			 path, internal_cb->callback_data);

	toolbar_local_remove_item (uih, path);
}

static void
toolbar_toplevel_item_remove_data (GnomeUIHandler *uih, ToolbarItemInternal *internal)
{
	CORBA_Environment ev;
	char *orig_key;
	char *path;
	GList *l;

	g_assert (internal != NULL);

	path = g_strdup (internal->item->path);

	/*
	 * Get the list of toolbar items which match this path and remove
	 * the specified toolbar item.
	 */
	g_hash_table_lookup_extended (uih->top->path_to_toolbar_item, path,
				      (gpointer *) &orig_key, (gpointer *) &l);
	g_hash_table_remove (uih->top->path_to_toolbar_item, path);
	g_free (orig_key);

	l = g_list_remove (l, internal);

	if (l != NULL) {
		g_hash_table_insert (uih->top->path_to_toolbar_item, g_strdup (path), l);
	} else {
		/*
		 * Remove this path entry from the parent's child list.
		 */
		toolbar_toplevel_item_remove_parent_entry (uih, path, TRUE);
	}

	/*
	 * Free the internal data structures associated with this
	 * item.
	 */
	CORBA_exception_init (&ev);
	CORBA_Object_release (internal->uih_corba, &ev);
	CORBA_exception_free (&ev);

	toolbar_free (internal->item);

	g_slist_free (internal->radio_items);
	g_free (internal);
	g_free (path);
}

static void
toolbar_toplevel_remove_item_internal (GnomeUIHandler *uih, ToolbarItemInternal *internal)
{
	gboolean is_head;
	char *path;

	is_head = toolbar_toplevel_item_is_head (uih, internal);

	path = g_strdup (internal->item->path);

	/*
	 * Destroy the widgets associated with this item.
	 */
	if (is_head)
		toolbar_toplevel_item_remove_widgets (uih, path);

	/*
	 * Notify the item's owner that the item has been removed.
	 */
	toolbar_toplevel_item_remove_notify (uih, internal);

	/*
	 * Remove the internal data structures associated with this
	 * item.
	 */
	toolbar_toplevel_item_remove_data (uih, internal);

	/*
	 * If we just destroyed an active toolbar item (one
	 * which was mapped to widgets on the screen), then
	 * we need to replace it.
	 */
	if (is_head) {
		ToolbarItemInternal *replacement;

		replacement = toolbar_toplevel_get_item (uih, path);

		if (replacement == NULL) {
			g_free (path);
			return;
		}

		/*
		 * Notify the replacement's owner that its item
		 * is being reinstated.
		 */
		toolbar_toplevel_item_reinstate_notify (uih, replacement->item->path);

		toolbar_toplevel_item_create_widgets (uih, replacement);
	}

	g_free (path);
}

static void
toolbar_toplevel_remove_item (GnomeUIHandler *uih, char *path, GNOME_UIHandler uih_corba)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item_for_containee (uih, path, uih_corba);

	g_return_if_fail (internal != NULL);

	toolbar_toplevel_remove_item_internal (uih, internal);
}

static void
toolbar_remote_remove_item (GnomeUIHandler *uih, char *path)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_toolbar_remove_item (uih->top_level_uih,
					     gnome_object_corba_objref (GNOME_OBJECT (uih)),
					     path,
					     &ev);

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_remove_item (PortableServer_Servant servant,
			  GNOME_UIHandler containee_uih,
			  CORBA_char *path,
			  CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	
	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	toolbar_toplevel_remove_item (uih, path, containee_uih);
}

/**
 * gnome_ui_handler_toolbar_remove:
 */
void
gnome_ui_handler_toolbar_remove (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_remote_remove_item (uih, path);
	else
		toolbar_toplevel_remove_item (uih, path, gnome_object_corba_objref (GNOME_OBJECT (uih)));
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_fetch (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback (GnomeUIHandler *uih,
					    GnomeUIHandlerCallbackFunc callback)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback_data (GnomeUIHandler *uih,
						 gpointer callback_data)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

gint
gnome_ui_handler_toolbar_get_pos (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);

	g_warning ("Unimplemented toolbar method");
	return -1;
}

static GnomeUIHandlerMenuItemType
toolbar_uiinfo_type_to_uih (GnomeUIInfoType uii_type)
{
	switch (uii_type) {
	case GNOME_APP_UI_ENDOFINFO:
		return GNOME_UI_HANDLER_TOOLBAR_END;

	case GNOME_APP_UI_ITEM:
		return GNOME_UI_HANDLER_TOOLBAR_ITEM;

	case GNOME_APP_UI_TOGGLEITEM:
		return GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM;

	case GNOME_APP_UI_RADIOITEMS:
		return GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP;

	case GNOME_APP_UI_SUBTREE:
		g_error ("Toolbar subtree doesn't make sense!\n");

	case GNOME_APP_UI_SEPARATOR:
		return GNOME_UI_HANDLER_TOOLBAR_SEPARATOR;

	case GNOME_APP_UI_HELP:
		g_error ("Help unimplemented.\n"); /* FIXME */

	case GNOME_APP_UI_BUILDER_DATA:
		g_error ("Builder data - what to do?\n"); /* FIXME */

	case GNOME_APP_UI_ITEM_CONFIGURABLE:
		g_warning ("Configurable item!");
		return GNOME_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_SUBTREE_STOCK:
		g_error ("Toolbar subtree doesn't make sense!\n");

	default:
		g_warning ("Unknown UIInfo Type: %d", uii_type);
		return GNOME_UI_HANDLER_TOOLBAR_ITEM;
	}
}

static void
toolbar_parse_uiinfo_one (GnomeUIHandlerToolbarItem *item, GnomeUIInfo *uii)
{
	item->path = NULL;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	item->type = toolbar_uiinfo_type_to_uih (uii->type);

	item->label = g_strdup (uii->label);
	item->hint = g_strdup (uii->hint);

	item->pos = -1;

	if (item->type == GNOME_UI_HANDLER_TOOLBAR_ITEM ||
	    item->type == GNOME_UI_HANDLER_TOOLBAR_RADIOITEM ||
	    item->type == GNOME_UI_HANDLER_TOOLBAR_TOGGLEITEM)
		item->callback = uii->moreinfo;
	item->callback_data = uii->user_data;

	item->pixmap_type = uiinfo_pixmap_type_to_uih (uii->pixmap_type);
	item->pixmap_data = pixmap_copy_data (item->pixmap_type, uii->pixmap_info);
	item->accelerator_key = uii->accelerator_key;
	item->ac_mods = uii->ac_mods;
}

static void
toolbar_parse_uiinfo_tree (GnomeUIHandlerToolbarItem *tree, GnomeUIInfo *uii)
{
	toolbar_parse_uiinfo_one (tree, uii);

	if (tree->type == GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP) {
		tree->children = gnome_ui_handler_toolbar_parse_uiinfo_list (uii->moreinfo);
	}
}

static void
toolbar_parse_uiinfo_one_with_data (GnomeUIHandlerToolbarItem *item, GnomeUIInfo *uii, void *data)
{
	toolbar_parse_uiinfo_one (item, uii);
	item->callback_data = data;
}

static void
toolbar_parse_uiinfo_tree_with_data (GnomeUIHandlerToolbarItem *tree, GnomeUIInfo *uii, void *data)
{
	toolbar_parse_uiinfo_one_with_data (tree, uii, data);

	if (tree->type == GNOME_UI_HANDLER_TOOLBAR_RADIOGROUP) {
		tree->children = gnome_ui_handler_toolbar_parse_uiinfo_list_with_data (uii->moreinfo, data);
	}
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_one:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_one (GnomeUIInfo *uii)
{
	GnomeUIHandlerToolbarItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (GnomeUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_one (item, uii);

	return item;
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_list:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_list (GnomeUIInfo *uii)
{
	GnomeUIHandlerToolbarItem *list;
	GnomeUIHandlerToolbarItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the GnomeUIHandlerToolbarItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (GnomeUIHandlerToolbarItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		toolbar_parse_uiinfo_tree (curr_uih, curr_uii);

	/* Parse the terminal entry. */
	toolbar_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_tree:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_tree (GnomeUIInfo *uii)
{
	GnomeUIHandlerToolbarItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (GnomeUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_tree (item_tree, uii);

	return item_tree;
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_one_with_data:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_one_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerToolbarItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (GnomeUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_one_with_data (item, uii, data);

	return item;
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_list_with_data:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_list_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerToolbarItem *list;
	GnomeUIHandlerToolbarItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the GnomeUIHandlerToolbarItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (GnomeUIHandlerToolbarItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		toolbar_parse_uiinfo_tree_with_data (curr_uih, curr_uii, data);

	/* Parse the terminal entry. */
	toolbar_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * gnome_ui_handler_toolbar_parse_uiinfo_tree_with_data:
 */
GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_tree_with_data (GnomeUIInfo *uii, void *data)
{
	GnomeUIHandlerToolbarItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (GnomeUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_tree_with_data (item_tree, uii, data);

	return item_tree;
}

void
gnome_ui_handler_toolbar_free_one (GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (item != NULL);

	g_warning ("Unimplemented toolbar method");
}

void gnome_ui_handler_toolbar_free_list (GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (item != NULL);

	g_warning ("Unimplemented toolbar method");
}

void gnome_ui_handler_toolbar_free_tree (GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (item != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_set_sensitivity (GnomeUIHandler *uih, char *path,
					  gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gboolean
gnome_ui_handler_toolbar_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_warning ("Unimplemented toolbar method");
	return FALSE;
}

void
gnome_ui_handler_toolbar_set_label (GnomeUIHandler *uih, char *path,
				    gchar *label)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gchar *
gnome_ui_handler_toolbar_get_label (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

void
gnome_ui_handler_toolbar_set_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_get_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_set_accel (GnomeUIHandler *uih, char *path,
				    guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_get_accel (GnomeUIHandler *uih, char *path,
				    guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_set_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc callback,
				       gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
gnome_ui_handler_toolbar_get_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc *callback,
				       gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gboolean
gnome_ui_handler_toolbar_toggle_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL,FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_warning ("Unimplemented toolbar method");
	return FALSE;
}


void
gnome_ui_handler_toolbar_toggle_set_state (GnomeUIHandler *uih, char *path,
					   gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gboolean
gnome_ui_handler_toolbar_radio_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_warning ("Unimplemented toolbar method");
	return FALSE;
}

void
gnome_ui_handler_toolbar_radio_set_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}
