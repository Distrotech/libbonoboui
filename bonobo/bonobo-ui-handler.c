/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Bonobo::UIHandler.
 *
 * Copyright 1999 Helix Code, Inc.
 *
 * Authors:
 *    Nat Friedman  (nat@helixcode.com)
 *    Michael Meeks (michael@helixcode.com)
 */

/*
 * Notes to self:
 *  - fill out internal Menu structures with regard to const correctly.
 *  - toolbar position handling
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
#include <bonobo/bonobo-main.h>
#include <bonobo/gnome-bonobo-widget.h>
#include <bonobo/bonobo-ui-handler.h>
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
static BonoboObjectClass *bonobo_ui_handler_parent_class;

/*
 * The entry point vectors for the Bonobo_UIHandler server used to
 * handle remote menu/toolbar merging.
 */
POA_Bonobo_UIHandler__vepv bonobo_ui_handler_vepv;

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
	 * The BonoboUIHandler.
	 */
	BonoboUIHandler *uih;

	/*
	 * A copy of the BonoboUIHandlerMenuItem for this toolbar item.
	 */
	BonoboUIHandlerMenuItem *item;

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
	Bonobo_UIHandler uih_corba;

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

	BonoboUIHandlerCallbackFunc	 callback;
	gpointer			 callback_data;
} MenuItemLocalInternal;

/*
 * This structure is used to maintain an internal representation of a
 * toolbar item.
 */

struct _ToolbarItemInternal {
	/*
	 * The BonoboUIHandler.
	 */
	BonoboUIHandler *uih;

	/*
	 * A copy of the BonoboUIHandlerToolbarItem for this toolbar item.
	 */
	BonoboUIHandlerToolbarItem *item;

	/*
	 * The UIHandler CORBA interface for the containee which owns
	 * this particular menu item.
	 */
	Bonobo_UIHandler uih_corba;

	/*
	 * If this item is a radio group, then this list points to the
	 * ToolbarItemInternal structures for the members of the group.
	 */
	GSList *radio_items;

	/*
	 * In the world of toolbars, only radio groups can have
	 * children.
	 */
	GList *children;

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
	Bonobo_UIHandler		 uih_corba;

	GtkOrientation		 orientation;
	GtkToolbarStyle		 style;
	GtkToolbarSpaceStyle	 space_style;
	int			 space_size;
	GtkReliefStyle		 relief;
};

struct _ToolbarItemLocalInternal {
	BonoboUIHandlerCallbackFunc callback;
	gpointer	           callback_data;
};

struct _ToolbarToolbarLocalInternal {
	GList		*children;
};

typedef struct {
	CORBA_char       *label, *hint;
	CORBA_long        accelerator_key, ac_mods, pos;
	CORBA_boolean     toggle_state, sensitive, active;
} UIRemoteAttributeData;

typedef struct {
	Bonobo_UIHandler_ToolbarOrientation orientation;
	Bonobo_UIHandler_ToolbarStyle       style;
	Bonobo_UIHandler_ToolbarSpaceStyle  space_style;
	Bonobo_UIHandler_ReliefStyle        relief_style;
	CORBA_long                         space_size;
	CORBA_boolean                      sensitive;
} ToolbarRemoteAttributeData;

/*
 * Prototypes for some internal functions.
 */
static void                       init_ui_handler_corba_class           (void);
static gboolean			  uih_toplevel_check_toplevel		(BonoboUIHandler *uih);
static void			  uih_toplevel_add_containee		(BonoboUIHandler *uih, Bonobo_UIHandler containee);
static void			  pixmap_free_data			(BonoboUIHandlerPixmapType pixmap_type,
									 gpointer pixmap_info);
static gpointer			  pixmap_copy_data			(BonoboUIHandlerPixmapType pixmap_type,
									 const gconstpointer pixmap_info);
static gpointer			  pixmap_xpm_copy_data			(const gconstpointer data);
static Bonobo_UIHandler_iobuf	 *pixmap_data_to_corba			(BonoboUIHandlerPixmapType type, gpointer data);
static Bonobo_UIHandler_PixmapType pixmap_corba_to_type			(Bonobo_UIHandler_PixmapType type);
static Bonobo_UIHandler_PixmapType pixmap_type_to_corba			(BonoboUIHandlerPixmapType type);
static gint			  pixmap_xpm_get_length			(const gconstpointer data, int *num_lines);

/*
 * Prototypes for some internal menu functions.
 */
static void			  menu_toplevel_remove_item_internal	(BonoboUIHandler *uih, MenuItemInternal *internal,
									 gboolean replace);
static void			  menu_toplevel_create_hint		(BonoboUIHandler *uih, BonoboUIHandlerMenuItem *item,
									 GtkWidget *menu_item);
static void			  menu_toplevel_set_toggle_state_internal(BonoboUIHandler *uih, MenuItemInternal *internal,
				 					 gboolean state);
static void			  menu_toplevel_set_sensitivity_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
									 gboolean sensitivity);
static void			  menu_toplevel_set_radio_state_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
				 					 gboolean state);
static gint			  menu_toplevel_item_activated		(GtkWidget *menu_item, MenuItemInternal *internal);

/*
 * Prototypes for some internal Toolbar functions.
 */
static void			  toolbar_toplevel_remove_item_internal (BonoboUIHandler *uih,
									 ToolbarItemInternal *internal);

/*
 * Convenience functions to get attributes
 * NB. can be re-used for toolbars.
 */
static UIRemoteAttributeData *
menu_remote_attribute_data_get (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs = g_new0 (UIRemoteAttributeData, 1);
	CORBA_Environment        ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_menu_get_attributes (uih->top_level_uih,
					     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					     path, &attrs->sensitive, &attrs->pos, &attrs->label, &attrs->hint,
					     &attrs->accelerator_key, &attrs->ac_mods, &attrs->toggle_state, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		g_free (attrs);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return attrs;
}

static UIRemoteAttributeData *
toolbar_item_remote_attribute_data_get (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs = g_new0 (UIRemoteAttributeData, 1);
	CORBA_Environment        ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_toolbar_item_get_attributes (uih->top_level_uih,
						     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
						     path, &attrs->sensitive, &attrs->active,
						     &attrs->pos, &attrs->label, &attrs->hint,
						     &attrs->accelerator_key, &attrs->ac_mods,
						     &attrs->toggle_state, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		g_free (attrs);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return attrs;
}

static void
ui_remote_attribute_data_free (UIRemoteAttributeData *attrs)
{
	g_return_if_fail (attrs != NULL);

	if (attrs->hint)
		CORBA_free (attrs->hint);
	if (attrs->label)
		CORBA_free (attrs->label);

	g_free (attrs);
}

/*
 * Convenience function to set attributes + free data.
 */
static gboolean
menu_remote_attribute_data_set (BonoboUIHandler *uih, const char *path,
				UIRemoteAttributeData *attrs)
{
	gboolean success = TRUE;
	CORBA_Environment ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_menu_set_attributes (uih->top_level_uih,
					     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					     path, attrs->sensitive, attrs->pos, attrs->label, attrs->hint,
					     attrs->accelerator_key, attrs->ac_mods, attrs->toggle_state, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		success = FALSE;
	}
	ui_remote_attribute_data_free (attrs);

	CORBA_exception_free   (&ev);

	return success;
}

static gboolean
toolbar_item_remote_attribute_data_set (BonoboUIHandler *uih, const char *path,
					UIRemoteAttributeData *attrs)
{
	gboolean success = TRUE;
	CORBA_Environment ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_toolbar_item_set_attributes (uih->top_level_uih,
						     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
						     path, attrs->sensitive, attrs->active,
						     attrs->pos, attrs->label, attrs->hint,
						     attrs->accelerator_key, attrs->ac_mods, attrs->toggle_state, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		success = FALSE;
	}
	ui_remote_attribute_data_free (attrs);

	CORBA_exception_free   (&ev);

	return success;
}

static ToolbarRemoteAttributeData *
toolbar_remote_attribute_data_get (BonoboUIHandler *uih, const char *path)
{
	ToolbarRemoteAttributeData *attrs = g_new0 (ToolbarRemoteAttributeData, 1);
	CORBA_Environment        ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_toolbar_get_attributes (uih->top_level_uih,
						bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
						path, &attrs->orientation, &attrs->style,
						&attrs->space_style, &attrs->relief_style,
						&attrs->space_size, &attrs->sensitive, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		CORBA_exception_free (&ev);
		g_free (attrs);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return attrs;
}

static void
toolbar_remote_attribute_data_free (ToolbarRemoteAttributeData *attrs)
{
	g_return_if_fail (attrs != NULL);
	g_free (attrs);
}

/*
 * Convenience function to set attributes + free data.
 */
static gboolean
toolbar_remote_attribute_data_set (BonoboUIHandler *uih, const char *path,
				   ToolbarRemoteAttributeData *attrs)
{
	gboolean success = TRUE;
	CORBA_Environment ev;

	CORBA_exception_init   (&ev);

	Bonobo_UIHandler_toolbar_set_attributes (uih->top_level_uih,
						bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
						path, attrs->orientation, attrs->style,
						attrs->space_style, attrs->relief_style,
						attrs->space_size, attrs->sensitive, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
		success = FALSE;
	}
	toolbar_remote_attribute_data_free (attrs);

	CORBA_exception_free   (&ev);

	return success;
}

/*

 * Basic GtkObject management.
 *
 * These are the BonoboUIHandler construction/deconstruction functions.
 */
static CORBA_Object
create_bonobo_ui_handler (BonoboObject *object)
{
	POA_Bonobo_UIHandler *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_UIHandler *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_ui_handler_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_UIHandler__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return bonobo_object_activate_servant (object, servant);
}

static void
bonobo_ui_handler_destroy (GtkObject *object)
{
/*	BonoboUIHandler *uih = BONOBO_UI_HANDLER (object);*/

	g_warning ("gnome ui handler destroy not fully implemented");

	GTK_OBJECT_CLASS (bonobo_ui_handler_parent_class)->destroy (object);
}

static void
bonobo_ui_handler_class_init (BonoboUIHandlerClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_ui_handler_parent_class = gtk_type_class (bonobo_object_get_type ());

	object_class->destroy = bonobo_ui_handler_destroy;

	uih_signals [MENU_ITEM_ACTIVATED] =
		gtk_signal_new ("menu_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [MENU_ITEM_REMOVED] =
		gtk_signal_new ("menu_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [MENU_ITEM_OVERRIDDEN] =
		gtk_signal_new ("menu_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_reinstated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_ACTIVATED] =
		gtk_signal_new ("toolbar_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_REMOVED] =
		gtk_signal_new ("toolbar_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	uih_signals [TOOLBAR_ITEM_OVERRIDDEN] =
		gtk_signal_new ("toolbar_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_reinstated),
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
bonobo_ui_handler_init (BonoboObject *object)
{
}

/**
 * bonobo_ui_handler_get_type:
 *
 * Returns: the GtkType corresponding to the BonoboUIHandler class.
 */
GtkType
bonobo_ui_handler_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"IDL:GNOME/UIHandler:1.0",
			sizeof (BonoboUIHandler),
			sizeof (BonoboUIHandlerClass),
			(GtkClassInitFunc) bonobo_ui_handler_class_init,
			(GtkObjectInitFunc) bonobo_ui_handler_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_ui_handler_construct:
 */
BonoboUIHandler *
bonobo_ui_handler_construct (BonoboUIHandler *ui_handler, Bonobo_UIHandler corba_uihandler)
{
	MenuItemLocalInternal *root_cb;
	MenuItemInternal *root;
	GList *l;

	g_return_val_if_fail (ui_handler != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (ui_handler), NULL);
	g_return_val_if_fail (corba_uihandler != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (ui_handler), corba_uihandler);

	ui_handler->top = g_new0 (BonoboUIHandlerTopLevelData, 1);

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
	root->uih_corba = bonobo_object_corba_objref (BONOBO_OBJECT (ui_handler));
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
 * bonobo_ui_handler_new:
 */
BonoboUIHandler *
bonobo_ui_handler_new (void)
{
	Bonobo_UIHandler corba_uihandler;
	BonoboUIHandler *uih;
	
	uih = gtk_type_new (bonobo_ui_handler_get_type ());

	corba_uihandler = create_bonobo_ui_handler (BONOBO_OBJECT (uih));
	if (corba_uihandler == CORBA_OBJECT_NIL) {
		gtk_object_destroy (GTK_OBJECT (uih));
		return NULL;
	}
	

	return bonobo_ui_handler_construct (uih, corba_uihandler);
}

/**
 * bonobo_ui_handler_set_app:
 *
 */
void
bonobo_ui_handler_set_app (BonoboUIHandler *uih, GnomeApp *app)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));

	uih->top->app = app;

	if (uih->top->accelgroup == NULL && app->accel_group != NULL)
		bonobo_ui_handler_set_accelgroup (uih, app->accel_group);
}

static void
impl_register_containee (PortableServer_Servant servant,
			 Bonobo_UIHandler        containee_uih,
			 CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	uih_toplevel_add_containee (uih, containee_uih);
}

static Bonobo_UIHandler
impl_get_toplevel (PortableServer_Servant servant,
		   CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), CORBA_OBJECT_NIL);

	if (uih->top_level_uih == CORBA_OBJECT_NIL)
		return CORBA_Object_duplicate (bonobo_object_corba_objref (BONOBO_OBJECT (uih)), ev);

	return CORBA_Object_duplicate (uih->top_level_uih, ev);
}

/**
 * bonobo_ui_handler_set_container:
 */
void
bonobo_ui_handler_set_container (BonoboUIHandler *uih, Bonobo_UIHandler container)
{
	CORBA_Environment ev;
	Bonobo_UIHandler top_level;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	/*
	 * Unregister with our current top-level container, if we have one.
	 */
	bonobo_ui_handler_unset_container (uih);

	/*
	 * Our container will hand us a pointer to the top-level
	 * UIHandler, with which we will communicate directly
	 * for the remainder of the session.  We never speak
	 * to our container again.
	 */
	CORBA_exception_init (&ev);

	top_level = Bonobo_UIHandler_get_toplevel (container, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) container, &ev);
	else {
		uih->top_level_uih = CORBA_Object_duplicate (top_level, &ev);

		/* Register with the top-level UIHandler. */
		Bonobo_UIHandler_register_containee (
			uih->top_level_uih,
			bonobo_object_corba_objref (BONOBO_OBJECT (uih)), &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			bonobo_object_check_env (
				BONOBO_OBJECT (uih),
				(CORBA_Object) uih->top_level_uih, &ev);
		}

	}
	CORBA_exception_free (&ev);
}

/**
 * bonobo_ui_handler_uset_container:
 */
void
bonobo_ui_handler_unset_container (BonoboUIHandler *uih)
{
	CORBA_Environment ev;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	CORBA_exception_init (&ev);

	/*
	 * Unregister with an existing container, if we have one
	 */
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {

		Bonobo_UIHandler_unregister_containee (
			uih->top_level_uih,
			bonobo_object_corba_objref (BONOBO_OBJECT (uih)), &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			bonobo_object_check_env (
				BONOBO_OBJECT (uih),
				(CORBA_Object) uih->top_level_uih, &ev);
		}
		
		CORBA_Object_release (uih->top_level_uih, &ev);

		/* FIXME: Check the exception */

		uih->top_level_uih = CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);
	/* FIXME: probably I should remove all local menu item data ?*/
}

static void
uih_toplevel_add_containee (BonoboUIHandler *uih, Bonobo_UIHandler containee)
{
	CORBA_Environment ev;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	uih->top->containee_uihs = g_list_prepend (uih->top->containee_uihs, CORBA_Object_duplicate (containee, &ev));
	CORBA_exception_free (&ev);
}

typedef struct {
	BonoboUIHandler *uih;
	Bonobo_UIHandler containee;
	GList *removal_list;
} removal_closure_t;

/*
 * This is a helper function used by bonobo_ui_handler_remove_containee
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
toolbar_toplevel_find_containee_items (gpointer path, gpointer value, gpointer user_data)
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
		ToolbarItemInternal *internal = (ToolbarItemInternal *) curr->data;

		if (CORBA_Object_is_equivalent (internal->uih_corba, closure->containee, &ev)) {
			closure->removal_list = g_list_prepend (closure->removal_list, internal);
		}
	}
	CORBA_exception_free (&ev);
}

static gint
toolbar_toplevel_prune_compare_function (gconstpointer a, gconstpointer b)
{
	ToolbarItemInternal *ai, *bi;
	int len_a, len_b;

	ai = (ToolbarItemInternal *) a;
	bi = (ToolbarItemInternal *) b;

	len_a = strlen (ai->item->path);
	len_b = strlen (bi->item->path);

	if (len_a > len_b)
		return -1;
	if (len_a < len_b)
		return 1;
	return 0;
}

static void
uih_toplevel_unregister_containee (BonoboUIHandler *uih, Bonobo_UIHandler containee)
{
	removal_closure_t *closure;
	Bonobo_UIHandler remove_me;
	CORBA_Environment ev;
	GList *curr;

	/*
	 * Remove the containee from the list of containees.
	 */
	remove_me = NULL;
	for (curr = uih->top->containee_uihs; curr != NULL; curr = curr->next) {

		CORBA_exception_init (&ev);
		if (CORBA_Object_is_equivalent ((Bonobo_UIHandler) curr->data, containee, &ev)) {
			remove_me = curr->data;
			CORBA_exception_free (&ev);
			break;
		}
		CORBA_exception_free (&ev);
	}

	if (remove_me == CORBA_OBJECT_NIL)
		return;

	uih->top->containee_uihs = g_list_remove (uih->top->containee_uihs, remove_me);
	CORBA_exception_init (&ev);
	CORBA_Object_release (remove_me, &ev);
	CORBA_exception_free (&ev);

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
	 * This takes care of that by putting the children first in
	 * the list.
	 */
	closure->removal_list = g_list_sort (closure->removal_list, menu_toplevel_prune_compare_function);

	/* Remove them */
	for (curr = closure->removal_list; curr != NULL; curr = curr->next)
		menu_toplevel_remove_item_internal (uih, (MenuItemInternal *) curr->data, TRUE);
	g_list_free (closure->removal_list);
	g_free (closure);

	/*
	 * Create a simple closure to pass some data into the removal
	 * function.
	 */
	closure = g_new0 (removal_closure_t, 1);
	closure->uih = uih;
	closure->containee = containee;

	/*
	 * Remove the toolbar items associated with this containee.
	 */

	/* Build a list of itnernal toolbar item structures to remove. */
	g_hash_table_foreach (uih->top->path_to_toolbar_item, toolbar_toplevel_find_containee_items, closure);

	closure->removal_list = g_list_sort (closure->removal_list, toolbar_toplevel_prune_compare_function);
	
	/* Remove them */
	for (curr = closure->removal_list; curr != NULL; curr = curr->next)
		toolbar_toplevel_remove_item_internal (uih, (ToolbarItemInternal *) curr->data);
	g_list_free (closure->removal_list);
	g_free (closure);
}

static void
uih_remote_unregister_containee (BonoboUIHandler *uih, Bonobo_UIHandler containee)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_unregister_containee (uih->top_level_uih, containee, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) uih->top_level_uih, &ev);
	
	CORBA_exception_free (&ev);
}

static void
impl_unregister_containee (PortableServer_Servant servant,
			   Bonobo_UIHandler containee_uih,
			   CORBA_Environment *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	uih_toplevel_unregister_containee (uih, containee_uih);
}

/**
 * bonobo_ui_handler_remove_containee:
 */
void
bonobo_ui_handler_remove_containee (BonoboUIHandler *uih, Bonobo_UIHandler containee)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		uih_remote_unregister_containee (uih, containee);
		return;
	}

	uih_toplevel_unregister_containee (uih, containee);
}

/**
 * bonobo_ui_handler_set_accelgroup:
 */
void
bonobo_ui_handler_set_accelgroup (BonoboUIHandler *uih, GtkAccelGroup *accelgroup)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	uih->top->accelgroup = accelgroup;
}

/**
 * bonobo_ui_handler_get_accelgroup:
 */
GtkAccelGroup *
bonobo_ui_handler_get_accelgroup (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

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
bonobo_ui_handler_build_path (char *comp1, ...)
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
path_tokenize (const char *path)
{
	int num_toks;
	int tok_idx;
	gchar **toks;
	const char *p, *q;

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
path_get_parent (const char *path)
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
uih_toplevel_create_pixmap (GtkWidget *window, BonoboUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
{
	GtkWidget *pixmap;
	char *name;

	pixmap = NULL;

	switch (pixmap_type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		break;

	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		pixmap = gnome_stock_pixmap_widget (window, pixmap_info);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		name = gnome_pixmap_file (pixmap_info);

		if (name == NULL)
			g_warning ("Could not find GNOME pixmap file %s",
				   (char *) pixmap_info);
		else {
			pixmap = gnome_pixmap_new_from_file (name);
		}

		g_free (name);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		if (pixmap_info != NULL)
			pixmap = gnome_pixmap_new_from_xpm_d (pixmap_info);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_RGB_DATA:
	case BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("Unsupported pixmap type (RGB[A]_DATA)\n");
		break;

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		
	}

	return pixmap;
}

static void
pixmap_free_data (BonoboUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
{
	int num_lines, i;

	switch (pixmap_type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		break;

	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		g_free (pixmap_info);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		pixmap_xpm_get_length (pixmap_info, &num_lines);

		for (i = 0; i < num_lines; i ++)
			g_free (((char **) pixmap_info) [i]);

		g_free (pixmap_info);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_RGB_DATA:
	case BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("Unsupported pixmap type (RGB[A]_DATA)\n");
		break;

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		
	}
}

static gpointer 
pixmap_copy_data (BonoboUIHandlerPixmapType pixmap_type, const gconstpointer pixmap_info)
{
	switch (pixmap_type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup ((char *) pixmap_info);

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		return g_strdup ((char *) pixmap_info);

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		return pixmap_xpm_copy_data (pixmap_info);

	default:
		g_warning ("Unknown pixmap type: %d\n", pixmap_type);
		return NULL;
	}
}

static Bonobo_UIHandler_PixmapType
pixmap_type_to_corba (BonoboUIHandlerPixmapType type)
{
	switch (type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		return Bonobo_UIHandler_PixmapTypeNone;
	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		return Bonobo_UIHandler_PixmapTypeStock;
	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		return Bonobo_UIHandler_PixmapTypeFilename;
	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		return Bonobo_UIHandler_PixmapTypeXPMData;
	case BONOBO_UI_HANDLER_PIXMAP_RGB_DATA:
		return Bonobo_UIHandler_PixmapTypeRGBData;
	case BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA:
		return Bonobo_UIHandler_PixmapTypeRGBAData;
	default:
		g_warning ("pixmap_type_to_corba: Unknown pixmap type [%d]!\n", (int) type);
		return Bonobo_UIHandler_PixmapTypeNone;
	}
}


static BonoboUIHandlerPixmapType
pixmap_corba_to_type (Bonobo_UIHandler_PixmapType type)
{
	switch (type) {
	case Bonobo_UIHandler_PixmapTypeNone:
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
	case Bonobo_UIHandler_PixmapTypeStock:
		return BONOBO_UI_HANDLER_PIXMAP_STOCK;
	case Bonobo_UIHandler_PixmapTypeFilename:
		return BONOBO_UI_HANDLER_PIXMAP_FILENAME;
	case Bonobo_UIHandler_PixmapTypeXPMData:
		return BONOBO_UI_HANDLER_PIXMAP_XPM_DATA;
	case Bonobo_UIHandler_PixmapTypeRGBData:
		return BONOBO_UI_HANDLER_PIXMAP_RGB_DATA;
	case Bonobo_UIHandler_PixmapTypeRGBAData:
		return BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA;
	default:
		g_warning ("pixmap_corba_to_type: Unknown pixmap type [%d]!\n", (int) type);
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
	}
}

static gint
pixmap_xpm_get_length (const gconstpointer data, int *num_lines)
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
pixmap_xpm_copy_data (const gconstpointer src)
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

static Bonobo_UIHandler_iobuf *
pixmap_data_to_corba (BonoboUIHandlerPixmapType type, gpointer data)
{
	Bonobo_UIHandler_iobuf *buffer;
	gpointer temp_xpm_buffer;

	buffer = Bonobo_UIHandler_iobuf__alloc ();
	CORBA_sequence_set_release (buffer, TRUE);

	switch (type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		buffer->_length = strlen ((char *) data) + 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (strlen ((char *) data));
		strcpy (buffer->_buffer, (char *) data);
		return buffer;

	case BONOBO_UI_HANDLER_PIXMAP_RGB_DATA:
	case BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("pixmap_data_to_corba: Pixmap type (RGB[A]) not yet supported!\n");
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
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
pixmap_corba_to_data (Bonobo_UIHandler_PixmapType   corba_pixmap_type,
		      const Bonobo_UIHandler_iobuf *corba_pixmap_data)
{
	BonoboUIHandlerPixmapType type;
	gpointer pixmap_data;

	type = pixmap_corba_to_type (corba_pixmap_type);

	switch (type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup (corba_pixmap_data->_buffer);

	case BONOBO_UI_HANDLER_PIXMAP_RGB_DATA:
	case BONOBO_UI_HANDLER_PIXMAP_RGBA_DATA:
		g_warning ("pixmap_corba_to_data: Pixmap type (RGB[A]) not yet supported!\n");
		return NULL;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		pixmap_data = pixmap_xpm_unflatten (corba_pixmap_data->_buffer,
						    corba_pixmap_data->_length);
		return pixmap_data;

	default:
		g_warning ("pixmap_corba_to_data: Unknown pixmap type [%d]\n", type);
		return NULL;
	}
}

static BonoboUIHandlerPixmapType
uiinfo_pixmap_type_to_uih (GnomeUIPixmapType ui_type)
{
	switch (ui_type) {
	case GNOME_APP_PIXMAP_NONE:
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
	case GNOME_APP_PIXMAP_STOCK:
		return BONOBO_UI_HANDLER_PIXMAP_STOCK;
	case GNOME_APP_PIXMAP_FILENAME:
		return BONOBO_UI_HANDLER_PIXMAP_FILENAME;
	case GNOME_APP_PIXMAP_DATA:
		return BONOBO_UI_HANDLER_PIXMAP_XPM_DATA;
	default:
		g_warning ("Unknown GnomeUIPixmapType: %d\n", ui_type);
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
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

/**
 * menu_make_item:
 * @path: 
 * @type: 
 * @label: 
 * @hint: 
 * @pos: 
 * @pixmap_type: 
 * @pixmap_data: 
 * @accelerator_key: 
 * @ac_mods: 
 * @callback: 
 * @callback_data: 
 * 
 * FIXME: this is _really_ broken.
 *
 * This Creates a _temporary_ copy of the menu item data,
 * when passed to menu_copy_item this makes valid copies of the
 * menu data. Why this split !
 * 
 * Return value: 
 **/
static BonoboUIHandlerMenuItem *
menu_make_item (const char *path, BonoboUIHandlerMenuItemType type,
		const char *label, const char *hint,
		int pos, BonoboUIHandlerPixmapType pixmap_type,
		gpointer pixmap_data,
		guint accelerator_key, GdkModifierType ac_mods,
		BonoboUIHandlerCallbackFunc callback,
		gpointer callback_data)

{
	BonoboUIHandlerMenuItem *item;

	item = g_new0 (BonoboUIHandlerMenuItem, 1);
	item->path        = path;
	item->type        = type;
	item->label       = label;
	item->hint        = hint;
	item->pos         = pos;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods         = ac_mods;
	item->callback        = callback;
	item->callback_data   = callback_data;

	return item;
}

static BonoboUIHandlerMenuItem *
menu_copy_item (BonoboUIHandlerMenuItem *item)
{
	BonoboUIHandlerMenuItem *copy;	

	copy = g_new0 (BonoboUIHandlerMenuItem, 1);

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
menu_free_data (BonoboUIHandlerMenuItem *item)
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
menu_free (BonoboUIHandlerMenuItem *item)
{
	menu_free_data (item);
	g_free (item);
}

static Bonobo_UIHandler_MenuType
menu_type_to_corba (BonoboUIHandlerMenuItemType type)
{
	switch (type) {

	case BONOBO_UI_HANDLER_MENU_END:
		g_warning ("Passing MenuTypeEnd through CORBA!\n");
		return Bonobo_UIHandler_MenuTypeEnd;

	case BONOBO_UI_HANDLER_MENU_ITEM:
		return Bonobo_UIHandler_MenuTypeItem;

	case BONOBO_UI_HANDLER_MENU_SUBTREE:
		return Bonobo_UIHandler_MenuTypeSubtree;

	case BONOBO_UI_HANDLER_MENU_RADIOITEM:
		return Bonobo_UIHandler_MenuTypeRadioItem;

	case BONOBO_UI_HANDLER_MENU_RADIOGROUP:
		return Bonobo_UIHandler_MenuTypeRadioGroup;

	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		return Bonobo_UIHandler_MenuTypeToggleItem;

	case BONOBO_UI_HANDLER_MENU_SEPARATOR:
		return Bonobo_UIHandler_MenuTypeSeparator;

	default:
		g_warning ("Unknown BonoboUIHandlerMenuItemType %d!\n", (int) type);
		return Bonobo_UIHandler_MenuTypeItem;
		
	}
}

static BonoboUIHandlerMenuItemType
menu_corba_to_type (Bonobo_UIHandler_MenuType type)
{
	switch (type) {
	case Bonobo_UIHandler_MenuTypeEnd:
		g_warning ("Warning: Getting MenuTypeEnd from CORBA!");
		return BONOBO_UI_HANDLER_MENU_END;

	case Bonobo_UIHandler_MenuTypeItem:
		return BONOBO_UI_HANDLER_MENU_ITEM;

	case Bonobo_UIHandler_MenuTypeSubtree:
		return BONOBO_UI_HANDLER_MENU_SUBTREE;

	case Bonobo_UIHandler_MenuTypeRadioItem:
		return BONOBO_UI_HANDLER_MENU_RADIOITEM;

	case Bonobo_UIHandler_MenuTypeRadioGroup:
		return BONOBO_UI_HANDLER_MENU_RADIOGROUP;

	case Bonobo_UIHandler_MenuTypeToggleItem:
		return BONOBO_UI_HANDLER_MENU_TOGGLEITEM;

	case Bonobo_UIHandler_MenuTypeSeparator:
		return BONOBO_UI_HANDLER_MENU_SEPARATOR;
	default:
		g_warning ("Unknown Bonobo_UIHandler menu type %d!\n", (int) type);
		return BONOBO_UI_HANDLER_MENU_ITEM;
		
	}
}
/**
 * bonobo_ui_handler_create_menubar:
 */
void
bonobo_ui_handler_create_menubar (BonoboUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (uih->top->app != NULL);
	g_return_if_fail (GNOME_IS_APP (uih->top->app));

	uih->top->menubar = gtk_menu_bar_new ();

	gnome_app_set_menus (uih->top->app, GTK_MENU_BAR (uih->top->menubar));
}

/**
 * bonobo_ui_handler_set_menubar:
 */
void
bonobo_ui_handler_set_menubar (BonoboUIHandler *uih, GtkWidget *menubar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (menubar);
	g_return_if_fail (GTK_IS_MENU_BAR (menubar));

	uih->top->menubar = menubar;
}

/**
 * bonobo_ui_handler_get_menubar:
 */
GtkWidget *
bonobo_ui_handler_get_menubar (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	return uih->top->menubar;
}

/**
 * bonobo_ui_handler_create_popup_menu:
 */
void
bonobo_ui_handler_create_popup_menu (BonoboUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	uih->top->menubar = gtk_menu_new ();
}

static void
menu_toplevel_popup_deactivated (GtkMenuShell *menu_shell, gpointer data)
{
	gtk_main_quit ();
}

/**
 * bonobo_ui_handler_do_popup_menu:
 */
void
bonobo_ui_handler_do_popup_menu (BonoboUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	gtk_menu_popup (GTK_MENU (uih->top->menubar), NULL, NULL, NULL, NULL, 0,
			GDK_CURRENT_TIME);

	gtk_signal_connect (GTK_OBJECT (uih->top->menubar), "deactivate",
			    GTK_SIGNAL_FUNC (menu_toplevel_popup_deactivated),
			    NULL);

	gtk_main ();
}

/**
 * bonobo_ui_handler_set_statusbar:
 */
void
bonobo_ui_handler_set_statusbar (BonoboUIHandler *uih, GtkWidget *statusbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (statusbar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (statusbar) || GNOME_IS_APPBAR (statusbar));

	uih->top->statusbar = statusbar;
}

/**
 * bonobo_ui_handler_get_statusbar:
 */
GtkWidget *
bonobo_ui_handler_get_statusbar (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	return uih->top->statusbar;
}

static void
uih_local_do_path (const char *parent_path, const char *item_label,
		   char **item_path)
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
 * BonoboUIHandlerMenuItem is consistent with the path of its parent.
 * If the item does not have a path, one is created for it.  If the
 * path is not consistent with the parent's path, an error is printed
 */
static void
menu_local_do_path (const char *parent_path, BonoboUIHandlerMenuItem *item)
{
	uih_local_do_path (parent_path, item->label, & item->path);
}


/*
 * Callback data is maintained in a stack, like toplevel internal menu
 * item data.  This is so that a local UIHandler can override its
 * own menu items.
 */
static MenuItemLocalInternal *
menu_local_get_item (BonoboUIHandler *uih, const char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_menu_callback, path);
	if (l == NULL)
		return NULL;

	return (MenuItemLocalInternal *) l->data;
}

static void
menu_local_remove_parent_entry (BonoboUIHandler *uih, const char *path,
				gboolean warn)
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
menu_local_add_parent_entry (BonoboUIHandler *uih, const char *path)
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
menu_local_create_item (BonoboUIHandler *uih, const char *parent_path,
			BonoboUIHandlerMenuItem *item)
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
menu_local_remove_item (BonoboUIHandler *uih, const char *path)
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
		g_hash_table_insert (uih->path_to_menu_callback, g_strdup (path), new_list);	
}

static void
menu_local_remove_item_recursive (BonoboUIHandler *uih, const char *path)
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
menu_toplevel_get_item (BonoboUIHandler *uih, const char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_menu_item, path);

	if (l == NULL)
		return NULL;

	return (MenuItemInternal *) l->data;
}

static MenuItemInternal *
menu_toplevel_get_item_for_containee (BonoboUIHandler *uih, const char *path,
				      Bonobo_UIHandler containee_uih)
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
menu_toplevel_item_is_head (BonoboUIHandler *uih, MenuItemInternal *internal)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_menu_item, internal->item->path);

	if (l->data == internal)
		return TRUE;

	return FALSE;
}

static gboolean
uih_toplevel_check_toplevel (BonoboUIHandler *uih)
{
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		g_warning ("CORBA method invoked on non-toplevel UIHandler!\n");
		return FALSE;
	}

	return TRUE;
}

static GtkWidget *
menu_toplevel_get_widget (BonoboUIHandler *uih, const char *path)
{
	return g_hash_table_lookup (uih->top->path_to_menu_widget, path);
}

/*
 * This function maps a path to either a subtree menu shell or the
 * top-level menu bar.  It returns NULL if the specified path is
 * not a subtree path (or "/").
 */
static GtkWidget *
menu_toplevel_get_shell (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;
	char *parent_path;

	if (! strcmp (path, "/"))
		return uih->top->menubar;

	internal = menu_toplevel_get_item (uih, path);
	if (internal->item->type == BONOBO_UI_HANDLER_MENU_RADIOGROUP) {
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
menu_toplevel_create_label (BonoboUIHandler *uih, BonoboUIHandlerMenuItem *item,
			    GtkWidget *parent_menu_shell_widget, GtkWidget *menu_widget)
{
	GtkWidget *label;
	guint keyval;

	label = gtk_accel_label_new (item->label);

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
menu_toplevel_create_item_widget (BonoboUIHandler *uih, const char *parent_path,
				  BonoboUIHandlerMenuItem *item)
{
	GtkWidget *menu_widget;
	MenuItemInternal *rg;

	switch (item->type) {
	case BONOBO_UI_HANDLER_MENU_ITEM:
	case BONOBO_UI_HANDLER_MENU_SUBTREE:

		/*
		 * Create a GtkMenuItem widget if it's a plain item.  If it contains
		 * a pixmap, create a GtkPixmapMenuItem.
		 */
		if (item->pixmap_data != NULL && item->pixmap_type != BONOBO_UI_HANDLER_PIXMAP_NONE) {
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

	case BONOBO_UI_HANDLER_MENU_RADIOITEM:
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

	case BONOBO_UI_HANDLER_MENU_TOGGLEITEM:
		menu_widget = gtk_check_menu_item_new ();

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_widget), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_widget), FALSE);
		break;

	case BONOBO_UI_HANDLER_MENU_SEPARATOR:

		/*
		 * A separator is just a plain menu item with its sensitivity
		 * set to FALSE.
		 */
		menu_widget = gtk_menu_item_new ();
		gtk_widget_set_sensitive (menu_widget, FALSE);
		break;

	case BONOBO_UI_HANDLER_MENU_RADIOGROUP:
		g_assert_not_reached ();
		return NULL;

	default:
		g_warning ("menu_toplevel_create_item_widget: Invalid BonoboUIHandlerMenuItemType %d\n",
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
menu_toplevel_install_global_accelerators (BonoboUIHandler *uih,
					   BonoboUIHandlerMenuItem *item,
					   GtkWidget *menu_widget)
{
	static guint save_accels_id = 0;
	char *globalaccelstr;

	if (! save_accels_id)
		save_accels_id = gtk_quit_add (1, menu_toplevel_save_accels, NULL);

	g_return_if_fail (gnome_app_id != NULL);
	globalaccelstr = g_strconcat (gnome_app_id, ">",
				      item->path, "<", NULL);
	gtk_item_factory_add_foreign (menu_widget, globalaccelstr, uih->top->accelgroup,
				      item->accelerator_key, item->ac_mods);
	g_free (globalaccelstr);


}

static void
menu_toplevel_connect_signal (GtkWidget *menu_widget, MenuItemInternal *internal)
{
	gtk_signal_connect (GTK_OBJECT (menu_widget), "activate",
			    GTK_SIGNAL_FUNC (menu_toplevel_item_activated), internal);
}

/*
 * This function does all the work associated with creating a menu
 * item's GTK widgets.
 */
static void
menu_toplevel_create_widgets (BonoboUIHandler *uih, const char *parent_path,
			      MenuItemInternal *internal)
{
	BonoboUIHandlerMenuItem *item;
	GtkWidget *parent_menu_shell_widget;
	GtkWidget *menu_widget;

	item = internal->item;

	/* No widgets to create for a radio group. */
	if (item->type == BONOBO_UI_HANDLER_MENU_RADIOGROUP)
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
	if (item->type == BONOBO_UI_HANDLER_MENU_SUBTREE) {
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
	if (internal->item->type == BONOBO_UI_HANDLER_MENU_TOGGLEITEM)
		menu_toplevel_set_toggle_state_internal (uih, internal, internal->active);
	else if (internal->item->type == BONOBO_UI_HANDLER_MENU_RADIOITEM)
		menu_toplevel_set_radio_state_internal (uih, internal, internal->active);
}

static void
menu_toplevel_create_widgets_recursive (BonoboUIHandler *uih, MenuItemInternal *internal)
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
menu_toplevel_create_hint (BonoboUIHandler *uih, BonoboUIHandlerMenuItem *item, GtkWidget *menu_item)
{
	char *old_hint;

	old_hint = gtk_object_get_data (GTK_OBJECT (menu_item), "menu_item_hint");
	if (old_hint)
		g_free (old_hint);

	/* FIXME: Do we have to i18n this here? */
	if (item->hint == NULL)
		gtk_object_set_data (GTK_OBJECT (menu_item),
				     "menu_item_hint", NULL);
	else
		gtk_object_set_data (GTK_OBJECT (menu_item),
				     "menu_item_hint", g_strdup (item->hint));

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
menu_toplevel_item_activated (GtkWidget *menu_item, MenuItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_activated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (internal->uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}
	
	CORBA_exception_free (&ev);

	return FALSE;
}

static void
impl_menu_activated (PortableServer_Servant servant,
		     const CORBA_char      *path,
		     CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received activation notification about a menu item I don't own [%s]!\n", path);
		return;
	}

	if (internal_cb->callback != NULL)
		internal_cb->callback (uih, internal_cb->callback_data, path);
	
	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_ACTIVATED], path, internal_cb->callback_data);
}

/*
 * The job of this function is to remove a child's entry from its
 * parent's list of children.
 */
static void
menu_toplevel_remove_parent_entry (BonoboUIHandler *uih, const char *path,
				   gboolean warn)
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

/*
 * Adds an entry to an item's parent's list of children.
 */
static void
menu_toplevel_add_parent_entry (BonoboUIHandler *uih, BonoboUIHandlerMenuItem *item)
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
 * This routine stores the top-level UIHandler's internal data about a
 * given menu item.
 */
static MenuItemInternal *
menu_toplevel_store_data (BonoboUIHandler *uih, Bonobo_UIHandler uih_corba, BonoboUIHandlerMenuItem *item)
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
menu_toplevel_override_notify (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;
	CORBA_Environment ev;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_overridden (internal->uih_corba, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (internal->uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
menu_toplevel_override_notify_recursive (BonoboUIHandler *uih, const char *path)
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
impl_menu_overridden (PortableServer_Servant servant,
		      const CORBA_char      *path,
		      CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received override notification for a menu item I don't own [%s]!\n", path);
		return;
	}
	
	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_OVERRIDDEN],
			 path, internal_cb->callback_data);
}

static void
menu_toplevel_remove_widgets (BonoboUIHandler *uih, const char *path)
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
menu_toplevel_remove_widgets_recursive (BonoboUIHandler *uih, const char *path)
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
 * This function is called to check if a new menu item is going to
 * override an existing one.  If it does, then the old (overridden)
 * menu item's widgets must be destroyed to make room for the new
 * item. The "menu_item_overridden" signal is propagated down from the
 * top-level UIHandler to the appropriate containee.
 */
static void
menu_toplevel_check_override (BonoboUIHandler *uih, const char *path)
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
menu_toplevel_create_item (BonoboUIHandler *uih, const char *parent_path,
			   BonoboUIHandlerMenuItem *item, Bonobo_UIHandler uih_corba)
{
	MenuItemInternal *internal_data;

	/*
	 * Check to see if there is already a menu item by this name,
	 * and if so, override it.
	 */
	menu_toplevel_check_override (uih, item->path);

	/*
	 * Duplicate the BonoboUIHandlerMenuItem structure and store
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
menu_remote_create_item (BonoboUIHandler *uih, const char *parent_path,
			 BonoboUIHandlerMenuItem *item)
{
	Bonobo_UIHandler_iobuf *pixmap_buf;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	pixmap_buf = pixmap_data_to_corba (item->pixmap_type, item->pixmap_data);

	Bonobo_UIHandler_menu_create (uih->top_level_uih,
				     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
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

	bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) uih->top_level_uih, &ev);

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buf);
}

static void
impl_menu_create (PortableServer_Servant   servant,
		  const Bonobo_UIHandler    containee_uih,
		  const CORBA_char        *path,
		  Bonobo_UIHandler_MenuType menu_type,
		  const CORBA_char        *label,
		  const CORBA_char        *hint,
		  CORBA_long               pos,
		  const Bonobo_UIHandler_PixmapType pixmap_type,
		  const Bonobo_UIHandler_iobuf     *pixmap_data,
		  CORBA_unsigned_long              accelerator_key,
		  CORBA_long                       modifier,
		  CORBA_Environment               *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	BonoboUIHandlerMenuItem *item;
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
 * bonobo_ui_handler_menu_add_one:
 */
void
bonobo_ui_handler_menu_add_one (BonoboUIHandler *uih, const char *parent_path, BonoboUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
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
	menu_toplevel_create_item (uih, parent_path, item, bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
}

/**
 * bonobo_ui_handler_menu_add_list:
 */
void
bonobo_ui_handler_menu_add_list (BonoboUIHandler *uih, const char *parent_path,
				BonoboUIHandlerMenuItem *item)
{
	BonoboUIHandlerMenuItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; curr->type != BONOBO_UI_HANDLER_MENU_END; curr ++)
		bonobo_ui_handler_menu_add_tree (uih, parent_path, curr);
}

/**
 * bonobo_ui_handler_menu_add_tree:
 */
void
bonobo_ui_handler_menu_add_tree (BonoboUIHandler *uih, const char *parent_path,
				BonoboUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Add this menu item.
	 */
	bonobo_ui_handler_menu_add_one (uih, parent_path, item);

	/*
	 * Recursive add its children.
	 */
	if (item->children != NULL)
		bonobo_ui_handler_menu_add_list (uih, item->path, item->children);
}

/**
 * bonobo_ui_handler_menu_new:
 */
void
bonobo_ui_handler_menu_new (BonoboUIHandler *uih, const char *path,
			   BonoboUIHandlerMenuItemType type,
			   const char *label, const char *hint,
			   int pos, BonoboUIHandlerPixmapType pixmap_type,
			   gpointer pixmap_data, guint accelerator_key,
			   GdkModifierType ac_mods, BonoboUIHandlerCallbackFunc callback,
			   gpointer callback_data)
{
	BonoboUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	item = menu_make_item (path, type, label, hint, pos,
			       pixmap_type, pixmap_data, accelerator_key, ac_mods,
			       callback, callback_data);

	parent_path = path_get_parent (path);
	g_return_if_fail (parent_path != NULL);

	bonobo_ui_handler_menu_add_one (uih, parent_path, item);

	g_free (item);
	g_free (parent_path);
}

/**
 * bonobo_ui_handler_menu_new_item:
 */
void
bonobo_ui_handler_menu_new_item (BonoboUIHandler *uih, const char *path,
				const char *label, const char *hint, int pos,
				BonoboUIHandlerPixmapType pixmap_type,
				gpointer pixmap_data, guint accelerator_key,
				GdkModifierType ac_mods,
				BonoboUIHandlerCallbackFunc callback,
				gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_ITEM,
				   label, hint, pos, pixmap_type,
				   pixmap_data, accelerator_key, ac_mods,
				   callback, callback_data);
}

/**
 * bonobo_ui_handler_menu_new_subtree:
 */
void
bonobo_ui_handler_menu_new_subtree (BonoboUIHandler *uih, const char *path,
				   const char *label, const char *hint, int pos,
				   BonoboUIHandlerPixmapType pixmap_type,
				   gpointer pixmap_data, guint accelerator_key,
				   GdkModifierType ac_mods)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_SUBTREE,
				   label, hint, pos, pixmap_type, pixmap_data,
				   accelerator_key, ac_mods, NULL, NULL);
}

/**
 * bonobo_ui_handler_menu_new_separator:
 */
void
bonobo_ui_handler_menu_new_separator (BonoboUIHandler *uih, const char *path, int pos)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_SEPARATOR,
				   NULL, NULL, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_menu_new_radiogroup:
 */
void
bonobo_ui_handler_menu_new_radiogroup (BonoboUIHandler *uih, const char *path)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_RADIOGROUP,
				   NULL, NULL, -1, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_menu_radioitem:
 */
void
bonobo_ui_handler_menu_new_radioitem (BonoboUIHandler *uih, const char *path,
				     const char *label, const char *hint, int pos,
				     guint accelerator_key, GdkModifierType ac_mods,
				     BonoboUIHandlerCallbackFunc callback,
				     gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_RADIOITEM,
				   label, hint, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

/**
 * bonobo_ui_handler_menu_new_toggleitem:
 */
void
bonobo_ui_handler_menu_new_toggleitem (BonoboUIHandler *uih, const char *path,
				      const char *label, const char *hint, int pos,
				      guint accelerator_key, GdkModifierType ac_mods,
				      BonoboUIHandlerCallbackFunc callback,
				      gpointer callback_data)
{
	bonobo_ui_handler_menu_new (uih, path, BONOBO_UI_HANDLER_MENU_TOGGLEITEM,
				   label, hint, pos, BONOBO_UI_HANDLER_PIXMAP_NONE,
				   NULL, accelerator_key, ac_mods, callback, callback_data);
}

static void
menu_toplevel_remove_data (BonoboUIHandler *uih, MenuItemInternal *internal)
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

	/*
	 * FIXME
	 *
	 * This g_slist_free() (or maybe the other one in this file)
	 * seems to corrupt the SList allocator's free list
	 */
/*	g_slist_free (internal->radio_items); */
	g_free (internal);
	g_free (path);
}

static void
menu_toplevel_remove_notify (BonoboUIHandler *uih, MenuItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_removed (internal->uih_corba, internal->item->path, &ev);

	CORBA_exception_free (&ev);
}

static void
impl_menu_removed (PortableServer_Servant servant,
		   const CORBA_char      *path,
		   CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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
menu_toplevel_reinstate_notify (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;
	CORBA_Environment ev;

	internal = menu_toplevel_get_item (uih, path);

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_reinstated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
menu_toplevel_reinstate_notify_recursive (BonoboUIHandler *uih, const char *path)
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
		      const CORBA_char      *path,
		      CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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
menu_toplevel_remove_item_internal (BonoboUIHandler *uih, MenuItemInternal *internal, gboolean replace)
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
	    internal->item->type != BONOBO_UI_HANDLER_MENU_RADIOGROUP)
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
menu_toplevel_remove_item (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	menu_toplevel_remove_item_internal (uih, internal, TRUE);
}

static void
menu_remote_remove_item (BonoboUIHandler *uih, const char *path)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_remove (uih->top_level_uih,
				     bonobo_object_corba_objref (BONOBO_OBJECT (uih)), path,
				     &ev);

	bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) uih->top_level_uih, &ev);

	CORBA_exception_free (&ev);
}

static void
impl_menu_remove (PortableServer_Servant servant,
		  Bonobo_UIHandler        containee_uih,
		  const CORBA_char      *path,
		  CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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
 * bonobo_ui_handler_menu_remove:
 */
void
bonobo_ui_handler_menu_remove (BonoboUIHandler *uih, const char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
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

static BonoboUIHandlerMenuItem *
menu_toplevel_fetch (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL)
		return NULL;

	return menu_copy_item (internal->item);
}

static BonoboUIHandlerMenuItem *
menu_remote_fetch (BonoboUIHandler *uih, const char *path)
{
	BonoboUIHandlerMenuItem *item;
	CORBA_Environment ev;
	CORBA_boolean exists;

	Bonobo_UIHandler_MenuType corba_menu_type;
	CORBA_char *corba_label;
	CORBA_char *corba_hint;
	CORBA_long corba_pos;
	Bonobo_UIHandler_PixmapType corba_pixmap_type;
	Bonobo_UIHandler_iobuf *corba_pixmap_iobuf;
	CORBA_long corba_accelerator_key;
	CORBA_long corba_modifier_type;

	item = g_new0 (BonoboUIHandlerMenuItem, 1);

	CORBA_exception_init (&ev);

	exists = Bonobo_UIHandler_menu_fetch (uih->top_level_uih,
					     path, &corba_menu_type, &corba_label,
					     &corba_hint, &corba_pos,
					     &corba_pixmap_type, &corba_pixmap_iobuf,
					     &corba_accelerator_key, &corba_modifier_type,
					     &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
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
impl_menu_fetch (PortableServer_Servant      servant,
		 const CORBA_char           *path,
		 Bonobo_UIHandler_MenuType   *type,
		 CORBA_char                **label,
		 CORBA_char                **hint,
		 CORBA_long                 *pos,
		 Bonobo_UIHandler_PixmapType *pixmap_type,
		 Bonobo_UIHandler_iobuf     **pixmap_data,
		 CORBA_unsigned_long        *accelerator_key,
		 CORBA_long                 *modifier,
		 CORBA_Environment          *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	BonoboUIHandlerMenuItem *item;
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
 * bonobo_ui_handler_menu_fetch_one:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_fetch_one (BonoboUIHandler *uih, const char *path)
{
	MenuItemLocalInternal *internal_cb;
	BonoboUIHandlerMenuItem *item;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
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
 * bonobo_ui_handler_menu_fetch_tree:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_fetch_tree (BonoboUIHandler *uih, const char *path)
{
	/* FIXME: Implement me */
	g_error ("Implement fetch tree\n");
	return NULL;
}

/**
 * bonobo_ui_handler_menu_free_one:
 */
void
bonobo_ui_handler_menu_free_one (BonoboUIHandlerMenuItem *item)
{
	g_return_if_fail (item != NULL);

	menu_free (item);
}

/**
 * bonobo_ui_handler_menu_free_list:
 * @array: A NULL-terminated array of BonoboUIHandlerMenuItem structures to be freed.
 *
 * Frees the list of menu items in @array and frees the array itself.
 */
void
bonobo_ui_handler_menu_free_list (BonoboUIHandlerMenuItem *array)
{
	BonoboUIHandlerMenuItem *curr;

	g_return_if_fail (array != NULL);

	for (curr = array; curr->type != BONOBO_UI_HANDLER_MENU_END; curr ++) {
		if (curr->children != NULL)
			bonobo_ui_handler_menu_free_list (curr->children);

		menu_free_data (curr);
	}

	g_free (array);
}

/**
 * bonobo_ui_handler_menu_free_tree:
 */
void
bonobo_ui_handler_menu_free_tree (BonoboUIHandlerMenuItem *tree)
{
	g_return_if_fail (tree != NULL);

	if (tree->type == BONOBO_UI_HANDLER_MENU_END)
		return;

	if (tree->children != NULL)
		bonobo_ui_handler_menu_free_list (tree->children);

	menu_free (tree);
}

static GList *
menu_toplevel_get_children (BonoboUIHandler *uih, const char *parent_path)
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
menu_remote_get_children (BonoboUIHandler *uih, const char *parent_path)
{
	Bonobo_UIHandler_StringSeq *childseq;
	CORBA_Environment ev;
	GList *children;
	int i;
	gboolean fail = FALSE;
	
	CORBA_exception_init (&ev);
	if (! Bonobo_UIHandler_menu_get_children (uih->top_level_uih,
						 (CORBA_char *) parent_path, &childseq,
						 &ev))
		return NULL;
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (BONOBO_OBJECT (uih), uih->top_level_uih, &ev);
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
impl_menu_get_children (PortableServer_Servant      servant,
			const CORBA_char           *parent_path,
			Bonobo_UIHandler_StringSeq **child_paths,
			CORBA_Environment          *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	MenuItemInternal *internal;
	int num_children, i;
	GList *curr;

	g_return_val_if_fail (uih_toplevel_check_toplevel (uih), FALSE);

	internal = menu_toplevel_get_item (uih, parent_path);

	if (internal == NULL)
		return FALSE;

	num_children = g_list_length (internal->children);

	*child_paths = Bonobo_UIHandler_StringSeq__alloc ();
	(*child_paths)->_buffer = CORBA_sequence_CORBA_string_allocbuf (num_children);
	(*child_paths)->_length = num_children;

	for (curr = internal->children, i = 0; curr != NULL; curr = curr->next, i ++)
		(*child_paths)->_buffer [i] = CORBA_string_dup ((char *) curr->data);

	return TRUE;
}

/**
 * bonobo_ui_handler_menu_get_child_paths:
 */
GList *
bonobo_ui_handler_menu_get_child_paths (BonoboUIHandler *uih, const char *parent_path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
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

static BonoboUIHandlerMenuItemType
menu_uiinfo_type_to_uih (GnomeUIInfoType uii_type)
{
	switch (uii_type) {
	case GNOME_APP_UI_ENDOFINFO:
		return BONOBO_UI_HANDLER_MENU_END;

	case GNOME_APP_UI_ITEM:
		return BONOBO_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_TOGGLEITEM:
		return BONOBO_UI_HANDLER_MENU_TOGGLEITEM;

	case GNOME_APP_UI_RADIOITEMS:
		return BONOBO_UI_HANDLER_MENU_RADIOGROUP;

	case GNOME_APP_UI_SUBTREE:
		return BONOBO_UI_HANDLER_MENU_SUBTREE;

	case GNOME_APP_UI_SEPARATOR:
		return BONOBO_UI_HANDLER_MENU_SEPARATOR;

	case GNOME_APP_UI_HELP:
		g_error ("Help unimplemented."); /* FIXME */

	case GNOME_APP_UI_BUILDER_DATA:
		g_error ("Builder data - what to do?"); /* FIXME */

	case GNOME_APP_UI_ITEM_CONFIGURABLE:
		g_warning ("Configurable item!");
		return BONOBO_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_SUBTREE_STOCK:
		return BONOBO_UI_HANDLER_MENU_SUBTREE;

	default:
		g_warning ("Unknown UIInfo Type: %d", uii_type);
		return BONOBO_UI_HANDLER_MENU_ITEM;
	}
}

static void
menu_parse_uiinfo_one (BonoboUIHandlerMenuItem *item, GnomeUIInfo *uii)
{
	item->path = NULL;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	item->type = menu_uiinfo_type_to_uih (uii->type);

	item->label = COPY_STRING (L_(uii->label));
	item->hint = COPY_STRING (L_(uii->hint));

	item->pos = -1;

	if (item->type == BONOBO_UI_HANDLER_MENU_ITEM ||
	    item->type == BONOBO_UI_HANDLER_MENU_RADIOITEM ||
	    item->type == BONOBO_UI_HANDLER_MENU_TOGGLEITEM)
		item->callback = uii->moreinfo;
	item->callback_data = uii->user_data;

	item->pixmap_type = uiinfo_pixmap_type_to_uih (uii->pixmap_type);
	item->pixmap_data = pixmap_copy_data (item->pixmap_type, uii->pixmap_info);
	item->accelerator_key = uii->accelerator_key;
	item->ac_mods = uii->ac_mods;
}

static void
menu_parse_uiinfo_tree (BonoboUIHandlerMenuItem *tree, GnomeUIInfo *uii)
{
	menu_parse_uiinfo_one (tree, uii);

	if (tree->type == BONOBO_UI_HANDLER_MENU_SUBTREE ||
	    tree->type == BONOBO_UI_HANDLER_MENU_RADIOGROUP) {
		tree->children = bonobo_ui_handler_menu_parse_uiinfo_list (uii->moreinfo);
	}
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_one:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_one (GnomeUIInfo *uii)
{
	BonoboUIHandlerMenuItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (BonoboUIHandlerMenuItem, 1);

	menu_parse_uiinfo_one (item, uii);
	
	return item;
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_list:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_list (GnomeUIInfo *uii)
{
	BonoboUIHandlerMenuItem *list;
	BonoboUIHandlerMenuItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the BonoboUIHandlerMenuItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (BonoboUIHandlerMenuItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		menu_parse_uiinfo_tree (curr_uih, curr_uii);

	/* Parse the terminal entry. */
	menu_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_tree:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_tree (GnomeUIInfo *uii)
{
	BonoboUIHandlerMenuItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (BonoboUIHandlerMenuItem, 1);

	menu_parse_uiinfo_tree (item_tree, uii);

	return item_tree;
}

static void
menu_parse_uiinfo_one_with_data (BonoboUIHandlerMenuItem *item, GnomeUIInfo *uii, void *data)
{
	menu_parse_uiinfo_one (item, uii);

	item->callback_data = data;
}

static void
menu_parse_uiinfo_tree_with_data (BonoboUIHandlerMenuItem *tree, GnomeUIInfo *uii, void *data)
{
	menu_parse_uiinfo_one_with_data (tree, uii, data);

	if (tree->type == BONOBO_UI_HANDLER_MENU_SUBTREE ||
	    tree->type == BONOBO_UI_HANDLER_MENU_RADIOGROUP) {
		tree->children = bonobo_ui_handler_menu_parse_uiinfo_list_with_data (uii->moreinfo, data);
	}
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_one_with_data:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_one_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerMenuItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (BonoboUIHandlerMenuItem, 1);

	menu_parse_uiinfo_one_with_data (item, uii, data);

	return item;
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_list_with_data:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_list_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerMenuItem *list;
	BonoboUIHandlerMenuItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the BonoboUIHandlerMenuItem list.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (BonoboUIHandlerMenuItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		menu_parse_uiinfo_tree_with_data (curr_uih, curr_uii, data);

	/* Parse the terminal entry. */
	menu_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * bonobo_ui_handler_menu_parse_uiinfo_tree_with_data:
 */
BonoboUIHandlerMenuItem *
bonobo_ui_handler_menu_parse_uiinfo_tree_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerMenuItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (BonoboUIHandlerMenuItem, 1);

	menu_parse_uiinfo_tree_with_data (item_tree, uii, data);

	return item_tree;
}

static gint
menu_toplevel_get_pos (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);

	g_return_val_if_fail (internal != NULL, -1);

	return internal->item->pos;
}

static gint
menu_remote_get_pos (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gint                     ans;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return -1;
	ans = (gint) attrs->pos;
	ui_remote_attribute_data_free (attrs);

	return ans;
}

/**
 * bonobo_ui_handler_menu_get_pos:
 */
int
bonobo_ui_handler_menu_get_pos (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_pos (uih, path);

	return menu_toplevel_get_pos (uih, path);
}

static void
menu_toplevel_set_sensitivity_internal (BonoboUIHandler *uih, MenuItemInternal *internal, gboolean sensitivity)
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
menu_toplevel_set_sensitivity (BonoboUIHandler *uih, const char *path,
			       gboolean sensitive)
{
	MenuItemInternal *internal;

	/* Update the internal state */
	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);


	menu_toplevel_set_sensitivity_internal (uih, internal, sensitive);
}

static void
menu_remote_set_sensitivity (BonoboUIHandler *uih, const char *path,
			     gboolean sensitive)
{
	UIRemoteAttributeData *attrs;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->sensitive = sensitive;
	menu_remote_attribute_data_set (uih, path, attrs);
}

/**
 * bonobo_ui_handler_menu_set_sensitivity:
 */
void
bonobo_ui_handler_menu_set_sensitivity (BonoboUIHandler *uih, const char *path,
				       gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_sensitivity (uih, path, sensitive);
		return;
	}

	menu_toplevel_set_sensitivity (uih, path, sensitive);
}

static gboolean
menu_toplevel_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	return internal->sensitive;
}

static gboolean
menu_remote_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gboolean                 ans;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return FALSE;
	ans = (gboolean) attrs->sensitive;
	ui_remote_attribute_data_free (attrs);

	return ans;
}

/**
 * bonobo_ui_handler_menu_get_sensitivity:
 */
gboolean
bonobo_ui_handler_menu_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_sensitivity (uih, path);

	return menu_toplevel_get_sensitivity (uih, path);
}

static void
menu_toplevel_set_label_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
				  const gchar *label_text)
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
menu_toplevel_set_label (BonoboUIHandler *uih, const char *path,
			 const gchar *label_text)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_label_internal (uih, internal, label_text);
}

static void
menu_remote_set_label (BonoboUIHandler *uih, const char *path,
		       const gchar *label_text)
{
	UIRemoteAttributeData *attrs;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	CORBA_free (attrs->label);
	attrs->label = CORBA_string_dup (CORBIFY_STRING (label_text));
	menu_remote_attribute_data_set (uih, path, attrs);
}

static void
toolbar_item_remote_set_label (BonoboUIHandler *uih, const char *path,
			       const gchar *label_text)
{
	UIRemoteAttributeData *attrs;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	CORBA_free (attrs->label);
	attrs->label = CORBA_string_dup (CORBIFY_STRING (label_text));
	toolbar_item_remote_attribute_data_set (uih, path, attrs);
}

/**
 * bonobo_ui_handler_menu_set_label:
 */
void
bonobo_ui_handler_menu_set_label (BonoboUIHandler *uih, const char *path,
				 const gchar *label_text)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_label (uih, path, label_text);
		return;
	}

	menu_toplevel_set_label (uih, path, label_text);
}

static gchar *
menu_toplevel_get_label (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, NULL);

	if (internal->item->label == NULL)
		return NULL;

	return g_strdup (internal->item->label);
}

static gchar *
menu_remote_get_label (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gchar                   *ans;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return NULL;
	ans = g_strdup (attrs->label);
	ui_remote_attribute_data_free (attrs);

	return ans;
}

/**
 * bonobo_ui_handler_menu_get_label:
 */
gchar *
bonobo_ui_handler_menu_get_label (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_label (uih, path);

	return menu_toplevel_get_label (uih, path);
}

static void
menu_toplevel_set_hint_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
				 const char *hint)
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
menu_toplevel_set_hint (BonoboUIHandler *uih, const char *path,
			const char *hint)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_hint_internal (uih, internal, hint);
}

static void
menu_remote_set_hint (BonoboUIHandler *uih, const char *path,
		      const char *hint)
{
	UIRemoteAttributeData *attrs;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	CORBA_free (attrs->hint);
	attrs->hint = CORBA_string_dup (CORBIFY_STRING (hint));
	menu_remote_attribute_data_set (uih, path, attrs);
}

static void
toolbar_item_remote_set_hint (BonoboUIHandler *uih, const char *path,
			      const char *hint)
{
	UIRemoteAttributeData *attrs;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	CORBA_free (attrs->hint);
	attrs->hint = CORBA_string_dup (CORBIFY_STRING (hint));
	toolbar_item_remote_attribute_data_set (uih, path, attrs);
}

/**
 * bonobo_ui_handler_menu_set_hint:
 */
void
bonobo_ui_handler_menu_set_hint (BonoboUIHandler *uih, const char *path,
				const char *hint)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_hint (uih, path, hint);
		return;
	}

	menu_toplevel_set_hint (uih, path, hint);
}

static gchar *
menu_toplevel_get_hint (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, NULL);

	return g_strdup (internal->item->hint);
}

static gchar *
menu_remote_get_hint (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gchar                   *ans;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return NULL;
	ans = g_strdup (attrs->hint);
	ui_remote_attribute_data_free (attrs);

	return ans;
}

/**
 * bonobo_ui_handler_menu_get_hint:
 */
gchar *
bonobo_ui_handler_menu_get_hint (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_hint (uih, path);

	return menu_toplevel_get_hint (uih, path);
}

static void
menu_toplevel_set_pixmap_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
				   BonoboUIHandlerPixmapType type, gpointer data)
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

inline static void
menu_toplevel_set_pixmap (BonoboUIHandler *uih, const char *path,
			  BonoboUIHandlerPixmapType type, gpointer data)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_pixmap_internal (uih, internal, type, data);
}

static void
menu_remote_set_pixmap (BonoboUIHandler *uih, const char *path,
			BonoboUIHandlerPixmapType type, gpointer data)
{
	Bonobo_UIHandler_iobuf *pixmap_buff;
	CORBA_Environment ev;

	pixmap_buff = pixmap_data_to_corba (type, data);
	
	CORBA_exception_init (&ev);

	Bonobo_UIHandler_menu_set_data (uih->top_level_uih,
				       bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
				       path,
				       pixmap_type_to_corba (type),
				       pixmap_buff,
				       &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buff);
}

static void
impl_menu_set_data (PortableServer_Servant       servant,
		    Bonobo_UIHandler              containee_uih,
		    const CORBA_char            *path,
		    Bonobo_UIHandler_PixmapType   corba_pixmap_type,
		    const Bonobo_UIHandler_iobuf *corba_pixmap_data,
		    CORBA_Environment           *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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

static void
impl_menu_get_data (PortableServer_Servant      servant,
		    Bonobo_UIHandler              containee_uih,
		    const CORBA_char           *path,
		    Bonobo_UIHandler_PixmapType *corba_pixmap_type,
		    Bonobo_UIHandler_iobuf     **corba_pixmap_data,
		    CORBA_Environment          *ev)
{
	/* FIXME: Implement me! */
	g_warning ("Unimplemented: remote get pixmap");
}

/**
 * bonobo_ui_handler_menu_set_pixmap:
 */
void
bonobo_ui_handler_menu_set_pixmap (BonoboUIHandler *uih, const char *path,
				  BonoboUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (data != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_pixmap (uih, path, type, data);
		return;
	}

	menu_toplevel_set_pixmap (uih, path, type, data);
}

static void
menu_toplevel_get_pixmap (BonoboUIHandler *uih, const char *path,
			  BonoboUIHandlerPixmapType *type, gpointer *data)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	*data = pixmap_copy_data (internal->item->pixmap_type, internal->item->pixmap_data);
	*type = internal->item->pixmap_type;
}

static void
menu_remote_get_pixmap (BonoboUIHandler *uih, const char *path,
			BonoboUIHandlerPixmapType *type, gpointer *data)
{
	/* FIXME: Implement me! */
	g_warning ("Unimplemented: remote get pixmap");
}

/**
 * bonobo_ui_handler_get_pixmap:
 */
void
bonobo_ui_handler_menu_get_pixmap (BonoboUIHandler *uih, const char *path,
				  BonoboUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
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
menu_toplevel_set_accel_internal (BonoboUIHandler *uih, MenuItemInternal *internal,
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
menu_toplevel_set_accel (BonoboUIHandler *uih, const char *path,
			 guint accelerator_key, GdkModifierType ac_mods)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_accel_internal (uih, internal, accelerator_key, ac_mods);
}

static void
menu_remote_set_accel (BonoboUIHandler *uih, const char *path,
		       guint accelerator_key, GdkModifierType ac_mods)
{
	UIRemoteAttributeData *attrs;
	
	g_return_if_fail (uih != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->accelerator_key = accelerator_key;
	attrs->ac_mods         = ac_mods;
	menu_remote_attribute_data_set (uih, path, attrs);
}

static void
toolbar_item_remote_set_accel (BonoboUIHandler *uih, const char *path,
			       guint accelerator_key, GdkModifierType ac_mods)
{
	UIRemoteAttributeData *attrs;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->accelerator_key = accelerator_key;
	attrs->ac_mods = ac_mods;
	toolbar_item_remote_attribute_data_set (uih, path, attrs);
}

/**
 * bonobo_ui_handler_menu_set_accel:
 */
void
bonobo_ui_handler_menu_set_accel (BonoboUIHandler *uih, const char *path,
				 guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_accel (uih, path, accelerator_key, ac_mods);
		return;
	}

	menu_toplevel_set_accel (uih, path, accelerator_key, ac_mods);
}

static void
menu_toplevel_get_accel (BonoboUIHandler *uih, const char *path,
			 guint *accelerator_key, GdkModifierType *ac_mods)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	*accelerator_key = internal->item->accelerator_key;
	*ac_mods = internal->item->ac_mods;
}

static void
menu_remote_get_accel (BonoboUIHandler *uih, const char *path,
		       guint *accelerator_key, GdkModifierType *ac_mods)
{
	UIRemoteAttributeData *attrs;

	/* FIXME: sensible error defaults ? */

	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;

	*accelerator_key = (guint) attrs->accelerator_key;
	*ac_mods = (GdkModifierType) attrs->ac_mods;

	ui_remote_attribute_data_free (attrs);
}

static void
toolbar_item_remote_get_accel (BonoboUIHandler *uih, const char *path,
			       guint *accelerator_key, GdkModifierType *ac_mods)
{
	UIRemoteAttributeData *attrs;

	/* FIXME: sensible error defaults ? */

	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;

	*accelerator_key = (guint) attrs->accelerator_key;
	*ac_mods = (GdkModifierType) attrs->ac_mods;

	ui_remote_attribute_data_free (attrs);
}

/**
 * bonobo_ui_handler_menu_get_accel:
 */
void
bonobo_ui_handler_menu_get_accel (BonoboUIHandler *uih, const char *path,
				 guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
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
menu_local_set_callback (BonoboUIHandler *uih, const char *path,
			 BonoboUIHandlerCallbackFunc callback,
			 gpointer callback_data)
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
 * bonobo_ui_handler_menu_set_callback:
 */
void
bonobo_ui_handler_menu_set_callback (BonoboUIHandler *uih, const char *path,
				    BonoboUIHandlerCallbackFunc callback,
				    gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	menu_local_set_callback (uih, path, callback, callback_data);
}

static void
menu_local_get_callback (BonoboUIHandler *uih, const char *path,
			 BonoboUIHandlerCallbackFunc *callback,
			 gpointer *callback_data)
{
	MenuItemLocalInternal *internal_cb;

	internal_cb = menu_local_get_item (uih, path);

	*callback = internal_cb->callback;
	*callback_data = internal_cb->callback_data;
}

/**
 * bonobo_ui_handler_menu_get_callback:
 */
void
bonobo_ui_handler_menu_get_callback (BonoboUIHandler *uih, const char *path,
				    BonoboUIHandlerCallbackFunc *callback,
				    gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (callback_data != NULL);

	menu_local_get_callback (uih, path, callback, callback_data);
}

static void
menu_toplevel_set_toggle_state_internal (BonoboUIHandler *uih, MenuItemInternal *internal, gboolean state)
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
menu_toplevel_set_toggle_state (BonoboUIHandler *uih, const char *path,
				gboolean state)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	menu_toplevel_set_toggle_state_internal (uih, internal, state);
}

static void
menu_remote_set_toggle_state (BonoboUIHandler *uih, const char *path,
			      gboolean state)
{
	UIRemoteAttributeData *attrs;
	
	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->toggle_state = state;
	menu_remote_attribute_data_set (uih, path, attrs);
}

static void
toolbar_item_remote_set_toggle_state (BonoboUIHandler *uih, const char *path,
				      gboolean state)
{
	UIRemoteAttributeData *attrs;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->toggle_state = state;
	toolbar_item_remote_attribute_data_set (uih, path, attrs);
}

/**
 * bonobo_ui_handler_menu_set_toggle_state:
 */
void
bonobo_ui_handler_menu_set_toggle_state (BonoboUIHandler *uih, const char *path,
					gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_toggle_state (uih, path, state);
		return;
	}

	menu_toplevel_set_toggle_state (uih, path, state);
}

static gboolean
menu_toplevel_get_toggle_state (BonoboUIHandler *uih, const char *path)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, FALSE);

	return internal->active;
}

static gboolean
menu_remote_get_toggle_state (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gboolean                 ans;

	attrs = menu_remote_attribute_data_get (uih, path);
	if (!attrs)
		return FALSE;

	ans = (gboolean) attrs->toggle_state;

	ui_remote_attribute_data_free (attrs);

	return ans;
}

/**
 * bonobo_ui_handler_menu_get_toggle_state:
 */
gboolean
bonobo_ui_handler_menu_get_toggle_state (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_toggle_state (uih, path);

	return menu_toplevel_get_toggle_state (uih, path);
}

static void
menu_toplevel_set_radio_state_internal (BonoboUIHandler *uih, MenuItemInternal *internal, gboolean state)
{
	menu_toplevel_set_toggle_state_internal (uih, internal, state);
}

static void
menu_toplevel_set_radio_state (BonoboUIHandler *uih, const char *path,
			       gboolean state)
{
	MenuItemInternal *internal;

	internal = menu_toplevel_get_item_for_containee (uih, path,
							 bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);
	
	menu_toplevel_set_radio_state_internal (uih, internal, state);
}

static void
menu_remote_set_radio_state (BonoboUIHandler *uih, const char *path,
			     gboolean state)
{
	menu_remote_set_toggle_state (uih, path, state);
}

static void
toolbar_item_remote_set_radio_state (BonoboUIHandler *uih, const char *path,
				     gboolean state)
{
	toolbar_item_remote_set_toggle_state (uih, path, state);
}

/**
 * bonobo_ui_handler_menu_set_radio_state:
 */
void
bonobo_ui_handler_menu_set_radio_state (BonoboUIHandler *uih, const char *path,
				       gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		menu_remote_set_radio_state (uih, path, state);
		return;
	}

	menu_toplevel_set_radio_state (uih, path, state);
}

static gboolean
menu_toplevel_get_radio_state (BonoboUIHandler *uih, const char *path)
{
	return menu_toplevel_get_toggle_state (uih, path);
}

static gboolean
menu_remote_get_radio_state (BonoboUIHandler *uih, const char *path)
{
	return menu_remote_get_toggle_state (uih, path);
}

/**
 * bonobo_ui_handler_menu_get_radio_state:
 */
gboolean
bonobo_ui_handler_menu_get_radio_state (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return menu_remote_get_radio_state (uih, path);

	return menu_toplevel_get_radio_state (uih, path);
}

static void
impl_menu_set_attributes (PortableServer_Servant servant,
			  const Bonobo_UIHandler  containee_uih,
			  const CORBA_char      *path,
			  CORBA_boolean          sensitive,
			  CORBA_long             pos,
			  const CORBA_char      *label,
			  const CORBA_char      *hint,
			  CORBA_long             accelerator_key,
			  CORBA_long             ac_mods,
			  CORBA_boolean          toggle_state,
			  CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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

	/* FIXME: menu_set_pos */
	menu_toplevel_set_label_internal (uih, internal, label);
	menu_toplevel_set_hint_internal  (uih, internal, hint);
	menu_toplevel_set_accel_internal (uih, internal, accelerator_key, ac_mods);
	menu_toplevel_set_toggle_state_internal (uih, internal, toggle_state);
}

static void
impl_menu_get_attributes (PortableServer_Servant servant,
			  const Bonobo_UIHandler  containee_uih,
			  const CORBA_char      *path,
			  CORBA_boolean         *sensitive,
			  CORBA_long            *pos,
			  CORBA_char           **label,
			  CORBA_char           **hint,
			  CORBA_long            *accelerator_key,
			  CORBA_long            *ac_mods,
			  CORBA_boolean         *toggle_state,
			  CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	MenuItemInternal *internal;

	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	internal = menu_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	*sensitive       = (CORBA_boolean) internal->sensitive;
	*pos             = (CORBA_long) menu_toplevel_get_pos (uih, (char *) path);
	*label           =  CORBA_string_dup (CORBIFY_STRING (internal->item->label));
	*hint            =  CORBA_string_dup (CORBIFY_STRING (internal->item->hint));
	*accelerator_key = (CORBA_long) internal->item->accelerator_key;
	*ac_mods         = (CORBA_long) internal->item->ac_mods;
	*toggle_state    = (CORBA_boolean) menu_toplevel_get_toggle_state (uih, path);
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

static BonoboUIHandlerToolbarItem *
toolbar_make_item (const char *path, BonoboUIHandlerToolbarItemType type,
		   const char *label, const char *hint, int pos,
		   const Bonobo_Control control,
		   BonoboUIHandlerPixmapType pixmap_type,
		   gconstpointer pixmap_data, guint accelerator_key,
		   GdkModifierType ac_mods,
		   BonoboUIHandlerCallbackFunc callback,
		   gpointer callback_data)
{
	BonoboUIHandlerToolbarItem *item;

	item = g_new0 (BonoboUIHandlerToolbarItem, 1);

	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->pos = pos;
	item->control = control;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods = ac_mods;
	item->callback = callback;
	item->callback_data = callback_data;

	return item;
}

static BonoboUIHandlerToolbarItem *
toolbar_copy_item (BonoboUIHandlerToolbarItem *item)
{
	BonoboUIHandlerToolbarItem *copy;
	CORBA_Environment ev;

	copy = g_new0 (BonoboUIHandlerToolbarItem, 1);

	copy->path = COPY_STRING (item->path);
	copy->type = item->type;
	copy->hint = COPY_STRING (item->hint);
	copy->pos = item->pos;

	copy->label = COPY_STRING (item->label);

	copy->pixmap_data = pixmap_copy_data (item->pixmap_type, item->pixmap_data);
	copy->pixmap_type = item->pixmap_type;

	copy->accelerator_key = item->accelerator_key;
	copy->ac_mods = item->ac_mods;

	copy->callback = item->callback;
	copy->callback_data = item->callback_data;

	CORBA_exception_init (&ev);
	copy->control = CORBA_Object_duplicate (item->control, &ev);
	CORBA_exception_free (&ev);

	return copy;
}

static void
toolbar_free_data (BonoboUIHandlerToolbarItem *item)
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
toolbar_free (BonoboUIHandlerToolbarItem *item)
{
	toolbar_free_data (item);
	g_free (item);
}

static Bonobo_UIHandler_ToolbarType
toolbar_type_to_corba (BonoboUIHandlerToolbarItemType type)
{
	switch (type) {
	case BONOBO_UI_HANDLER_TOOLBAR_END:
		g_warning ("Passing ToolbarTypeEnd through CORBA!\n");
		return Bonobo_UIHandler_ToolbarTypeEnd;

	case BONOBO_UI_HANDLER_TOOLBAR_ITEM:
		return Bonobo_UIHandler_ToolbarTypeItem;

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM:
		return Bonobo_UIHandler_ToolbarTypeRadioItem;

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP:
		return Bonobo_UIHandler_ToolbarTypeRadioGroup;

	case BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM:
		return Bonobo_UIHandler_ToolbarTypeToggleItem;

	case BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR:
		return Bonobo_UIHandler_ToolbarTypeSeparator;

	case BONOBO_UI_HANDLER_TOOLBAR_CONTROL:
		return Bonobo_UIHandler_ToolbarTypeControl;

	default:
		g_warning ("toolbar_type_to_corba: Unknown toolbar type [%d]!\n", (gint) type);
	}

	return BONOBO_UI_HANDLER_TOOLBAR_ITEM;
}

static BonoboUIHandlerToolbarItemType
toolbar_corba_to_type (Bonobo_UIHandler_ToolbarType type)
{
	switch (type) {
	case Bonobo_UIHandler_ToolbarTypeEnd:
		g_warning ("Passing ToolbarTypeEnd through CORBA!\n");
		return BONOBO_UI_HANDLER_TOOLBAR_END;

	case Bonobo_UIHandler_ToolbarTypeItem:
		return BONOBO_UI_HANDLER_TOOLBAR_ITEM;

	case Bonobo_UIHandler_ToolbarTypeRadioItem:
		return BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM;

	case Bonobo_UIHandler_ToolbarTypeRadioGroup:
		return BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP;

	case Bonobo_UIHandler_ToolbarTypeSeparator:
		return BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR;

	case Bonobo_UIHandler_ToolbarTypeToggleItem:
		return BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM;

	case Bonobo_UIHandler_ToolbarTypeControl:
		return BONOBO_UI_HANDLER_TOOLBAR_CONTROL;

	default:
		g_warning ("toolbar_corba_to_type: Unknown toolbar type [%d]!\n", (gint) type);
			
	}

	return BONOBO_UI_HANDLER_TOOLBAR_ITEM;
}


static ToolbarItemLocalInternal *
toolbar_local_get_item (BonoboUIHandler *uih, const char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_toolbar_callback, path);

	if (l == NULL)
		return NULL;

	return (ToolbarItemLocalInternal *) l->data;
}

inline static GtkWidget *
toolbar_toplevel_item_get_widget (BonoboUIHandler *uih, const char *path)
{
	return g_hash_table_lookup (uih->top->path_to_toolbar_item_widget, path);
}

inline static GtkWidget *
toolbar_toplevel_get_widget (BonoboUIHandler *uih, const char *name)
{
	return g_hash_table_lookup (uih->top->name_to_toolbar_widget, name);
}

static ToolbarToolbarLocalInternal *
toolbar_local_get_toolbar (BonoboUIHandler *uih, const char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_toolbar_toolbar, path);

	if (l == NULL)
		return NULL;

	return (ToolbarToolbarLocalInternal *) l->data;
}

static ToolbarToolbarInternal *
toolbar_toplevel_get_toolbar (BonoboUIHandler *uih, const char *name)
{
	GList *l;
	
	l = g_hash_table_lookup (uih->top->name_to_toolbar, name);

	if (l == NULL)
		return NULL;

	return (ToolbarToolbarInternal *) l->data;
}

static ToolbarItemInternal *
toolbar_toplevel_get_item (BonoboUIHandler *uih, const char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, path);

	if (l == NULL)
		return NULL;

	return (ToolbarItemInternal *) l->data;
}

static ToolbarItemInternal *
toolbar_toplevel_get_item_for_containee (BonoboUIHandler *uih, const char *path,
					 Bonobo_UIHandler uih_corba)
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
toolbar_toplevel_item_is_head (BonoboUIHandler *uih, ToolbarItemInternal *internal)
{
	GList *l;

	l = g_hash_table_lookup (uih->top->path_to_toolbar_item, internal->item->path);

	if (l->data == internal)
		return TRUE;

	return FALSE;
}

static char *
toolbar_get_toolbar_name (const char *path)
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
toolbar_toplevel_toolbar_create_widget (BonoboUIHandler *uih, ToolbarToolbarInternal *internal)
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
toolbar_toplevel_item_remove_widgets (BonoboUIHandler *uih, const char *path)
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
toolbar_toplevel_item_remove_widgets_recursive (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);

	if (internal->children != NULL) {
		GList *curr;

		for (curr = internal->children; curr != NULL; curr = curr->next)
			toolbar_toplevel_item_remove_widgets_recursive (
				uih, (char *) curr->data);
	}

	toolbar_toplevel_item_remove_widgets (uih, path);
}

static void
toolbar_local_toolbar_create (BonoboUIHandler *uih, const char *name)
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
		g_hash_table_insert (uih->path_to_toolbar_toolbar, g_strdup (name), l);
	}
}

static void
toolbar_toplevel_toolbar_remove_widgets_recursive (BonoboUIHandler *uih, const char *name)
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

	if (GNOME_IS_DOCK_ITEM (toolbar_widget->parent)) {
		/*
		 * Since GnomeDock's get left floating, and since there is
		 * no good api for removing toolbars we must assasinate our parent.
		 */
		gtk_widget_destroy (toolbar_widget->parent);
	} else {
		/*
		 * just destroy ourselfs then
		 */
		gtk_widget_destroy (toolbar_widget);
	}
}

static void
toolbar_toplevel_item_override_notify (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;
	CORBA_Environment ev;

	internal = toolbar_toplevel_get_item (uih, path);
	g_return_if_fail (internal != NULL);

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_overridden (internal->uih_corba, path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
toolbar_toplevel_item_override_notify_recursive (BonoboUIHandler *uih,
						 const char *path)
{
	ToolbarItemInternal *internal;

	toolbar_toplevel_item_override_notify (uih, path);

	internal = toolbar_toplevel_get_item (uih, path);
	if (internal->children != NULL) {
		GList *curr;

		for (curr = internal->children; curr != NULL; curr = curr->next)
			toolbar_toplevel_item_override_notify_recursive
				(uih, (char *) curr->data);
	}
}

static void
toolbar_toplevel_toolbar_override_notify_recursive (BonoboUIHandler *uih, const char *name)
{
	ToolbarToolbarInternal *internal;
	CORBA_Environment ev;
	char *toolbar_path;
	GList *curr;

	internal = toolbar_toplevel_get_toolbar (uih, name);

	toolbar_path = g_strconcat ("/", name, NULL);

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_overridden (internal->uih_corba, toolbar_path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);

	g_free (toolbar_path);

	for (curr = internal->children; curr != NULL; curr = curr->next)
		toolbar_toplevel_item_override_notify_recursive (uih, (char *) curr->data);
}

static void
toolbar_toplevel_toolbar_check_override (BonoboUIHandler *uih, const char *name)
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
toolbar_toplevel_toolbar_store_data (BonoboUIHandler *uih, const char *name, Bonobo_UIHandler uih_corba)
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
		g_hash_table_insert (uih->top->name_to_toolbar, g_strdup (name), l);
	} else {
		l = g_list_prepend (NULL, internal);
		g_hash_table_insert (uih->top->name_to_toolbar, g_strdup (name), l);
	}

	return internal;
}

static void
toolbar_toplevel_toolbar_create (BonoboUIHandler *uih, const char *name,
				 Bonobo_UIHandler uih_corba)
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
toolbar_remote_toolbar_create (BonoboUIHandler *uih, const char *name)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_create (uih->top_level_uih,
					bonobo_object_corba_objref (BONOBO_OBJECT (uih)), name,
					&ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_create (PortableServer_Servant servant,
		     const Bonobo_UIHandler  containee,
		     const CORBA_char      *name,
		     CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	toolbar_toplevel_toolbar_create (uih, name, containee);
}

/**
 * bonobo_ui_handler_create_toolbar:
 * @uih: UI handle
 * @name: Name of toolbar eg. 'pdf', must not
 * include leading or trailing '/'s
 * 
 * creates a toolbar, at the path given by 'name'.
 * heirarchical toolbars are quite meaningless, hence
 * only a single level of toolbar path is needed.
 * 
 **/
void
bonobo_ui_handler_create_toolbar (BonoboUIHandler *uih, const char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
	g_return_if_fail (name[0] != '/');

	toolbar_local_toolbar_create (uih, name);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		toolbar_remote_toolbar_create (uih, name);
		return;
	}

	toolbar_toplevel_toolbar_create (uih, name, bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
}

static void
toolbar_toplevel_toolbar_remove (BonoboUIHandler *uih, const char *name)
{
	g_warning ("toolbar remove unimplemented");
}

static void
toolbar_remote_toolbar_remove (BonoboUIHandler *uih, const char *name)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_remove (uih->top_level_uih,
					bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					name, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_remove (PortableServer_Servant servant,
		     Bonobo_UIHandler        containee,
		     const CORBA_char      *name,
		     CORBA_Environment     *ev)
{
/*	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));*/

	g_warning ("toolbar remove unimplemented");
}

/**
 * bonobo_ui_handler_remove_toolbar:
 */
void
bonobo_ui_handler_remove_toolbar (BonoboUIHandler *uih, const char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_remote_toolbar_remove (uih, name);
	else
		toolbar_toplevel_toolbar_remove (uih, name);
}

void
bonobo_ui_handler_set_toolbar (BonoboUIHandler *uih, const char *name,
			      GtkWidget *toolbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
}

/**
 * bonobo_ui_handler_get_toolbar_list:
 */
GList *
bonobo_ui_handler_get_toolbar_list (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	return NULL;
}

static void
toolbar_local_do_path (const char *parent_path, BonoboUIHandlerToolbarItem *item)
{
	uih_local_do_path (parent_path, item->label, & item->path);
}

static void
toolbar_local_remove_parent_entry (BonoboUIHandler *uih, const char *path,
				   gboolean warn)
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
toolbar_local_add_parent_entry (BonoboUIHandler *uih, const char *path)
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
toolbar_local_create_item (BonoboUIHandler *uih, const char *parent_path,
			   BonoboUIHandlerToolbarItem *item)
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
impl_toolbar_overridden (PortableServer_Servant servant,
			 const CORBA_char      *path,
			 CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemLocalInternal *internal_cb;
	char *parent_path;

	parent_path = path_get_parent (path);

	internal_cb = toolbar_local_get_item (uih, path);

	if (internal_cb == NULL && strcmp (parent_path, "/")) {
		g_warning ("Received override notification for a toolbar item I don't own [%s]!\n", path);
		return;
	}

	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [TOOLBAR_ITEM_OVERRIDDEN],
			 path, internal_cb ? internal_cb->callback_data : NULL);
}

static void
toolbar_toplevel_item_check_override (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);

	if (internal == NULL)
		return;

	toolbar_toplevel_item_remove_widgets_recursive (uih, path);

	toolbar_toplevel_item_override_notify_recursive (uih, path);
}

static gint
toolbar_toplevel_item_activated (GtkWidget *widget, ToolbarItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_activated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (internal->uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}
	
	CORBA_exception_free (&ev);

	return FALSE;
}

static void
impl_toolbar_activated (PortableServer_Servant servant,
			const CORBA_char      *path,
			CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemLocalInternal *internal_cb;

	internal_cb = toolbar_local_get_item (uih, path);

	if (internal_cb == NULL) {
		g_warning ("Received activation notification about a toolbar item I don't own [%s]!\n", path);
		return;
	}

	if (internal_cb->callback != NULL)
		internal_cb->callback (uih, internal_cb->callback_data, path);
	
	gtk_signal_emit (GTK_OBJECT (uih), uih_signals [MENU_ITEM_ACTIVATED], path, internal_cb->callback_data);
}

static GtkToolbarChildType
toolbar_type_to_gtk_type (BonoboUIHandlerToolbarItemType type)
{
	switch (type) {
	case BONOBO_UI_HANDLER_TOOLBAR_ITEM:
		return GTK_TOOLBAR_CHILD_BUTTON;

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM:
		return GTK_TOOLBAR_CHILD_RADIOBUTTON;

	case BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR:
		return GTK_TOOLBAR_CHILD_SPACE;

	case BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM:
		return GTK_TOOLBAR_CHILD_TOGGLEBUTTON;

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP:
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
toolbar_toplevel_item_create_widgets (BonoboUIHandler *uih,
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

	case BONOBO_UI_HANDLER_TOOLBAR_CONTROL:
		toolbar_item = bonobo_widget_new_control_from_objref (internal->item->control);

		gtk_widget_show (toolbar_item);

		if (internal->item->pos > 0)
			gtk_toolbar_insert_widget (GTK_TOOLBAR (toolbar),
						   toolbar_item,
						   internal->item->hint, NULL,
						   internal->item->pos);
		else
			gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar),
						   toolbar_item,
						   internal->item->hint, NULL);

		break;

	case BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR:
		gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), internal->item->pos);
		break;

	case BONOBO_UI_HANDLER_TOOLBAR_ITEM:
		pixmap = NULL;
		if (internal->item->pixmap_data != NULL && internal->item->pixmap_type != BONOBO_UI_HANDLER_PIXMAP_NONE)
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

	case BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM:

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

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM:
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
toolbar_toplevel_item_remove_parent_entry (BonoboUIHandler *uih, const char *path,
					   gboolean warn)
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
toolbar_toplevel_item_add_parent_entry (BonoboUIHandler *uih, const char *path)
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
toolbar_toplevel_item_store_data (BonoboUIHandler *uih, Bonobo_UIHandler uih_corba,
				  BonoboUIHandlerToolbarItem *item)
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
toolbar_toplevel_item_set_sensitivity_internal (BonoboUIHandler *uih, ToolbarItemInternal *internal,
						gboolean sensitive)
{
	GtkWidget *toolbar_widget;

	internal->sensitive = sensitive;

	if (! toolbar_toplevel_item_is_head (uih, internal))
		return;

	toolbar_widget = toolbar_toplevel_item_get_widget (uih, internal->item->path);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_widget_set_sensitive (toolbar_widget, sensitive);
}

static void
toolbar_toplevel_set_sensitivity_internal (BonoboUIHandler *uih, ToolbarToolbarInternal *internal,
					   gboolean sensitive)
{
	GtkWidget *toolbar_widget;

	toolbar_widget = toolbar_toplevel_get_widget (uih, internal->name);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_widget_set_sensitive (toolbar_widget, sensitive);
}

static void
toolbar_toplevel_set_space_size_internal (BonoboUIHandler *uih, ToolbarToolbarInternal *internal,
					  gint size)
{
	GtkWidget *toolbar_widget;

	toolbar_widget = toolbar_toplevel_get_widget (uih, internal->name);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar_widget), size);
}

static void
toolbar_toplevel_set_style_internal (BonoboUIHandler *uih, ToolbarToolbarInternal *internal,
				     GtkToolbarStyle style)
{
	GtkWidget *toolbar_widget;

	toolbar_widget = toolbar_toplevel_get_widget (uih, internal->name);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar_widget), style);
}

static void
toolbar_toplevel_set_orientation (BonoboUIHandler *uih, ToolbarToolbarInternal *internal,
				  GtkOrientation orientation)
{
	GtkWidget *toolbar_widget;

	toolbar_widget = toolbar_toplevel_get_widget (uih, internal->name);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar_widget), orientation);
}

static void
toolbar_toplevel_set_button_relief (BonoboUIHandler *uih, ToolbarToolbarInternal *internal,
				    GtkReliefStyle relief_style)
{
	GtkWidget *toolbar_widget;

	toolbar_widget = toolbar_toplevel_get_widget (uih, internal->name);
	g_return_if_fail (toolbar_widget != NULL);

	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar_widget), relief_style);
}

static void
toolbar_toplevel_item_set_toggle_state_internal (BonoboUIHandler *uih, ToolbarItemInternal *internal,
						 gboolean active)
{
}

static void
toolbar_toplevel_item_set_radio_state_internal (BonoboUIHandler *uih, ToolbarItemInternal *internal,
						gboolean active)
{
}

static void
toolbar_toplevel_create_item (BonoboUIHandler *uih, const char *parent_path,
			      BonoboUIHandlerToolbarItem *item,
			      Bonobo_UIHandler uih_corba)
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

	/*
	 * Set its sensitivity.
	 */
	toolbar_toplevel_item_set_sensitivity_internal (uih, internal, internal->sensitive);

	/*
	 * Set its active state.
	 */
	if (internal->item->type == BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM)
		toolbar_toplevel_item_set_toggle_state_internal (uih, internal, internal->active);
	else if (internal->item->type == BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM)
		toolbar_toplevel_item_set_radio_state_internal (uih, internal, internal->active);
}

static void
toolbar_remote_create_item (BonoboUIHandler *uih, const char *parent_path,
			    BonoboUIHandlerToolbarItem *item)
{
	Bonobo_UIHandler_iobuf *pixmap_buf;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	pixmap_buf = pixmap_data_to_corba (item->pixmap_type, item->pixmap_data);

	Bonobo_UIHandler_toolbar_create_item (uih->top_level_uih,
					     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					     item->path,
					     toolbar_type_to_corba (item->type),
					     CORBIFY_STRING (item->label),
					     CORBIFY_STRING (item->hint),
					     item->pos,
					     item->control,
					     pixmap_type_to_corba (item->pixmap_type),
					     pixmap_buf,
					     (CORBA_unsigned_long) item->accelerator_key,
					     (CORBA_long) item->ac_mods,
					     &ev);

	bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) uih->top_level_uih, &ev);

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buf);
}

static void
impl_toolbar_create_item (PortableServer_Servant servant,
			  const Bonobo_UIHandler  containee_uih,
			  const CORBA_char      *path,
			  Bonobo_UIHandler_ToolbarType toolbar_type,
			  const CORBA_char      *label,
			  const CORBA_char      *hint,
			  CORBA_long             pos,
			  const Bonobo_Control    control,
			  Bonobo_UIHandler_PixmapType   pixmap_type,
			  const Bonobo_UIHandler_iobuf *pixmap_data,
			  CORBA_unsigned_long          accelerator_key,
			  CORBA_long                   modifier,
			  CORBA_Environment           *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	BonoboUIHandlerToolbarItem *item;
	char *parent_path;


	item = toolbar_make_item (path, toolbar_corba_to_type (toolbar_type),
				  UNCORBIFY_STRING (label),
				  UNCORBIFY_STRING (hint),
				  pos,
				  control,
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
 * bonobo_ui_handler_toolbar_add_one:
 */
void
bonobo_ui_handler_toolbar_add_one (BonoboUIHandler *uih, const char *parent_path,
				  BonoboUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
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
					       bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
}

void
bonobo_ui_handler_toolbar_add_list (BonoboUIHandler *uih, const char *parent_path,
				   BonoboUIHandlerToolbarItem *item)
{
	BonoboUIHandlerToolbarItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; curr->type != BONOBO_UI_HANDLER_TOOLBAR_END; curr ++)
		bonobo_ui_handler_toolbar_add_tree (uih, parent_path, curr);
}

void
bonobo_ui_handler_toolbar_add_tree (BonoboUIHandler *uih, const char *parent_path,
				   BonoboUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * Add this toolbar item.
	 */
	bonobo_ui_handler_toolbar_add_one (uih, parent_path, item);

	/*
	 * Recursive add its children.
	 */
	if (item->children != NULL)
		bonobo_ui_handler_toolbar_add_list (uih, item->path, item->children);
}

void
bonobo_ui_handler_toolbar_new (BonoboUIHandler *uih, const char *path,
			      BonoboUIHandlerToolbarItemType type,
			      const char *label, const char *hint,
			      int pos, const Bonobo_Control control,
			      BonoboUIHandlerPixmapType pixmap_type,
			      gconstpointer pixmap_data, guint accelerator_key,
			      GdkModifierType ac_mods,
			      BonoboUIHandlerCallbackFunc callback,
			      gpointer callback_data)
{
	BonoboUIHandlerToolbarItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	item = toolbar_make_item (path, type, label, hint, pos, control, pixmap_type,
				  pixmap_data, accelerator_key, ac_mods, callback, callback_data);

	parent_path = path_get_parent (path);
	g_return_if_fail (parent_path != NULL);

	bonobo_ui_handler_toolbar_add_one (uih, parent_path, item);

	g_free (item);
	g_free (parent_path);
}

/**
 * bonobo_ui_handler_toolbar_new_item
 */
void
bonobo_ui_handler_toolbar_new_item (BonoboUIHandler *uih, const char *path,
				   const char *label, const char *hint, int pos,
				   BonoboUIHandlerPixmapType pixmap_type,
				   gconstpointer pixmap_data,
				   guint accelerator_key, GdkModifierType ac_mods,
				   BonoboUIHandlerCallbackFunc callback,
				   gpointer callback_data)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_ITEM,
				      label, hint, pos, CORBA_OBJECT_NIL, pixmap_type,
				      pixmap_data, accelerator_key,
				      ac_mods, callback, callback_data);
}

/**
 * bonobo_ui_handler_toolbar_new_control:
 */
void
bonobo_ui_handler_toolbar_new_control (BonoboUIHandler *uih, const char *path,
				      int pos, Bonobo_Control control)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_CONTROL,
				      NULL, NULL, pos, control, BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}


/**
 * bonobo_ui_handler_toolbar_new_separator:
 */
void
bonobo_ui_handler_toolbar_new_separator (BonoboUIHandler *uih, const char *path,
					int pos)
{
	bonobo_ui_handler_toolbar_new (uih, path,
				      BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR,
				      NULL, NULL, pos, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_toolbar_new_radiogroup:
 */
void
bonobo_ui_handler_toolbar_new_radiogroup (BonoboUIHandler *uih, const char *path)
{
	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP,
				      NULL, NULL, -1, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, 0, 0, NULL, NULL);
}

/**
 * bonobo_ui_handler_toolbar_new_radioitem:
 */
void
bonobo_ui_handler_toolbar_new_radioitem (BonoboUIHandler *uih, const char *path,
					const char *label, const char *hint, int pos,
					guint accelerator_key, GdkModifierType ac_mods,
					BonoboUIHandlerCallbackFunc callback,
					gpointer callback_data)
{
	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM,
				      label, hint, pos, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, accelerator_key, ac_mods, callback, callback_data);
}

/**
 * bonobo_ui_handler_toolbar_new_toggleitem:
 */
void
bonobo_ui_handler_toolbar_new_toggleitem (BonoboUIHandler *uih, const char *path,
					 const char *label, const char *hint, int pos,
					 guint accelerator_key, GdkModifierType ac_mods,
					 BonoboUIHandlerCallbackFunc callback,
					 gpointer callback_data)
{
	bonobo_ui_handler_toolbar_new (uih, path, BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM,
				      label, hint, pos, CORBA_OBJECT_NIL,
				      BONOBO_UI_HANDLER_PIXMAP_NONE,
				      NULL, accelerator_key, ac_mods, callback,
				      callback_data);
}

static void
toolbar_local_remove_item (BonoboUIHandler *uih, const char *path)
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
		g_hash_table_insert (uih->path_to_toolbar_callback, g_strdup (path), new_list);
	
}

static void
toolbar_toplevel_item_reinstate_notify (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;
	CORBA_Environment ev;

	internal = toolbar_toplevel_get_item (uih, path);

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_reinstated (internal->uih_corba, internal->item->path, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) internal->uih_corba, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_reinstated (PortableServer_Servant servant,
			 const CORBA_char *path,
			 CORBA_Environment *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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
toolbar_toplevel_item_remove_notify (BonoboUIHandler *uih, ToolbarItemInternal *internal)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_removed (internal->uih_corba, internal->item->path, &ev);

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_removed (PortableServer_Servant servant,
		      const CORBA_char *path,
		      CORBA_Environment *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
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
toolbar_toplevel_item_remove_data (BonoboUIHandler *uih, ToolbarItemInternal *internal)
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

	/*
	 * FIXME
	 *
	 * This g_slist_free() (or maybe the other one in this file)
	 * seems to corrupt the SList allocator's free list
	 */
/*	g_slist_free (internal->radio_items); */
	g_free (internal);
	g_free (path);
}

static void
toolbar_toplevel_remove_item_internal (BonoboUIHandler *uih, ToolbarItemInternal *internal)
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
toolbar_toplevel_remove_item (BonoboUIHandler *uih, const char *path, Bonobo_UIHandler uih_corba)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item_for_containee (uih, path, uih_corba);
	g_return_if_fail (internal != NULL);

	toolbar_toplevel_remove_item_internal (uih, internal);
}

static void
toolbar_remote_remove_item (BonoboUIHandler *uih, const char *path)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_remove_item (uih->top_level_uih,
					     bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					     path,
					     &ev);

	CORBA_exception_free (&ev);
}

static void
impl_toolbar_remove_item (PortableServer_Servant servant,
			  Bonobo_UIHandler containee_uih,
			  const CORBA_char *path,
			  CORBA_Environment *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	
	g_return_if_fail (uih_toplevel_check_toplevel (uih));

	toolbar_toplevel_remove_item (uih, path, containee_uih);
}

/**
 * bonobo_ui_handler_toolbar_remove:
 */
void
bonobo_ui_handler_toolbar_remove (BonoboUIHandler *uih, const char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_remote_remove_item (uih, path);
	else
		toolbar_toplevel_remove_item (uih, path, bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
}

BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_fetch_one (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

GList *
bonobo_ui_handler_toolbar_fetch_by_callback (BonoboUIHandler *uih,
					    BonoboUIHandlerCallbackFunc callback)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

GList *
bonobo_ui_handler_toolbar_fetch_by_callback_data (BonoboUIHandler *uih,
						 gpointer callback_data)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

static BonoboUIHandlerMenuItemType
toolbar_uiinfo_type_to_uih (GnomeUIInfoType uii_type)
{
	switch (uii_type) {
	case GNOME_APP_UI_ENDOFINFO:
		return BONOBO_UI_HANDLER_TOOLBAR_END;

	case GNOME_APP_UI_ITEM:
		return BONOBO_UI_HANDLER_TOOLBAR_ITEM;

	case GNOME_APP_UI_TOGGLEITEM:
		return BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM;

	case GNOME_APP_UI_RADIOITEMS:
		return BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP;

	case GNOME_APP_UI_SUBTREE:
		g_error ("Toolbar subtree doesn't make sense!\n");

	case GNOME_APP_UI_SEPARATOR:
		return BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR;

	case GNOME_APP_UI_HELP:
		g_error ("Help unimplemented.\n"); /* FIXME */

	case GNOME_APP_UI_BUILDER_DATA:
		g_error ("Builder data - what to do?\n"); /* FIXME */

	case GNOME_APP_UI_ITEM_CONFIGURABLE:
		g_warning ("Configurable item!");
		return BONOBO_UI_HANDLER_MENU_ITEM;

	case GNOME_APP_UI_SUBTREE_STOCK:
		g_error ("Toolbar subtree doesn't make sense!\n");

	default:
		g_warning ("Unknown UIInfo Type: %d", uii_type);
		return BONOBO_UI_HANDLER_TOOLBAR_ITEM;
	}
}

static void
toolbar_parse_uiinfo_one (BonoboUIHandlerToolbarItem *item, GnomeUIInfo *uii)
{
	item->path = NULL;

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	item->type = toolbar_uiinfo_type_to_uih (uii->type);

	item->label = g_strdup (L_(uii->label));
	item->hint = g_strdup (L_(uii->hint));

	item->pos = -1;

	if (item->type == BONOBO_UI_HANDLER_TOOLBAR_ITEM ||
	    item->type == BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM ||
	    item->type == BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM)
		item->callback = uii->moreinfo;
	item->callback_data = uii->user_data;

	item->pixmap_type = uiinfo_pixmap_type_to_uih (uii->pixmap_type);
	item->pixmap_data = pixmap_copy_data (item->pixmap_type, uii->pixmap_info);
	item->accelerator_key = uii->accelerator_key;
	item->ac_mods = uii->ac_mods;
}

static void
toolbar_parse_uiinfo_tree (BonoboUIHandlerToolbarItem *tree, GnomeUIInfo *uii)
{
	toolbar_parse_uiinfo_one (tree, uii);

	if (tree->type == BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP) {
		tree->children = bonobo_ui_handler_toolbar_parse_uiinfo_list (uii->moreinfo);
	}
}

static void
toolbar_parse_uiinfo_one_with_data (BonoboUIHandlerToolbarItem *item, GnomeUIInfo *uii, void *data)
{
	toolbar_parse_uiinfo_one (item, uii);
	item->callback_data = data;
}

static void
toolbar_parse_uiinfo_tree_with_data (BonoboUIHandlerToolbarItem *tree, GnomeUIInfo *uii, void *data)
{
	toolbar_parse_uiinfo_one_with_data (tree, uii, data);

	if (tree->type == BONOBO_UI_HANDLER_TOOLBAR_RADIOGROUP) {
		tree->children = bonobo_ui_handler_toolbar_parse_uiinfo_list_with_data (uii->moreinfo, data);
	}
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_one:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_one (GnomeUIInfo *uii)
{
	BonoboUIHandlerToolbarItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (BonoboUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_one (item, uii);

	return item;
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_list:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_list (GnomeUIInfo *uii)
{
	BonoboUIHandlerToolbarItem *list;
	BonoboUIHandlerToolbarItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the BonoboUIHandlerToolbarItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (BonoboUIHandlerToolbarItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		toolbar_parse_uiinfo_tree (curr_uih, curr_uii);

	/* Parse the terminal entry. */
	toolbar_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_tree:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_tree (GnomeUIInfo *uii)
{
	BonoboUIHandlerToolbarItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (BonoboUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_tree (item_tree, uii);

	return item_tree;
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_one_with_data:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_one_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerToolbarItem *item;

	g_return_val_if_fail (uii != NULL, NULL);

	item = g_new0 (BonoboUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_one_with_data (item, uii, data);

	return item;
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_list_with_data:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_list_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerToolbarItem *list;
	BonoboUIHandlerToolbarItem *curr_uih;
	GnomeUIInfo *curr_uii;
	int list_len;

	g_return_val_if_fail (uii != NULL, NULL);

	/*
	 * Allocate the BonoboUIHandlerToolbarItem array.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (BonoboUIHandlerToolbarItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++)
		toolbar_parse_uiinfo_tree_with_data (curr_uih, curr_uii, data);

	/* Parse the terminal entry. */
	toolbar_parse_uiinfo_one (curr_uih, curr_uii);

	return list;
}

/**
 * bonobo_ui_handler_toolbar_parse_uiinfo_tree_with_data:
 */
BonoboUIHandlerToolbarItem *
bonobo_ui_handler_toolbar_parse_uiinfo_tree_with_data (GnomeUIInfo *uii, void *data)
{
	BonoboUIHandlerToolbarItem *item_tree;

	g_return_val_if_fail (uii != NULL, NULL);

	item_tree = g_new0 (BonoboUIHandlerToolbarItem, 1);

	toolbar_parse_uiinfo_tree_with_data (item_tree, uii, data);

	return item_tree;
}

void
bonobo_ui_handler_toolbar_free_one (BonoboUIHandlerToolbarItem *item)
{
	g_return_if_fail (item != NULL);

	toolbar_free (item);
}

void
bonobo_ui_handler_toolbar_free_list (BonoboUIHandlerToolbarItem *array)
{
	BonoboUIHandlerToolbarItem *curr;

	g_return_if_fail (array != NULL);

	for (curr = array; curr->type != BONOBO_UI_HANDLER_TOOLBAR_END; curr ++) {
		if (curr->children != NULL)
			bonobo_ui_handler_toolbar_free_list (curr->children);

		toolbar_free_data (curr);
	}

	g_free (array);
}

void
bonobo_ui_handler_toolbar_free_tree (BonoboUIHandlerToolbarItem *tree)
{
	g_return_if_fail (tree != NULL);

	if (tree->type == BONOBO_UI_HANDLER_TOOLBAR_END)
		return;

	if (tree->children != NULL)
		bonobo_ui_handler_toolbar_free_list (tree->children);

	toolbar_free (tree);
}

static gint
toolbar_toplevel_item_get_pos (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);

	g_return_val_if_fail (internal != NULL, -1);

	return internal->item->pos;
}

static gint
toolbar_item_remote_get_pos (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gint ans;

	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return -1;

	ans = (gint)attrs->pos;

	ui_remote_attribute_data_free (attrs);

	return ans;
}

int
bonobo_ui_handler_toolbar_item_get_pos (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return toolbar_item_remote_get_pos (uih, path);

	return toolbar_toplevel_item_get_pos (uih, path);
}

static void
toolbar_toplevel_set_sensitivity (BonoboUIHandler *uih, const char *name,
				  gboolean sensitive)
{
	ToolbarToolbarInternal *internal;

	internal = toolbar_toplevel_get_toolbar (uih, name);
	g_return_if_fail (internal != NULL);

	toolbar_toplevel_set_sensitivity_internal (uih, internal, sensitive);
}

static gboolean
toolbar_item_remote_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	UIRemoteAttributeData *attrs;
	gboolean                    ans;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return TRUE;
	ans = (gboolean) attrs->sensitive;
	ui_remote_attribute_data_free (attrs);

	return ans;
}

static void
toolbar_item_remote_set_sensitivity (BonoboUIHandler *uih, const char *path,
				     gboolean sensitive)
{
	UIRemoteAttributeData *attrs;
	
	attrs = toolbar_item_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;
	attrs->sensitive = sensitive;
	toolbar_item_remote_attribute_data_set (uih, path, attrs);
}

void
bonobo_ui_handler_toolbar_item_set_sensitivity (BonoboUIHandler *uih, const char *path,
					       gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		toolbar_item_remote_set_sensitivity (uih, path, sensitive);
	else
		toolbar_toplevel_set_sensitivity (uih, path, sensitive);
}

static gboolean
toolbar_toplevel_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);
	g_return_val_if_fail (internal != NULL, FALSE);

	return internal->sensitive;
}

static void
impl_toolbar_item_get_attributes (PortableServer_Servant servant,
				  const Bonobo_UIHandler  containee,
				  const CORBA_char      *path,
				  CORBA_boolean         *sensitive,
				  CORBA_boolean         *active,
				  CORBA_long            *pos,
				  CORBA_char           **label,
				  CORBA_char           **hint,
				  CORBA_long            *accelerator_key,
				  CORBA_long            *ac_mods,
				  CORBA_boolean         *toggle_state,
				  CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item (uih, path);

	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}

	g_warning ("toolbar_get_attributes substantially unimplemented");

	*pos       = toolbar_toplevel_item_get_pos (uih, (char *) path);
	*sensitive = toolbar_toplevel_get_sensitivity (uih, path);
}

static void
impl_toolbar_item_set_attributes (PortableServer_Servant servant,
				  const Bonobo_UIHandler  containee,
				  const CORBA_char      *path,
				  CORBA_boolean          sensitive,
				  CORBA_boolean          active,
				  CORBA_long             pos,
				  const CORBA_char      *label,
				  const CORBA_char      *hint,
				  CORBA_long             accelerator_key,
				  CORBA_long             ac_mods,
				  CORBA_boolean          toggle_state,
				  CORBA_Environment     *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item_for_containee (uih, path, containee);
	g_return_if_fail (internal != NULL);

	toolbar_toplevel_item_set_sensitivity_internal (uih, internal, sensitive);

	g_warning ("toolbar_set_attributes substantially unimplemented");
}


/*
 * Ugly hack !, gtk_toolbar is insufficiently flexible for this by a long way !
 * In fact, this makes me want to puke.
 */
static void
toolbar_toplevel_item_set_pixmap_internal (BonoboUIHandler *uih, ToolbarItemInternal *internal,
					   BonoboUIHandlerPixmapType type, gpointer data)
{
	GtkToolbar *toolbar;
	GList      *l;
	int         i;

	/*
	 * Update the widgets.
	 */
	if (! toolbar_toplevel_item_is_head (uih, internal))
		return;

	toolbar = GTK_TOOLBAR (toolbar_toplevel_item_get_widget (uih, internal->item->path));

	i = 0;
	for (l = toolbar->children; l; l = l->next) {
		GtkToolbarChild *kid = l->data;

		if (kid->label && !strcmp (GTK_LABEL (kid->label)->label, internal->item->label))
			break;
		i++;
	}
	if (l) {
		GtkToolbarChild *kid = l->data;

		/* Ugly remove code */
		gtk_widget_destroy (kid->widget);
		toolbar->children = g_list_remove (toolbar->children, kid);

		/*
		 * Update the internal data for this toolbar item.
		 */
		pixmap_free_data (internal->item->pixmap_type, internal->item->pixmap_data);
		
		internal->item->pixmap_type = type;
		internal->item->pixmap_data = pixmap_copy_data (type, data);

		/* Now re-create it ... */
		internal->item->pos = i;
		toolbar_toplevel_item_create_widgets (uih, internal);
	} else
		g_warning ("Can't find toolbar to update pixmap");
}

static void
toolbar_toplevel_item_set_pixmap (BonoboUIHandler *uih, const char *path,
				  BonoboUIHandlerPixmapType type, gpointer data)
{
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_item_for_containee (uih, path,
							    bonobo_object_corba_objref (BONOBO_OBJECT (uih)));
	g_return_if_fail (internal != NULL);

	toolbar_toplevel_item_set_pixmap_internal (uih, internal, type, data);
}

static void
toolbar_item_remote_set_pixmap (BonoboUIHandler *uih, const char *path,
				BonoboUIHandlerPixmapType type, gpointer data)
{
	Bonobo_UIHandler_iobuf *pixmap_buff;
	CORBA_Environment ev;

	pixmap_buff = pixmap_data_to_corba (type, data);
	
	CORBA_exception_init (&ev);

	Bonobo_UIHandler_toolbar_item_set_data (uih->top_level_uih,
					       bonobo_object_corba_objref (BONOBO_OBJECT (uih)),
					       path,
				       pixmap_type_to_corba (type),
				       pixmap_buff,
				       &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		bonobo_object_check_env (
			BONOBO_OBJECT (uih),
			(CORBA_Object) uih->top_level_uih, &ev);
	}

	CORBA_exception_free (&ev);

	CORBA_free (pixmap_buff);
}

void
bonobo_ui_handler_toolbar_item_set_pixmap (BonoboUIHandler *uih, const char *path,
					  BonoboUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (data != NULL);

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		toolbar_item_remote_set_pixmap (uih, path, type, data);
		return;
	}

	toolbar_toplevel_item_set_pixmap (uih, path, type, data);
}

void
bonobo_ui_handler_toolbar_item_get_pixmap (BonoboUIHandler *uih, const char *path,
					  BonoboUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

static void
impl_toolbar_item_set_data (PortableServer_Servant       servant,
			    Bonobo_UIHandler              containee_uih,
			    const CORBA_char            *path,
			    Bonobo_UIHandler_PixmapType   corba_pixmap_type,
			    const Bonobo_UIHandler_iobuf *corba_pixmap_data,
			    CORBA_Environment           *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemInternal *internal;
	
	g_return_if_fail (uih_toplevel_check_toplevel (uih));
	
	internal = toolbar_toplevel_get_item_for_containee (uih, path, containee_uih);
	
	if (internal == NULL) {
		/* FIXME: Set exception. */
		return;
	}
	
	toolbar_toplevel_item_set_pixmap_internal (uih, internal,
						   pixmap_corba_to_type (corba_pixmap_type),
						   pixmap_corba_to_data (corba_pixmap_type,
									 corba_pixmap_data));
}

static void
impl_toolbar_item_get_data (PortableServer_Servant      servant,
			    Bonobo_UIHandler              containee_uih,
			    const CORBA_char           *path,
			    Bonobo_UIHandler_PixmapType *corba_pixmap_type,
			    Bonobo_UIHandler_iobuf     **corba_pixmap_data,
			    CORBA_Environment          *ev)
{
	/* FIXME: Implement me! */
	g_warning ("Unimplemented: remote get pixmap");
}


inline static GtkOrientation
toolbar_corba_to_orientation (Bonobo_UIHandler_ToolbarOrientation o)
{
	switch (o) {
	case Bonobo_UIHandler_ToolbarOrientationHorizontal:
		return GTK_ORIENTATION_HORIZONTAL;
	case Bonobo_UIHandler_ToolbarOrientationVertical:
	default:
	        return GTK_ORIENTATION_VERTICAL;
	}
}

inline static Bonobo_UIHandler_ToolbarOrientation
toolbar_orientation_to_corba (GtkOrientation orientation)
{
	switch (orientation) {
	case GTK_ORIENTATION_VERTICAL:
		return Bonobo_UIHandler_ToolbarOrientationVertical;
	case GTK_ORIENTATION_HORIZONTAL:
	default:
		return Bonobo_UIHandler_ToolbarOrientationHorizontal;
	}
}

inline static GtkToolbarStyle
toolbar_corba_to_style (Bonobo_UIHandler_ToolbarStyle s)
{
	switch (s) {
	case Bonobo_UIHandler_ToolbarStyleIcons:
		return GTK_TOOLBAR_ICONS;
	case Bonobo_UIHandler_ToolbarStyleText:
		return GTK_TOOLBAR_TEXT;
	case Bonobo_UIHandler_ToolbarStyleBoth:
	default:
		return GTK_TOOLBAR_BOTH;
	}
}

inline static Bonobo_UIHandler_ToolbarStyle
toolbar_style_to_corba (GtkToolbarStyle s)
{
	switch (s) {
	case GTK_TOOLBAR_ICONS:
		return Bonobo_UIHandler_ToolbarStyleIcons;
	case GTK_TOOLBAR_TEXT:
		return Bonobo_UIHandler_ToolbarStyleText;
	case GTK_TOOLBAR_BOTH:
	default:
		return Bonobo_UIHandler_ToolbarStyleBoth;
	}
}

inline static GtkReliefStyle
relief_corba_to_style (Bonobo_UIHandler_ReliefStyle s)
{
	switch (s) {
	case Bonobo_UIHandler_ReliefHalf:
		return GTK_RELIEF_HALF;
	case Bonobo_UIHandler_ReliefNone:
		return GTK_RELIEF_NONE;
	case Bonobo_UIHandler_ReliefNormal:
	default:
		return GTK_RELIEF_NORMAL;
	}
}

inline static Bonobo_UIHandler_ReliefStyle
relief_style_to_corba (GtkReliefStyle s)
{
	switch (s) {
	case GTK_RELIEF_HALF:
		return Bonobo_UIHandler_ReliefHalf;
	case GTK_RELIEF_NONE:
		return Bonobo_UIHandler_ReliefNone;
	case GTK_RELIEF_NORMAL:
	default:
		return Bonobo_UIHandler_ReliefNormal;
	}
}


static void
impl_toolbar_set_attributes (PortableServer_Servant                   servant,
			     const Bonobo_UIHandler                    containee,
			     const CORBA_char                        *name,
			     const Bonobo_UIHandler_ToolbarOrientation orientation,
			     const Bonobo_UIHandler_ToolbarStyle       style,
			     const Bonobo_UIHandler_ToolbarSpaceStyle  space_style,
			     const Bonobo_UIHandler_ReliefStyle        relief_style,
			     const CORBA_long                         space_size,
			     const CORBA_boolean                      sensitive,
			     CORBA_Environment                       *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarToolbarInternal *internal;

	internal = toolbar_toplevel_get_toolbar (uih, name);
	g_return_if_fail (internal != NULL);

	toolbar_toplevel_set_orientation          (uih, internal, 
						   toolbar_corba_to_orientation (orientation));
	toolbar_toplevel_set_style_internal       (uih, internal,
						   toolbar_corba_to_style (style));
	toolbar_toplevel_set_space_size_internal  (uih, internal, space_size);
	toolbar_toplevel_set_sensitivity_internal (uih, internal, sensitive);
	toolbar_toplevel_set_button_relief        (uih, internal,
						   relief_corba_to_style (relief_style));
}

static void
impl_toolbar_get_attributes (PortableServer_Servant              servant,
			     const Bonobo_UIHandler               containee,
			     const CORBA_char                   *name,
			     Bonobo_UIHandler_ToolbarOrientation *orientation,
			     Bonobo_UIHandler_ToolbarStyle       *style,
			     Bonobo_UIHandler_ToolbarSpaceStyle  *space_style,
			     Bonobo_UIHandler_ReliefStyle        *relief_style,
			     CORBA_long                         *space_size,
			     CORBA_boolean                      *sensitive,
			     CORBA_Environment                  *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));
	ToolbarItemInternal *internal;

	internal = toolbar_toplevel_get_toolbar (uih, name);
	g_return_if_fail (internal != NULL);

	g_warning ("Unimplemented");
}

void
bonobo_ui_handler_toolbar_set_orientation (BonoboUIHandler *uih, const char *path,
					  GtkOrientation orientation)
{
	ToolbarRemoteAttributeData *attrs;
	
	attrs = toolbar_remote_attribute_data_get (uih, path);
	if (!attrs)
		return;

	attrs->orientation = toolbar_orientation_to_corba (orientation);
	toolbar_remote_attribute_data_set (uih, path, attrs);
}

GtkOrientation
bonobo_ui_handler_toolbar_get_orientation (BonoboUIHandler *uih, const char *path)
{
	ToolbarRemoteAttributeData *attrs;
	GtkOrientation              ans;
	
	attrs = toolbar_remote_attribute_data_get (uih, path);
	if (!attrs)
		return GTK_ORIENTATION_HORIZONTAL;

	ans = toolbar_orientation_to_corba (attrs->orientation);

	toolbar_remote_attribute_data_free (attrs);

	return ans;
}

gboolean
bonobo_ui_handler_toolbar_item_get_sensitivity (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	if (uih->top_level_uih != CORBA_OBJECT_NIL)
		return toolbar_item_remote_get_sensitivity (uih, path);

	return toolbar_toplevel_get_sensitivity (uih, path);
}

void
bonobo_ui_handler_toolbar_item_set_label (BonoboUIHandler *uih, const char *path,
					 gchar *label)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		toolbar_item_remote_set_label (uih, path, label);
		return;
	}

/*	toolbar_toplevel_item_set_label (uih, path, label_text);*/
}

gchar *
bonobo_ui_handler_toolbar_item_get_label (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	g_warning ("Unimplemented toolbar method");
	return NULL;
}

void
bonobo_ui_handler_toolbar_item_get_accel (BonoboUIHandler *uih, const char *path,
					 guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
bonobo_ui_handler_toolbar_item_set_callback (BonoboUIHandler *uih, const char *path,
					    BonoboUIHandlerCallbackFunc callback,
					    gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

void
bonobo_ui_handler_toolbar_item_get_callback (BonoboUIHandler *uih, const char *path,
					    BonoboUIHandlerCallbackFunc *callback,
					    gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gboolean
bonobo_ui_handler_toolbar_item_toggle_get_state (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL,FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_warning ("Unimplemented toolbar method");
	return FALSE;
}

void
bonobo_ui_handler_toolbar_item_toggle_set_state (BonoboUIHandler *uih, const char *path,
						gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

gboolean
bonobo_ui_handler_toolbar_item_radio_get_state (BonoboUIHandler *uih, const char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	g_warning ("Unimplemented toolbar method");
	return FALSE;
}

void
bonobo_ui_handler_toolbar_item_radio_set_state (BonoboUIHandler *uih, const char *path,
					       gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_warning ("Unimplemented toolbar method");
}

/**
 * bonobo_ui_handler_get_epv:
 */
POA_Bonobo_UIHandler__epv *
bonobo_ui_handler_get_epv (void)
{
	POA_Bonobo_UIHandler__epv *epv;

	epv = g_new0 (POA_Bonobo_UIHandler__epv, 1);

	/* General server management. */
	epv->register_containee = impl_register_containee;
	epv->unregister_containee = impl_unregister_containee;
	epv->get_toplevel = impl_get_toplevel;

	/* Menu management. */
	epv->menu_create = impl_menu_create;
	epv->menu_remove = impl_menu_remove;
	epv->menu_fetch  = impl_menu_fetch;
	epv->menu_get_children = impl_menu_get_children;
	epv->menu_set_attributes = impl_menu_set_attributes;
	epv->menu_get_attributes = impl_menu_get_attributes;
	epv->menu_set_data = impl_menu_set_data;
	epv->menu_get_data = impl_menu_get_data;

	/* Toolbar management. */
	epv->toolbar_create = impl_toolbar_create;
	epv->toolbar_remove = impl_toolbar_remove;

	epv->toolbar_create_item    = impl_toolbar_create_item;
	epv->toolbar_remove_item    = impl_toolbar_remove_item;
	/* FIXME: toolbar_fetch_item */
	epv->toolbar_set_attributes = impl_toolbar_set_attributes;
	epv->toolbar_get_attributes = impl_toolbar_get_attributes;

	epv->toolbar_item_set_attributes = impl_toolbar_item_set_attributes;
	epv->toolbar_item_get_attributes = impl_toolbar_item_get_attributes;
	epv->toolbar_item_set_data       = impl_toolbar_item_set_data;
	epv->toolbar_item_get_data       = impl_toolbar_item_get_data;

	/* Menu notification. */
	epv->menu_activated  = impl_menu_activated;
	epv->menu_removed    = impl_menu_removed;
	epv->menu_overridden = impl_menu_overridden;
	epv->menu_reinstated = impl_menu_reinstated;

	/* Toolbar notification. */
	epv->toolbar_activated  = impl_toolbar_activated;
	epv->toolbar_removed    = impl_toolbar_removed;
	epv->toolbar_reinstated = impl_toolbar_reinstated;
	epv->toolbar_overridden = impl_toolbar_overridden;

	return epv;
}

static void
init_ui_handler_corba_class (void)
{
	/* Setup the vector of epvs */
	bonobo_ui_handler_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_ui_handler_vepv.Bonobo_UIHandler_epv = bonobo_ui_handler_get_epv ();
}
