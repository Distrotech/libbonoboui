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

#define BONOBO_UI_HANDLER_COMPILATION

#include <config.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-pixmap.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-widget.h>
#include <bonobo/bonobo-ui-handler.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "bonobo-uih-private.h"

static BonoboObjectClass   *bonobo_ui_handler_parent_class;
POA_Bonobo_UIHandler__vepv  bonobo_ui_handler_vepv;
guint                       bonobo_ui_handler_signals [LAST_SIGNAL];

static void         init_ui_handler_corba_class (void);
static CORBA_Object create_bonobo_ui_handler    (BonoboObject *object);

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
		bonobo_object_unref (BONOBO_OBJECT (uih));
		return NULL;
	}
	

	return bonobo_ui_handler_construct (uih, corba_uihandler);
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
	
	servant       = (POA_Bonobo_UIHandler *) g_new0 (BonoboObjectServant, 1);
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
bonobo_ui_handler_init (BonoboObject *object)
{
}


static void
bonobo_ui_handler_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (bonobo_ui_handler_parent_class)->destroy (object);
}

static void
bonobo_ui_handler_class_init (BonoboUIHandlerClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_ui_handler_parent_class = gtk_type_class (bonobo_object_get_type ());

	object_class->destroy = bonobo_ui_handler_destroy;

	bonobo_ui_handler_signals [MENU_ITEM_ACTIVATED] =
		gtk_signal_new ("menu_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [MENU_ITEM_REMOVED] =
		gtk_signal_new ("menu_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [MENU_ITEM_OVERRIDDEN] =
		gtk_signal_new ("menu_item_overridden",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_overridden),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [MENU_ITEM_REINSTATED] =
		gtk_signal_new ("menu_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, menu_item_reinstated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [TOOLBAR_ITEM_ACTIVATED] =
		gtk_signal_new ("toolbar_item_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_activated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [TOOLBAR_ITEM_REMOVED] =
		gtk_signal_new ("toolbar_item_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_removed),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [TOOLBAR_ITEM_OVERRIDDEN] =
		gtk_signal_new ("toolbar_item_overridden",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_overridden),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	bonobo_ui_handler_signals [TOOLBAR_ITEM_REINSTATED] =
		gtk_signal_new ("toolbar_item_reinstated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (BonoboUIHandlerClass, toolbar_item_reinstated),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER,
				GTK_TYPE_POINTER);

	gtk_object_class_add_signals (
		object_class, bonobo_ui_handler_signals, LAST_SIGNAL);

	init_ui_handler_corba_class ();
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
			"BonoboUIHandler",
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
	ui_handler->path_to_menu_callback            = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->path_to_toolbar_callback         = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->path_to_toolbar_toolbar          = g_hash_table_new (g_str_hash, g_str_equal);

	ui_handler->top->path_to_menu_item           = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_toolbar_item        = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->name_to_toolbar             = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_toolbar_item_widget = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->name_to_toolbar_widget      = g_hash_table_new (g_str_hash, g_str_equal);

	ui_handler->top->path_to_menu_widget         = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_menu_shell          = g_hash_table_new (g_str_hash, g_str_equal);

	ui_handler->top->name_to_dock                = g_hash_table_new (g_str_hash, g_str_equal);

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

/**
 * bonobo_ui_handler_get_app:
 */
GnomeApp *
bonobo_ui_handler_get_app (BonoboUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL,                NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);

	return uih->top->app;
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


static void
impl_Bonobo_UIHandler_register_containee (PortableServer_Servant  servant,
					  Bonobo_UIHandler        containee_uih,
					  CORBA_Environment      *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	if (! bonobo_ui_handler_toplevel_check_toplevel (uih)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_UIHandler_NotToplevelHandler,
				     NULL);
		return;
	}

	uih_toplevel_add_containee (uih, containee_uih);
}

static Bonobo_UIHandler
impl_Bonobo_UIHandler_get_toplevel (PortableServer_Servant  servant,
				    CORBA_Environment      *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	if (uih->top_level_uih == CORBA_OBJECT_NIL)
		return bonobo_object_dup_ref (
			bonobo_object_corba_objref (BONOBO_OBJECT (uih)), ev);

	return bonobo_object_dup_ref (uih->top_level_uih, ev);
}

/**
 * bonobo_ui_handler_set_container:
 */
void
bonobo_ui_handler_set_container (BonoboUIHandler  *uih,
				 Bonobo_UIHandler  container)
{
	CORBA_Environment  ev;
	Bonobo_UIHandler   top_level;

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
		uih->top_level_uih = top_level;

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

		bonobo_object_release_unref (uih->top_level_uih, &ev);

		/* FIXME: Check the exception */
		uih->top_level_uih = CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);
	/* FIXME: probably I should remove all local menu item data ?*/
}

typedef struct {
	BonoboUIHandler  *uih;
	Bonobo_UIHandler  containee;
	GList            *removal_list;
} removal_closure_t;

/*
 * This is a helper function used by bonobo_ui_handler_remove_containee
 * to find all of the menu items associated with a given containee.
 */
static void
menu_toplevel_find_containee_items (gpointer path,
				    gpointer value,
				    gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	GList             *l = (GList *) value;
	GList             *curr;
	CORBA_Environment  ev;

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
menu_toplevel_prune_compare_function (gconstpointer a,
				      gconstpointer b)
{
	MenuItemInternal *ai, *bi;
	int len_a, len_b;

	ai    = (MenuItemInternal *) a;
	bi    = (MenuItemInternal *) b;

	len_a = strlen (ai->item->path);
	len_b = strlen (bi->item->path);

	if (len_a > len_b)
		return -1;
	if (len_a < len_b)
		return 1;
	return 0;
}

static void
toolbar_toplevel_find_containee_items (gpointer path,
				       gpointer value,
				       gpointer user_data)
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

	ai    = (ToolbarItemInternal *) a;
	bi    = (ToolbarItemInternal *) b;

	len_a = strlen (ai->item->path);
	len_b = strlen (bi->item->path);

	if (len_a > len_b)
		return -1;
	if (len_a < len_b)
		return 1;
	return 0;
}

static void
dock_toplevel_find_containee_items (gpointer path,
				    gpointer value,
				    gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	DockInternal      *internal;
	CORBA_Environment  ev;

	CORBA_exception_init (&ev);

	internal = value;

	if (CORBA_Object_is_equivalent (internal->containee, closure->containee, &ev)) {
		closure->removal_list = g_list_prepend (closure->removal_list, internal);
	}

	CORBA_exception_free (&ev);
}

static void
uih_toplevel_unregister_containee (BonoboUIHandler  *uih,
				   Bonobo_UIHandler  containee)
{
	removal_closure_t *closure;
	Bonobo_UIHandler   remove_me;
	CORBA_Environment  ev;
	GList             *curr;

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
	closure            = g_new0 (removal_closure_t, 1);
	closure->uih       = uih;
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
		bonobo_ui_handler_menu_toplevel_remove_item_internal (
			uih, (MenuItemInternal *) curr->data, TRUE);
	g_list_free (closure->removal_list);
	g_free (closure);

	/*
	 * Create a simple closure to pass some data into the removal
	 * function.
	 */
	closure            = g_new0 (removal_closure_t, 1);
	closure->uih       = uih;
	closure->containee = containee;

	/*
	 * Remove the toolbar items associated with this containee.
	 */

	/* Build a list of internal toolbar item structures to remove. */
	g_hash_table_foreach (uih->top->path_to_toolbar_item, toolbar_toplevel_find_containee_items, closure);

	closure->removal_list = g_list_sort (closure->removal_list, toolbar_toplevel_prune_compare_function);
	
	/* Remove them */
	for (curr = closure->removal_list; curr != NULL; curr = curr->next)
		bonobo_ui_handler_toolbar_toplevel_remove_item_internal (
			uih, (ToolbarItemInternal *) curr->data);
	g_list_free (closure->removal_list);
	g_free (closure);


	/*
	 * Remove the docks associated with this guy. 
	 */

	closure            = g_new0 (removal_closure_t, 1);
	closure->uih       = uih;
	closure->containee = containee;

	/* Build a list of internal dock item structures to remove. */
	g_hash_table_foreach (uih->top->name_to_dock, dock_toplevel_find_containee_items, closure);

	/* Remove them */
	for (curr = closure->removal_list; curr != NULL; curr = curr->next) {
		DockInternal *internal = curr->data;

		bonobo_ui_handler_toplevel_dock_remove (
			uih, internal->containee, internal->name);
	}
	g_list_free (closure->removal_list);
	g_free (closure);
}

static void
uih_remote_unregister_containee (BonoboUIHandler  *uih,
				 Bonobo_UIHandler  containee)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_UIHandler_unregister_containee (uih->top_level_uih, containee, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (uih), (CORBA_Object) uih->top_level_uih, &ev);
	
	CORBA_exception_free (&ev);
}

static void
impl_Bonobo_UIHandler_unregister_containee (PortableServer_Servant  servant,
					    Bonobo_UIHandler        containee_uih,
					    CORBA_Environment      *ev)
{
	BonoboUIHandler *uih = BONOBO_UI_HANDLER (bonobo_object_from_servant (servant));

	if (! bonobo_ui_handler_toplevel_check_toplevel (uih)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_UIHandler_NotToplevelHandler,
				     NULL);
		return;
	}
	uih_toplevel_unregister_containee (uih, containee_uih);
}

/**
 * bonobo_ui_handler_remove_containee:
 */
void
bonobo_ui_handler_remove_containee (BonoboUIHandler  *uih,
				    Bonobo_UIHandler  containee)
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
bonobo_ui_handler_set_accelgroup (BonoboUIHandler *uih,
				  GtkAccelGroup   *accelgroup)
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
path_escape_forward_slashes (const char *str)
{
	const char *p = str;
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
 * bonobo_ui_handler_build_path_v:
 * 
 * Builds a path from an optional base path and a list of individual pieces.
 * 
 * @base: Beginning of path, already including leading separator and 
 * separators between pieces. Pass NULL to construct the entire path
 * from individual pieces.
 * @va_list: List of individual path pieces, without any separators yet added.
 * 
 * Result: Path ready for use with bonobo_ui_handler calls that take paths.
 */
char *
bonobo_ui_handler_build_path_v (const char *base, va_list path_components)
{
	char *path;
	const char *path_component;

	/* Note that base is not escaped, because it already contains separators. */
	path = g_strdup (base);
	for (;;) {
		char *old_path;
		char *escaped_component;
	
		path_component = va_arg (path_components, const char *);
		if (path_component == NULL)
			break;
			
		old_path = path;
		escaped_component = path_escape_forward_slashes (path_component);

		if (path == NULL) {
			path = g_strconcat ("/", escaped_component, NULL);
		} else {
			path = g_strconcat (old_path, "/", escaped_component, NULL);
		}
		g_free (old_path);
	}
	
	return path;
}

/*
 * bonobo_ui_handler_build_path:
 * 
 * Builds a path from an optional base path and a NULL-terminated list of individual pieces.
 * 
 * @base: Beginning of path, already including leading separator and 
 * separators between pieces. Pass NULL to construct the entire path
 * from individual pieces.
 * @...: List of individual path pieces, without any separators yet added, ending with NULL.
 * 
 * Result: Path ready for use with bonobo_ui_handler calls that take paths.
 */
char *
bonobo_ui_handler_build_path (const char *base, ...)
{
	va_list args;
	char *path;

	va_start (args, base);
	path = bonobo_ui_handler_build_path_v (base, args);
	va_end (args);
	
	return path;
}

/*
 * We have to manually tokenize the path (instead of using g_strsplit,
 * or a similar function) to deal with the forward-slash escaping.  So
 * we split the string on all instances of "/", except when "/" is
 * preceded by a backslash.
 */
gchar **
bonobo_ui_handler_path_tokenize (const char *path)
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

gchar *
bonobo_ui_handler_path_get_parent (const char *path)
{
	char *parent_path;
	char **toks;
	int i;

	toks = bonobo_ui_handler_path_tokenize (path);

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

/**
 * bonobo_ui_handler_get_epv:
 */
POA_Bonobo_UIHandler__epv *
bonobo_ui_handler_get_epv (void)
{
	POA_Bonobo_UIHandler__epv *epv;

	epv = g_new0 (POA_Bonobo_UIHandler__epv, 1);

	/* General server management. */
	epv->register_containee          = impl_Bonobo_UIHandler_register_containee;
	epv->unregister_containee        = impl_Bonobo_UIHandler_unregister_containee;
	epv->get_toplevel                = impl_Bonobo_UIHandler_get_toplevel;

	/* Menu management. */
	epv->menu_create                 = impl_Bonobo_UIHandler_menu_create;
	epv->menu_remove                 = impl_Bonobo_UIHandler_menu_remove;
	epv->menu_fetch                  = impl_Bonobo_UIHandler_menu_fetch;
	epv->menu_get_children           = impl_Bonobo_UIHandler_menu_get_children;
	epv->menu_set_attributes         = impl_Bonobo_UIHandler_menu_set_attributes;
	epv->menu_get_attributes         = impl_Bonobo_UIHandler_menu_get_attributes;
	epv->menu_set_data               = impl_Bonobo_UIHandler_menu_set_data;
	epv->menu_get_data               = impl_Bonobo_UIHandler_menu_get_data;

	/* Toolbar management. */
	epv->toolbar_create              = impl_Bonobo_UIHandler_toolbar_create;
	epv->toolbar_remove              = impl_Bonobo_UIHandler_toolbar_remove;

	epv->toolbar_create_item         = impl_Bonobo_UIHandler_toolbar_create_item;
	epv->toolbar_remove_item         = impl_Bonobo_UIHandler_toolbar_remove_item;
	/* FIXME: toolbar_fetch_item */
	epv->toolbar_set_attributes      = impl_Bonobo_UIHandler_toolbar_set_attributes;
	epv->toolbar_get_attributes      = impl_Bonobo_UIHandler_toolbar_get_attributes;

	epv->toolbar_item_set_attributes = impl_Bonobo_UIHandler_toolbar_item_set_attributes;
	epv->toolbar_item_get_attributes = impl_Bonobo_UIHandler_toolbar_item_get_attributes;
	epv->toolbar_item_set_data       = impl_Bonobo_UIHandler_toolbar_item_set_data;
	epv->toolbar_item_get_data       = impl_Bonobo_UIHandler_toolbar_item_get_data;

	/* Menu notification. */
	epv->menu_activated              = impl_Bonobo_UIHandler_menu_activated;
	epv->menu_removed                = impl_Bonobo_UIHandler_menu_removed;
	epv->menu_overridden             = impl_Bonobo_UIHandler_menu_overridden;
	epv->menu_reinstated             = impl_Bonobo_UIHandler_menu_reinstated;

	/* Toolbar notification. */
	epv->toolbar_activated           = impl_Bonobo_UIHandler_toolbar_activated;
	epv->toolbar_removed             = impl_Bonobo_UIHandler_toolbar_removed;
	epv->toolbar_reinstated          = impl_Bonobo_UIHandler_toolbar_reinstated;
	epv->toolbar_overridden          = impl_Bonobo_UIHandler_toolbar_overridden;

	/* Docks */
	epv->dock_add                    = impl_Bonobo_UIHandler_dock_add;
	epv->dock_remove                 = impl_Bonobo_UIHandler_dock_remove;
	epv->dock_set_sensitive          = impl_Bonobo_UIHandler_dock_set_sensitive;
	epv->dock_get_sensitive          = impl_Bonobo_UIHandler_dock_get_sensitive;

	return epv;
}

static void
init_ui_handler_corba_class (void)
{
	/* Setup the vector of epvs */
	bonobo_ui_handler_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_ui_handler_vepv.Bonobo_UIHandler_epv = bonobo_ui_handler_get_epv ();
}


gboolean
bonobo_ui_handler_toplevel_check_toplevel (BonoboUIHandler *uih)
{
	if (uih->top_level_uih != CORBA_OBJECT_NIL) {
		g_warning ("CORBA method invoked on non-toplevel UIHandler!");
		return FALSE;
	}

	return TRUE;
}

void
bonobo_ui_handler_local_do_path (const char  *parent_path,
				 const char  *item_label,
				 char       **item_path)
{
	gchar **parent_toks;
	gchar **item_toks;
	int i;

	parent_toks = bonobo_ui_handler_path_tokenize (parent_path);
	item_toks = NULL;

	/*
	 * If there is a path set on the item already, make sure it
	 * matches the parent path.
	 */
	if (*item_path != NULL) {
		int i;
		int paths_match = TRUE;
	
		item_toks = bonobo_ui_handler_path_tokenize (*item_path);

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
			g_warning ("uih_local_do_path: Item path [%s] does not jibe with parent path [%s]!",
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
 * Convenience functions to get attributes
 * NB. can be re-used for toolbars.
 */
void
bonobo_ui_handler_remote_attribute_data_free (UIRemoteAttributeData *attrs)
{
	g_return_if_fail (attrs != NULL);

	if (attrs->hint)
		CORBA_free (attrs->hint);
	if (attrs->label)
		CORBA_free (attrs->label);

	g_free (attrs);
}

static gint
bonobo_ui_handler_pixmap_xpm_get_length (const gconstpointer data, int *num_lines)
{
	int width, height, num_colors, chars_per_pixel;
	char **lines;
	int length;
	int i;

	lines = (char **) data;

	sscanf (lines [0], "%i %i %i %i", &width, &height, &num_colors, &chars_per_pixel);

	*num_lines = height + num_colors + 1;

	length = 0;
	for (i = 0; i < *num_lines; i ++)
		length += strlen (lines [i]) + 1;

	return length;
}

static gpointer
bonobo_ui_handler_pixmap_xpm_copy_data (const gconstpointer src)
{
	int num_lines;
	char **dest;
	int i;

	bonobo_ui_handler_pixmap_xpm_get_length (src, &num_lines);
	dest = g_new0 (char *, num_lines);

	/*
	 * Copy the XPM data into the destination buffer.
	 */
	for (i = 0; i < num_lines; i ++)
		dest [i] = g_strdup (((char **) src) [i]);

	return (gpointer) dest;
}


#define ALPHA_THRESHOLD 128

GtkWidget *
bonobo_ui_handler_toplevel_create_pixmap (GtkWidget *window,
					  BonoboUIHandlerPixmapType  pixmap_type,
					  gpointer                   pixmap_info)
{
	GtkWidget 	*pixmap;
	GdkPixbuf	*pixbuf;
	GdkPixmap 	*gdk_pixmap;
	GdkBitmap 	*gdk_bitmap;	
	char 		*name;
	
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

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:
		g_return_val_if_fail (pixmap_info != NULL, NULL);
		
		/* Get pointer to GdkPixbuf */
		pixbuf = (GdkPixbuf *) pixmap_info;

		/* Get GdkPixmap and mask */
		gdk_pixbuf_render_pixmap_and_mask (pixbuf, &gdk_pixmap, &gdk_bitmap, ALPHA_THRESHOLD);
			
		/* Create GtkPixmap to return */
		pixmap = gtk_pixmap_new (gdk_pixmap, gdk_bitmap);

		/* Unref source pimxap and mask data */
		gdk_pixmap_unref (gdk_pixmap);

		if (gdk_bitmap != NULL)
			gdk_bitmap_unref (gdk_bitmap);
		break;

	default:
		g_warning ("Unknown pixmap type: %d", pixmap_type);
		
	}

	return pixmap;
}

void
bonobo_ui_handler_pixmap_free_data (BonoboUIHandlerPixmapType pixmap_type, gpointer pixmap_info)
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
		bonobo_ui_handler_pixmap_xpm_get_length (pixmap_info, &num_lines);

		for (i = 0; i < num_lines; i ++)
			g_free (((char **) pixmap_info) [i]);

		g_free (pixmap_info);
		break;

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:
		g_return_if_fail (pixmap_info != NULL);
		gdk_pixbuf_unref ((GdkPixbuf *)pixmap_info);
		break;

	default:
		g_warning ("Unknown pixmap type: %d", pixmap_type);
		
	}
}

gpointer 
bonobo_ui_handler_pixmap_copy_data (BonoboUIHandlerPixmapType pixmap_type, const gconstpointer pixmap_info)
{
	switch (pixmap_type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup ((char *) pixmap_info);

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
		return g_strdup ((char *) pixmap_info);

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		return bonobo_ui_handler_pixmap_xpm_copy_data (pixmap_info);

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:
		g_return_val_if_fail(pixmap_info != NULL, NULL);
		return gdk_pixbuf_ref((GdkPixbuf *)pixmap_info);
		
	default:
		g_warning ("Unknown pixmap type: %d", pixmap_type);
		return NULL;
	}
}

Bonobo_UIHandler_PixmapType
bonobo_ui_handler_pixmap_type_to_corba (BonoboUIHandlerPixmapType type)
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
	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:
		return Bonobo_UIHandler_PixmapTypePixbufData;
	default:
		g_warning ("pixmap_type_to_corba: Unknown pixmap type [%d]!", (int) type);
		return Bonobo_UIHandler_PixmapTypeNone;
	}
}

BonoboUIHandlerPixmapType
bonobo_ui_handler_pixmap_corba_to_type (Bonobo_UIHandler_PixmapType type)
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
	case Bonobo_UIHandler_PixmapTypePixbufData:
		return BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA;
	default:
		g_warning ("pixmap_corba_to_type: Unknown pixmap type [%d]!", (int) type);
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
	}
}

/*
 * XPM data in its normal, "unflattened" form is usually an array of
 * strings.  This converts normal XPM data to a completely flat
 * sequence of characters which can be transmitted over CORBA.
 */
static gpointer
bonobo_ui_handler_pixmap_xpm_flatten (char **src, int *length)
{
	int num_lines;
	int dest_offset;
	char *flat;
	int i;

	*length = bonobo_ui_handler_pixmap_xpm_get_length (src, &num_lines);

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
bonobo_ui_handler_pixmap_xpm_unflatten (char *src, int length)
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
	for (p = src; ((p [0] != '\0') || (p [1] != '\0')) && ((p - src) < length); p++) {
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
	for (curr = line_copies, i = 0; curr != NULL; curr = curr->next, i++)
		unflattened [i] = curr->data;

	g_list_free (line_copies);

	return unflattened;
}

static char *
write_four_bytes (char *start, int value) 
{
	start [0] = value >> 24;
	start [1] = value >> 16;
	start [2] = value >> 8;
	start [3] = value;

	return start + 4;
}

static const char *
read_four_bytes (const char *start, int *value)
{
	guchar *as_uchar = (guchar *)start;

	*value = (as_uchar [0] << 24) | (as_uchar [1] << 16) |
		 (as_uchar [2] << 8) | as_uchar [3];

	return start + 4;
}

/*
 * A Pixbuf in its normal, "unflattened" form is usually an array of
 * strings.  This converts normal XPM data to a completely flat
 * sequence of characters which can be transmitted over CORBA.
 *
 * 	Data is stored in the buffer in this format:
 * 		width		<4 bytes>
 * 		height		<4 bytes>
 * 		has_alpha	<1 byte>
 * 		
 * 	This is followed by the pixbuf data which has been written
 *      into the buffer as a sequences of chars, row by row.
 */
static Bonobo_UIHandler_iobuf *
bonobo_ui_handler_pixmap_pixbuf_flatten (Bonobo_UIHandler_iobuf *buffer, 
					 GdkPixbuf              *pixbuf)
{
	int 	size, width, height, row;
	char 	*dst;
	guchar	*src;
	int      row_stride, flattened_row_stride;
	gboolean has_alpha;
			
	g_return_val_if_fail (pixbuf != NULL, NULL);

	width  = gdk_pixbuf_get_width  (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	flattened_row_stride = width * (3 + (has_alpha ? 1 : 0));
	
	size = height * flattened_row_stride;

	buffer->_length  = 2 * 4 + 1 + size;
	buffer->_buffer  = CORBA_sequence_CORBA_octet_allocbuf(buffer->_length);

	dst = (char *)buffer->_buffer;

	dst = write_four_bytes (dst, gdk_pixbuf_get_width  (pixbuf));
	dst = write_four_bytes (dst, gdk_pixbuf_get_height (pixbuf));
	*dst = has_alpha;
	dst++;

	/* Copy over bitmap information */	
	src        = gdk_pixbuf_get_pixels    (pixbuf);
	row_stride = gdk_pixbuf_get_rowstride (pixbuf);
			
	for (row = 0; row < height; row++) {
		memcpy (dst, src, flattened_row_stride);
		dst += flattened_row_stride;
		src += row_stride;
	}

	/* Check that we copied the correct amount of data */
	g_assert (dst - (char *)buffer->_buffer == buffer->_length);
	
	return buffer;
}

/*
 * After a flattened Pixbuf has been received via CORBA, it can be
 * converted to the normal, unflattened form with this function.
 */
static gpointer
bonobo_ui_handler_pixmap_pixbuf_unflatten (char *flat_data, int length)
{

	GdkPixbuf 	*pixbuf;
	int 		width, height;
	gboolean 	has_alpha;
	int 		pix_length, row;
	int             flattened_row_stride, row_stride;
	const char 	*src;
	guchar		*dst;

	g_return_val_if_fail (flat_data != NULL, NULL);
	
	if (length < 2 * 4 + 1) {
		g_warning ("bonobo_ui_handler_pixmap_pixbuf_unflatten(): Length not large enough to contain geometry info.");
		return NULL;
	}
	 
	src = (char *)flat_data;
	src = read_four_bytes (src, &width);
	src = read_four_bytes (src, &height);
	has_alpha = *src++;
	
	flattened_row_stride = width * (3 + (has_alpha ? 1 : 0));
	pix_length = height * flattened_row_stride;

	if (length != pix_length + 2 * 4 + 1) {
		g_warning ("bonobo_ui_handler_pixmap_pixbuf_unflatten(): flat_data buffer has an improper size.");
		return NULL;
	}
	
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, width, height);

	dst        = gdk_pixbuf_get_pixels    (pixbuf);
	row_stride = gdk_pixbuf_get_rowstride (pixbuf);
	
	for (row = 0; row < height; row++) {
		memcpy (dst, src, flattened_row_stride);
		dst += row_stride;
		src += flattened_row_stride;
	}

	/* Verify that we copied the proper amount of data */							  		
	g_assert (src - (char *)flat_data == length);
	
	return pixbuf;
}


Bonobo_UIHandler_iobuf *
bonobo_ui_handler_pixmap_data_to_corba (BonoboUIHandlerPixmapType type, gpointer data)
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
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (buffer->_length);
		strcpy (buffer->_buffer, (char *) data);
		return buffer;

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		temp_xpm_buffer = bonobo_ui_handler_pixmap_xpm_flatten (data, &(buffer->_length));
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (buffer->_length);
		memcpy (buffer->_buffer, temp_xpm_buffer, buffer->_length);
		g_free (temp_xpm_buffer);
		return buffer;

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA:		
		buffer = bonobo_ui_handler_pixmap_pixbuf_flatten(buffer, (GdkPixbuf *)data);
		return buffer;
		
	default:
		g_warning ("bonobo_ui_handler_pixmap_data_to_corba: Unknown pixmap type [%d]", type);
		buffer->_length = 1;
		buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);
		return buffer;
	}

	return buffer;
}

gpointer
bonobo_ui_handler_pixmap_corba_to_data (Bonobo_UIHandler_PixmapType corba_pixmap_type,
					const Bonobo_UIHandler_iobuf *corba_pixmap_data)
{
	BonoboUIHandlerPixmapType type;
	gpointer pixmap_data;

	type = bonobo_ui_handler_pixmap_corba_to_type (corba_pixmap_type);

	switch (type) {
	case BONOBO_UI_HANDLER_PIXMAP_NONE:
		return NULL;

	case BONOBO_UI_HANDLER_PIXMAP_FILENAME:
	case BONOBO_UI_HANDLER_PIXMAP_STOCK:
		return g_strdup (corba_pixmap_data->_buffer);

	case BONOBO_UI_HANDLER_PIXMAP_XPM_DATA:
		pixmap_data = bonobo_ui_handler_pixmap_xpm_unflatten (
			corba_pixmap_data->_buffer, corba_pixmap_data->_length);
		return pixmap_data;

	case BONOBO_UI_HANDLER_PIXMAP_PIXBUF_DATA: 
		pixmap_data = bonobo_ui_handler_pixmap_pixbuf_unflatten (
			corba_pixmap_data->_buffer, corba_pixmap_data->_length);
		return pixmap_data;
		
	default:
		g_warning ("pixmap_corba_to_data: Unknown pixmap type [%d]", type);
		return NULL;
	}
}

BonoboUIHandlerPixmapType
bonobo_ui_handler_uiinfo_pixmap_type_to_uih (GnomeUIPixmapType ui_type)
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
		g_warning ("Unknown GnomeUIPixmapType: %d", ui_type);
		return BONOBO_UI_HANDLER_PIXMAP_NONE;
	}
}
