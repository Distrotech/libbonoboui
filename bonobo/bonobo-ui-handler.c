/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * GNOME::UIHandler.
 *
 * Author:
 *    Nat Friedman (nat@gnome-support.com)
 */

/*
 * Notes to self:
 *  - in mutation functions, modify internal data _and_ propagate to container.
 *    internal data setting always has to be done.
 *  - emit signals in the appropriate places.
 *  - dynamic lists
 *     . removal especially
 *  - make sure this is a workable model for scripting languages.
 *  - comment static routines.
 *  - overriding needs to be handled properly.
 *  - make sure we handle child lists properly.
 *    . implement fetch_tree
 *  - handle menu item positions.
 *  - make sure it's easy to walk the tree.  Note that if you don't take a snapshot, it may
 *    change while you walk it.
 *  - popup menus!  fuck.  Need to check the gnome app functionality for these
 *  - tree/list/one:
 *		One is always just one.
 *		List is always a null-terminated list, which is parsed recursively.  It's a list of structures, NOT
 *		 a list of pointers to structures.
 *		Tree is a single item which is parsed recursively.
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

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_ui_handler_parent_class;

/* The entry point vectors for the server we provide */
static POA_GNOME_UIHandler__epv gnome_ui_handler_epv;
static POA_GNOME_UIHandler__vepv gnome_ui_handler_vepv;

enum {
	MENU_ITEM_ACTIVATED,
	MENU_ITEM_OVERRIDEN,
	TOOLBAR_ITEM_ACTIVATED,
	TOOLBAR_ITEM_OVERRIDEN,
	LAST_SIGNAL
};

static guint uih_signals [LAST_SIGNAL];

/*
 * This structure is used to maintain an internal representation of a
 * menu item.
 */
typedef struct {

	/*
	 * The GnomeUIHandler.  FIXME: This is gross.  We need it for
	 * menu_item_activated, though.
	 */
	GnomeUIHandler *uih;

	/*
	 * A copy of the GnomeUIHandlerMenuItem for this toolbar item.
	 */
	GnomeUIHandlerMenuItem *item;

	/*
	 * If this item is a subtree or a radio group, this list
	 * contains the item's children.  It is a list of path
	 * strings.
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
} MenuItemInternal;

/*
 * This structure is used to maintain an internal representation of a
 * toolbar item.
 */

typedef struct {
	/*
	 * The GnomeUIHandler.  FIXME: This is gross.  We need it for
	 * toolbar_item_activated, though.
	 */
	GnomeUIHandler *uih;

	/*
	 * A copy of the GnomeUIHandlerToolbarItem for this toolbar item.
	 */
	GnomeUIHandlerToolbarItem *item;

	/*
	 * If this item is a subtree or a radio group, this list
	 * contains the item's children.  It is a list of path
	 * strings.
	 */
	GList *children;

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
} ToolbarItemInternal;

static MenuItemInternal *store_menu_item_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, GnomeUIHandlerMenuItem *item);
static void remove_menu_item (GnomeUIHandler *uih, MenuItemInternal *internal_remove_me, gboolean replace);
static void impl_GNOME_UIHandler_register_containee (PortableServer_Servant servant, GNOME_UIHandler uih,
						     CORBA_Environment *ev);
static void impl_GNOME_UIHandler_menu_item_activated (PortableServer_Servant servant, const CORBA_char *path,
						      CORBA_Environment *ev);
static void
impl_GNOME_UIHandler_menu_item_create (PortableServer_Servant servant,
				       const CORBA_char *path,
				       const GNOME_UIHandler_MenuType menu_type,
				       const CORBA_char *label,
				       const CORBA_char *hint,
				       const GNOME_UIHandler_iobuf *pixmap_data,
				       const CORBA_unsigned_long accelerator_key,
				       const CORBA_long modifier,
				       const CORBA_long pos,
				       const GNOME_UIHandler containee_uih,
				       CORBA_Environment *ev);

static CORBA_Object
create_gnome_ui_handler (GnomeObject *object)
{
	POA_GNOME_UIHandler *servant;
	CORBA_Object o;
	
	servant = (POA_GNOME_UIHandler *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_ui_handler_vepv;

	POA_GNOME_UIHandler__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

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
	/* The entry point vectors for this GNOME::UIHandler class */
	gnome_ui_handler_epv.register_containee = impl_GNOME_UIHandler_register_containee;
	gnome_ui_handler_epv.menu_item_activated = impl_GNOME_UIHandler_menu_item_activated;
	gnome_ui_handler_epv.menu_item_create = impl_GNOME_UIHandler_menu_item_create;
#if 0
	gnome_ui_handler_epv.unregister_containee = impl_GNOME_UIHandler_unregister_containee;
	gnome_ui_handler_epv.menu_item_remove = impl_GNOME_UIHandler_menu_item_remove;
	gnome_ui_handler_epv.menu_item_fetch = impl_GNOME_UIHandler_menu_item_fetch;
	gnome_ui_handler_epv.menu_item_get_pos = impl_GNOME_UIHandler_menu_item_get_pos;
	gnome_ui_handler_epv.menu_item_set_sensitivity = impl_GNOME_UIHandler_menu_item_set_sensitivity;
	gnome_ui_handler_epv.menu_item_get_sensitivity = impl_GNOME_UIHandler_menu_item_get_sensitivity;
#endif
	/* FIXME: Finish these */

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

	uih_signals [MENU_ITEM_OVERRIDEN] =
		gtk_signal_new ("menu_item_overriden",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, menu_item_overriden),
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

	uih_signals [TOOLBAR_ITEM_OVERRIDEN] =
		gtk_signal_new ("toolbar_item_overriden",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeUIHandlerClass, toolbar_item_overriden),
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

/*
 * Basic object creation/manipulation functions.
 */

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
	MenuItemInternal *root;
	GList *l;

	g_return_val_if_fail (ui_handler != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (ui_handler), NULL);
	g_return_val_if_fail (corba_uihandler != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (ui_handler), corba_uihandler);

	ui_handler->top = g_new0 (GnomeUIHandlerTopLevelData, 1);

	ui_handler->path_to_menu_item = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->path_to_toolbar_item = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_menu_widget = g_hash_table_new (g_str_hash, g_str_equal);
	ui_handler->top->path_to_menu_shell = g_hash_table_new (g_str_hash, g_str_equal);

	/*
	 * Create the root menu item entry in the internal data structures so
	 * that the user doesn't have to do this himself.
	 */
	root = g_new0 (MenuItemInternal, 1);
	root->uih = ui_handler;
	root->item = NULL;
	root->children = NULL;
	root->uih_corba = gnome_object_corba_objref (GNOME_OBJECT (ui_handler));
	l = g_list_prepend (NULL, root);
	g_hash_table_insert (ui_handler->path_to_menu_item, "/", l);

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
 * gnome_ui_handler_set_container:
 */
void
gnome_ui_handler_set_container (GnomeUIHandler *uih, GNOME_UIHandler container)
{
	CORBA_Environment ev;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	/*
	 * Unregister with an existing container, if we have one
	 */
	if (uih->container_uih != CORBA_OBJECT_NIL) {

		GNOME_UIHandler_unregister_containee (uih->container_uih,
						      gnome_object_corba_objref (GNOME_OBJECT (uih)), &ev);
		/* FIXME: Check the exception */
	}

	/*
	 * Register with the new one.
	 */
	GNOME_UIHandler_register_containee (container, gnome_object_corba_objref (GNOME_OBJECT (uih)), &ev);

	CORBA_exception_free (&ev);
					    
	uih->container_uih = container;
}

/**
 * gnome_ui_handler_add_containee:
 */
void
gnome_ui_handler_add_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	uih->containee_uihs = g_list_prepend (uih->containee_uihs, containee);
}

typedef struct {
	GnomeUIHandler *uih;
	GNOME_UIHandler containee;
} removal_closure_t;

/*
 * This is a helper function used by gnome_ui_handler_remove_containee
 * to remove all of the menu items associated with a given containee.
 */
static gboolean
remove_containee_menu_item (gpointer path, gpointer value, gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	GList *l = (GList *) value;

	MenuItemInternal *remove_me;
	GList *curr;

	do {
		remove_me = NULL;
		for (curr = l; curr != NULL; curr = curr->next) {
			MenuItemInternal *internal = (MenuItemInternal *) curr->data;

			if (internal->uih_corba == closure->containee) {
				remove_me = internal;
				break;
			}
		}


/* FIXME FIXME FIXME		g_list_remove ( .. FIXME .. ); */

	} while (remove_me != NULL);

	/*
	 * If there are no items left in the list, then remove this
	 * entry from the hash table.
	 */
	if (l == NULL)
		return TRUE;

	
	/*
	 * Otherwise, just insert the new shortened list into the hash
	 * table.
	 *
	 * FIXME: Handle maintaining subtree child lists.
	 *
	 */
	g_hash_table_insert (closure->uih->path_to_menu_item, (char *) path, l);

	return FALSE;
}

static gboolean
remove_containee_toolbar_item (gpointer path, gpointer value, gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	GList *l = (GList *) value;

	ToolbarItemInternal *remove_me;
	GList *curr;

	do {
		remove_me = NULL;
		for (curr = l; curr != NULL; curr = curr->next) {
			ToolbarItemInternal *internal = (ToolbarItemInternal *) curr->data;

			if (internal->uih_corba == closure->containee) {
				remove_me = internal;
				break;
			}
		}

		l = g_list_remove (l, remove_me);
	} while (remove_me != NULL);

	/*
	 * If there are no items left in the list, then remove this
	 * entry from the hash table.
	 */
	if (l == NULL)
		return TRUE;

	/*
	 * Otherwise, just insert the new shortened list into the hash
	 * table.
	 */
	g_hash_table_insert (closure->uih->path_to_toolbar_item, (char *) path, l);

	return FALSE;
}

/**
 * gnome_ui_handler_remove_containee:
 */
void
gnome_ui_handler_remove_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	removal_closure_t *closure;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	uih->containee_uihs = g_list_remove (uih->containee_uihs, containee);

	/*
	 * Create a simple closure to pass some data into the removal
	 * function.
	 */
	closure = g_new0 (removal_closure_t, 1);
	closure->uih = uih;
	closure->containee = containee;

	/*
	 * Remove all of the menu items owned by this containee.
	 */
	g_hash_table_foreach_remove (uih->path_to_menu_item, remove_containee_menu_item, closure);

	/*
	 * Remove the toolbar items owned by this containee.
	 */
	g_hash_table_foreach_remove (uih->path_to_menu_item, remove_containee_toolbar_item, closure);
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
escape_forward_slashes (char *str)
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
unescape_forward_slashes (char *str)
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
		char *escaped_component = escape_forward_slashes (path_component);

		path = g_strconcat (old_path, "/", escaped_component);
		g_free (old_path);
	}
	va_end (args);
	
	return path;
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

/*
 * This function grabs the current active menu item associated with a
 * given menu path string.
 */
static MenuItemInternal *
get_menu_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_menu_item, path);

	if (l == NULL)
		return NULL;

	return l->data;
}

/*
 * This function maps a path to either a subtree menu shell or the
 * top-level menu bar.  It returns NULL if the specified path is
 * not a subtree path (or "/").
 */
static GtkWidget *
get_menu_shell (GnomeUIHandler *uih, char *path)
{
	if (! strcmp (path, "/"))
		return uih->top->menubar;

	return g_hash_table_lookup (uih->top->path_to_menu_shell, path);
}

/**
 * gnome_ui_handler_create_menubar:
 */
void
gnome_ui_handler_create_menubar (GnomeUIHandler *uih, GnomeApp *app)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));

	uih->top->menubar = gtk_menu_bar_new ();

	gnome_app_set_menus (app, GTK_MENU_BAR (uih->top->menubar));
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

/*
 * We have to manually tokenize the path (instead of using g_strsplit,
 * or a similar function) to deal with the forward-slash escaping.  So
 * we split the string on all instances of "/", except when "/" is
 * preceded by a backslash.
 */
static gchar **
tokenize_path (char *path)
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
get_parent_path (const char *path)
{
	char *parent_path;
	char **toks;
	int i;

	toks = tokenize_path (path);

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
		printf ("i: %d\n", i);
		g_free (toks [i]);
	}
	g_free (toks);


	return parent_path;
} 

/*
 * This function checks to make sure that the path of a given
 * GnomeUIHandlerMenuItem is consistent with the path of its parent.
 * If the item does not have a path, one is created for it.  If the
 * path is not consistent with the parent's path, an error is printed
 */
static void
do_menu_item_path (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	gchar **parent_toks;
	gchar **item_toks;
	int i;

	parent_toks = tokenize_path (parent_path);
	item_toks = NULL;

	/*
	 * If there is a path set on the item already, make sure it
	 * matches the parent path.
	 */
	if (item->path != NULL) {
		int i;
		int paths_match = TRUE;
	
		item_toks = tokenize_path (item->path);

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
			g_warning ("do_menu_item_path: Item path [%s] does not jibe with parent path [%s]!\n",
				   item->path, parent_path);
	}

	/*
	 * Build a path for the item.
	 */
	if (item->path == NULL) {
		char *path_component;

		/*
		 * If the item has a label, then use the label
		 * as the path component.
		 */
		if (item->label != NULL) {
			char *tmp;

			tmp = remove_ulines (item->label);
			path_component = escape_forward_slashes (tmp);
			g_free (tmp);
		} else {

			/*
			 * FIXME: This is an ugly hack to give us
			 * a unique path for an item with no label.
			 */
			path_component = g_new0 (char, 32);
			snprintf (path_component, 32, "%x", (unsigned long) ((char *) item + rand () % 10000));
		}

		if (parent_path [strlen (parent_path) - 1] == '/')
			item->path = g_strconcat (parent_path, path_component, NULL);
		else
			item->path = g_strconcat (parent_path, "/", path_component, NULL);

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

static GtkWidget *
create_menu_label (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item,
		       GtkWidget *parent_menu_shell, GtkWidget *menu_item)
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
	gtk_container_add (GTK_CONTAINER (menu_item), label);
	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_item);

	/*
	 * Setup the underline accelerators.
	 *
	 * FIXME: Should this be an option?
	 */
	keyval = gtk_label_parse_uline (GTK_LABEL (label), item->label);

	if (keyval != GDK_VoidSymbol) {
		if (GTK_IS_MENU (parent_menu_shell))
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    gtk_menu_ensure_uline_accel_group (GTK_MENU (parent_menu_shell)),
						    keyval, 0,
						    0);

		if (GTK_IS_MENU_BAR (parent_menu_shell) && uih->top->accelgroup != NULL)
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    uih->top->accelgroup,
						    keyval, GDK_MOD1_MASK,
						    0);
	}

	return label;
}

static void
remove_menu_label (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *parent_shell,
		   GtkWidget *menu_item)
{
	GtkWidget *child;
	guint keyval;
	
	child = GTK_BIN (menu_item)->child;

	/*
	 * Remove the old label widget.
	 */

	if (child != NULL) {
		/*
		 * Grab the keyval for the label, if appropriate, so the
		 * corresponding accelerator can be removed.
		 */
		keyval = gtk_label_parse_uline (GTK_LABEL (child), item->label);
		if (uih->top->accelgroup != NULL) {
			if (parent_shell != uih->top->menubar)
				gtk_widget_remove_accelerator (GTK_WIDGET (menu_item), uih->top->accelgroup, keyval, 0);
			else
				gtk_widget_remove_accelerator (GTK_WIDGET (menu_item), uih->top->accelgroup,
							       keyval, GDK_MOD1_MASK);
		}
			
		/*
		 * Now remove the old label widget.
		 */
		gtk_container_remove (GTK_CONTAINER (menu_item), child);
	}
		
}


static GnomeUIHandlerMenuItem *
make_menu_item (char *path, GnomeUIHandlerMenuItemType type,
		char *label, char *hint, gint dynlist_maxsize,
		GnomeUIHandlerPixmapType pixmap_type,
		gpointer pixmap_data,
		guint accelerator_key, GdkModifierType ac_mods,
		GnomeUIHandlerCallbackFunc callback,
		gpointer callback_data, int pos)

{
	GnomeUIHandlerMenuItem *item;

	item = g_new0 (GnomeUIHandlerMenuItem, 1);
	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->dynlist_maxsize = dynlist_maxsize;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods = ac_mods;
	item->callback = callback;
	item->callback_data = callback_data;

	return item;
}


static GtkPixmap *
create_pixmap (GnomeUIHandlerPixmapType type, gpointer data)
{
	/* FIXME: Implement me */
	g_error ("Implement me");

	return NULL;
}

static void
free_pixmap_data (GnomeUIHandlerPixmapType type, gpointer data)
{
}

static gpointer 
copy_pixmap_data (GnomeUIHandlerPixmapType type, gpointer data)
{
	g_warning ("Implement me please");
	return NULL;
}

/*
 * This callback puts the hint for a menu item into the GnomeAppBar
 * when the menu item is selected.
 */
static void
put_menu_hint_in_appbar (GtkWidget *menu_item, gpointer data)
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
remove_menu_hint_from_appbar (GtkWidget *menu_item, gpointer data)
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
put_menu_hint_in_statusbar (GtkWidget *menu_item, gpointer data)
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
remove_menu_hint_from_statusbar (GtkWidget *menu_item, gpointer data)
{
	GtkWidget *bar = GTK_WIDGET (data);
	guint id;

	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (bar));

	id = gtk_statusbar_get_context_id (GTK_STATUSBAR (bar), "gnome-ui-handler:menu-hint");
	
	gtk_statusbar_pop (GTK_STATUSBAR (bar), id);
}

static void
set_menu_hint (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *menu_item)
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
		gtk_signal_connect (GTK_OBJECT (menu_item), "select", GTK_SIGNAL_FUNC (put_menu_hint_in_statusbar),
				    uih->top->statusbar);
		gtk_signal_connect (GTK_OBJECT (menu_item), "deselect", GTK_SIGNAL_FUNC (remove_menu_hint_from_statusbar),
				    uih->top->statusbar);

		return;
	}

	if (GNOME_IS_APPBAR (uih->top->statusbar)) {
		gtk_signal_connect (GTK_OBJECT (menu_item), "select", GTK_SIGNAL_FUNC (put_menu_hint_in_appbar),
				    uih->top->statusbar);
		gtk_signal_connect (GTK_OBJECT (menu_item), "deselect", GTK_SIGNAL_FUNC (remove_menu_hint_from_appbar),
				    uih->top->statusbar);

		return;
	}
}

static GtkWidget *
create_gtk_menu_item (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	GtkWidget *menu_item;
	MenuItemInternal *rg;

	switch (item->type) {
	case GNOME_UI_HANDLER_MENU_ITEM:
	case GNOME_UI_HANDLER_MENU_SUBTREE:

		/*
		 * Create a GtkMenuItem widget if it's a plain item.  If it contains
		 * a pixmap, create a GtkPixmapMenuItem.
		 */
		if (item->pixmap_data != NULL && item->pixmap_type != GNOME_UI_HANDLER_PIXMAP_NONE) {
			GtkPixmap *pixmap;

			menu_item = gtk_pixmap_menu_item_new ();

			pixmap = create_pixmap (item->pixmap_type, item->pixmap_data);
			if (pixmap != NULL) {
				gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_item),
								 GTK_WIDGET (pixmap));
				gtk_widget_show (GTK_WIDGET (pixmap));
			}
		} else {
			menu_item = gtk_menu_item_new ();
		}

		break;

	case GNOME_UI_HANDLER_MENU_RADIOITEM:
		/*
		 * The parent path should be the path to the radio
		 * group lead item.
		 */
		rg = get_menu_item (uih, parent_path);

		/*
		 * Create the new radio menu item and add it to the
		 * radio group.
		 */
		menu_item = gtk_radio_menu_item_new (rg->radio_items);
		rg->radio_items = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_TOGGLEITEM:
		menu_item = gtk_check_menu_item_new ();

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_SEPARATOR:

		/*
		 * A separator is just a plain menu item with its sensitivity
		 * set to FALSE.
		 */
		menu_item = gtk_menu_item_new ();
		gtk_widget_set_sensitive (menu_item, FALSE);
		break;

	default:
		g_warning ("create_menu_item: Invalid GnomeUIHandlerMenuItemType %d\n",
			   (int) item->type);
		return NULL;
			
	}

	if (uih->top->accelgroup == NULL)
		gtk_widget_lock_accelerators (menu_item);

	/*
	 * Create a label for the menu item.
	 */
	if (item->label != NULL) {
		GtkWidget *parent_menu_shell;

		parent_menu_shell = g_hash_table_lookup (uih->top->path_to_menu_shell, parent_path);

		/* Create the label. */
		create_menu_label (uih, item, parent_menu_shell, menu_item);
	}

	gtk_widget_show (menu_item);


	/*
	 * Add the hint for this menu item.
	 */
	if (item->hint != NULL) {
		set_menu_hint (uih, item, menu_item);
	}
	
	return menu_item;
}

static gint
save_accels (gpointer data)
{
	gchar *file_name;

	file_name = g_concat_dir_and_file (gnome_user_accels_dir, gnome_app_id);
	gtk_item_factory_dump_rc (file_name, NULL, TRUE);
	g_free (file_name);

	return TRUE;
}

static void
install_global_accelerators (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *menu_item)
{
	static guint save_accels_id = 0;
	char *globalaccelstr;

	g_return_if_fail (gnome_app_id != NULL);
	globalaccelstr = g_strconcat (gnome_app_id, ">", item->path, "<", NULL);
	gtk_item_factory_add_foreign (menu_item, globalaccelstr, uih->top->accelgroup,
				      item->accelerator_key, item->ac_mods);
	g_free (globalaccelstr);

	if (! save_accels_id)
		save_accels_id = gtk_quit_add (1, save_accels, NULL);

}

/*
 * This function only applies to top-level UIHandler objects, i.e. the
 * UIHandlers which actually maintain the widgets.
 *
 * This function is the callback through which all GtkMenuItem
 * activations pass.  If a user selects a menu item, this function is
 * the first to know about it.
 *
 * So the job of this function is to dispatch to the appropriate user
 * callback or containee object.
 */
static gint
menu_item_activated (GtkWidget *menu_item, gpointer data)
{
	MenuItemInternal *internal = (MenuItemInternal *) data;

	/*
	 * If this item is owned by a containee, then we forward the
	 * activation on to the containee.
	 */
	if ((internal->uih_corba != NULL)
	    && (gnome_object_corba_objref (GNOME_OBJECT (internal->uih)) != internal->uih_corba)) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		GNOME_UIHandler_menu_item_activated (internal->uih_corba, internal->item->path, &ev);
		/* FIXME: Check exception. */
		CORBA_exception_free (&ev);

		return FALSE;
	}

	/*
	 * If this menu item is locally owned, then we dispatch to the
	 * local callback, if it exists.
	 */
	if (internal->item->callback == NULL)
		return FALSE;

	(internal->item->callback) (internal->uih, internal->item->callback_data, internal->item->path);

	return FALSE;
}


static GnomeUIHandlerMenuItem *
copy_menu_item (GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *copy;	

	copy = g_new0 (GnomeUIHandlerMenuItem, 1);

#define COPY_STRING(x) ((x) == NULL ? NULL : g_strdup (x))
	copy->path = COPY_STRING (item->path);
	copy->type = item->type;
	copy->hint = COPY_STRING (item->hint);
	copy->label = COPY_STRING (item->label);
	copy->children = NULL; /* FIXME: Make sure this gets handled properly. */
	copy->dynlist_maxsize = item->dynlist_maxsize;

	copy->pixmap_data = copy_pixmap_data (item->pixmap_type, item->pixmap_data);
	copy->pixmap_type = item->pixmap_type;

	copy->accelerator_key = item->accelerator_key;
	copy->ac_mods = item->ac_mods;
	copy->callback = item->callback;
	copy->callback_data = item->callback_data;

	return copy;
}

/*
 * This function creates the parent of a menu item, if it does not
 * exist.  This function will only get invoked if this UIHandler
 * is a containee, and does not have information on the menu items
 * residing in its parents.  Full lineages are created recursively
 * if several of an item's ancestors aren't present in the internal
 * data.
 */

static void
create_parent_menu_item (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, char *item_path)
{
	GnomeUIHandlerMenuItem *parent_item;

	parent_item = g_new0 (GnomeUIHandlerMenuItem, 1);
	parent_item->path = get_parent_path (item_path);
	parent_item->type = GNOME_UI_HANDLER_MENU_SUBTREE;

	store_menu_item_data (uih, uih_corba, parent_item);
}


static MenuItemInternal *
store_menu_item_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal, *parent;
	char *parent_path;
	GList *l;

	/*
	 * Create the internal representation of the menu item.
	 */
	internal = g_new0 (MenuItemInternal, 1);
	internal->item = copy_menu_item (item);
	internal->uih = uih;
	internal->uih_corba = uih_corba;
	
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
	l = g_hash_table_lookup (uih->path_to_menu_item, internal->item->path);
	l = g_list_prepend (l, internal);

	g_hash_table_insert (uih->path_to_menu_item, internal->item->path, l);

	/*
	 * Now insert a child record for this item's parent, if it has
	 * a parent.
	 */
	parent_path = get_parent_path (item->path);
	parent = get_menu_item (uih, parent_path);

	/*
	 * If we don't have internal data for the parent,
	 * then either the user hasn't created the appropriate
	 * parent menu items, or they were created by our container.
	 *
	 * If they were created by the container, and we just don't
	 * have data on them, that's ok.  But if we are the top-level
	 * UIHandler, then we must insist they exist.
	 */
	if (parent == NULL) {

		if (uih->container_uih == CORBA_OBJECT_NIL) {
			g_warning ("store_menu_item_data: Parent data does not exist for path [%s]!\n",
				   item->path);
			/* FIXME: Memory leak */

			return NULL;
		}


		create_parent_menu_item (uih, uih_corba, item->path);
		parent = get_menu_item (uih, parent_path);
	}

	g_free (parent_path);

	if (! g_list_find_custom (parent->children, item->path, g_str_equal))
		parent->children = g_list_prepend (parent->children, item->path);

	return internal;
}

/*
 * This internal function does all the work associated with
 * creating a menu item except:
 *
 *	1. Storing the internal representation for the menu item.
 *	2. Connecting to the menu item activation signal.
 */
static GtkWidget *
create_menu_item_widgets (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item, int pos)
{
	GtkWidget *parent_menu_shell;
	GtkWidget *menu_item;

	/*
	 * Make sure that the parent path exists before creating the
	 * item.
	 */
	parent_menu_shell = get_menu_shell (uih, parent_path);
	g_return_val_if_fail (parent_menu_shell != NULL, NULL);

	/*
	 * Create the GtkMenuItem widget for this item, and stash it
	 * in the hash table.
	 */
	menu_item = create_gtk_menu_item (uih, parent_path, item);
	g_hash_table_insert (uih->top->path_to_menu_widget, item->path, menu_item);

	/*
	 * Insert the new GtkMenuItem into the menu shell of the
	 * parent.
	 */
	gtk_menu_shell_insert (GTK_MENU_SHELL (parent_menu_shell), menu_item, pos);

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
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), GTK_WIDGET (menu));

		/*
		 * Store the menu shell for later retrieval (e.g. when
		 * we add submenu items into this shell).
		 */
		g_hash_table_insert (uih->top->path_to_menu_shell, item->path, menu);
	}

	/*
	 * Install the global accelerators.
	 *
	 * FIXME: I'm not entirely sure what this does.
	 */
	install_global_accelerators (uih, item, menu_item);

	return menu_item;
}


static void
menu_item_connect_signal (GtkWidget *menu_item, MenuItemInternal *internal)
{
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (menu_item_activated), internal);
}

/*
 * This function uses create_menu_item_widgets to do most of the work to create
 * a new menu item, and then:
 *
 *	1. Stores the MenuItemInternal structure for the item.
 *	2. Connects to the menu item activation signal.
 */
static void
create_menu_item (GnomeUIHandler *uih, char *parent_path, MenuItemInternal *internal, int pos)
{
	GtkWidget *menu_item;

	/*
	 * This function performs most of the work involved in creating
	 * a new menu item.
	 */
	menu_item = create_menu_item_widgets (uih, parent_path, internal->item, pos);

	/*
	 * Connect to the activate signal.
	 */
	menu_item_connect_signal (menu_item, internal);
}

static GNOME_UIHandler_MenuType
menu_type_to_corba (GnomeUIHandlerMenuItemType type)
{
	switch (type) {
	case GNOME_UI_HANDLER_MENU_END:
		g_warning ("Warning: Passing MenuTypeEnd through CORBA!");
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
	case GNOME_UI_HANDLER_MENU_DYNLIST:
		return GNOME_UIHandler_MenuTypeDynlist;
	case GNOME_UI_HANDLER_MENU_SEPARATOR:
		return GNOME_UIHandler_MenuTypeSeparator;
	default:
		g_warning ("Unknown GnomeUIHandlerMenuItemType %d!\n", (int) type);
		return GNOME_UIHandler_MenuTypeItem;
		
	}
}

static GnomeUIHandlerMenuItemType
corba_to_menu_type (GNOME_UIHandler_MenuType type)
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
	case GNOME_UIHandler_MenuTypeDynlist:
		return GNOME_UI_HANDLER_MENU_DYNLIST;
	case GNOME_UIHandler_MenuTypeSeparator:
		return GNOME_UI_HANDLER_MENU_SEPARATOR;
	default:
		g_warning ("Unknown GNOME_UIHandler menu type %d!\n", (int) type);
		return GNOME_UI_HANDLER_MENU_ITEM;
		
	}
}

static GNOME_UIHandler_iobuf *
pixmap_data_to_corba (GnomeUIHandlerPixmapType type, gpointer data)
{
	GNOME_UIHandler_iobuf *buffer;

	/* FIXME: Free me */
	buffer = GNOME_UIHandler_iobuf__alloc ();
	CORBA_sequence_set_release (buffer, TRUE);
	buffer->_length = 1;
	buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1);

	return buffer;
}

static void
menu_item_container_create (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, int pos)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

#define CORBIFY_STRING(s) (s == NULL ? "" : s)
	GNOME_UIHandler_menu_item_create (uih->container_uih,
					  item->path,
					  menu_type_to_corba (item->type),
					  CORBIFY_STRING (item->label),
					  CORBIFY_STRING (item->hint),
					  pixmap_data_to_corba (item->pixmap_type,
								item->pixmap_data),
					  (CORBA_unsigned_long) item->accelerator_key,
					  (CORBA_long) item->ac_mods,
					  pos,
					  gnome_object_corba_objref (uih),
					  &ev);

	/* FIXME: Check exception here */

	CORBA_exception_free (&ev);
}

static void
impl_GNOME_UIHandler_register_containee (PortableServer_Servant servant,
					 GNOME_UIHandler uih_containee,
					 CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));

	g_message ("Registering containee!\n");
	gnome_ui_handler_add_containee (uih, uih_containee);
}

static void
impl_GNOME_UIHandler_menu_item_activated (PortableServer_Servant servant,
					  const CORBA_char *path,
					  CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *item;

	/*
	 * Grab the internal data structure which represents the
	 * activated menu item.
	 */
	item = get_menu_item (uih, path);

	if (item == NULL) {
		g_warning ("impl_item_activated: Cannot find menu item with path [%s]\n",
			   path);
		return;
	}

	/*
	 * Now call the activation signal handler which will handle
	 * dispatching to the appropriate callback or containee.
	 */
	menu_item_activated (NULL, item);
}

static void
impl_GNOME_UIHandler_menu_item_create (PortableServer_Servant servant,
				       const CORBA_char *path,
				       const GNOME_UIHandler_MenuType menu_type,
				       const CORBA_char *label,
				       const CORBA_char *hint,
				       const GNOME_UIHandler_iobuf *pixmap_data,
				       const CORBA_unsigned_long accelerator_key,
				       const CORBA_long modifier,
				       const CORBA_long pos,
				       const GNOME_UIHandler containee_uih,
				       CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	GnomeUIHandlerMenuItem *item;
	MenuItemInternal *internal_data;

	g_message ("impl_GNOME_UIHandler_menu_item_create!\n");

	/*
	 * CORBA does not distinguish betwen a zero-length string and
	 * a NULL char pointer; everything is a zero-length string to
	 * it.  So I translate zero-length CORBA strings to NULL char
	 * pointers here.
	 */
#define UNCORBIFY_STRING(s) (strlen (s) == 0 ? NULL : s)

	item = make_menu_item (path, corba_to_menu_type (menu_type),
			       UNCORBIFY_STRING (label),
			       UNCORBIFY_STRING (hint),
			       0, /* FIXME: Dynlist */
			       GNOME_UI_HANDLER_PIXMAP_NONE, NULL,
			       (guint) accelerator_key, (GdkModifierType) modifier,
			       NULL, NULL, -1);

	internal_data = store_menu_item_data (uih, containee_uih, item);

	if (uih->container_uih) {
		menu_item_container_create (uih, item, pos);
	} else {
		char *parent_path;

		parent_path = get_parent_path (path);
		create_menu_item (uih, parent_path, internal_data, pos);
		g_free (parent_path);
	}
}

/**
 * gnome_ui_handler_menu_add_one:
 */
void
gnome_ui_handler_menu_add_one (GnomeUIHandler *uih, char *parent_path,
			       GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, -1, item);
}

/**
 * gnome_ui_handler_menu_add_one_pos:
 */
void
gnome_ui_handler_menu_add_one_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal_data;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * If the provided GnomeUIHandlerMenuItem does not have a path
	 * set, set one for it.
	 */
	do_menu_item_path (uih, parent_path, item);

	/*
	 * Duplicate the GnomeUIHandlerMenuItem structure and store
	 * it internally.
	 *
	 * FIXME: Should we check to make sure there isn't another
	 * menu item for this GNOME_UIHandler ?
	 */
	internal_data = store_menu_item_data (uih, gnome_object_corba_objref (GNOME_OBJECT (uih)), item);

	/*
	 * If we are a containee, then we propagate the menu item
	 * construction request to our container.
	 */
	if (uih->container_uih) {
		menu_item_container_create (uih, item, pos);
	} else {
		create_menu_item (uih, parent_path, internal_data, pos);
	}
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
 * gnome_ui_handler_menu_add_list_pos:
 */
void
gnome_ui_handler_menu_add_list_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; curr->type != GNOME_UI_HANDLER_MENU_END; curr ++, pos ++)
		gnome_ui_handler_menu_add_tree_pos (uih, parent_path, pos, curr);
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

	gnome_ui_handler_menu_add_tree_pos (uih, parent_path, -1, item);
}

/**
 * gnome_ui_handler_menu_add_tree_pos:
 */
void
gnome_ui_handler_menu_add_tree_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				    GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, pos, item);

	if (item->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || item->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		GnomeUIHandlerMenuItem *curr;

		for (curr = item->children; curr->type != GNOME_UI_HANDLER_MENU_END; curr ++)
			gnome_ui_handler_menu_add_tree (uih, item->path, curr);
	}
}

/**
 * gnome_ui_handler_menu_new_item:
 */
void
gnome_ui_handler_menu_new_item (GnomeUIHandler *uih, char *path,
				GnomeUIHandlerMenuItemType type,
				char *label, char *hint, gint dynlist_maxsize,
				GnomeUIHandlerPixmapType pixmap_type,
				gpointer pixmap_data,
				guint accelerator_key, GdkModifierType ac_mods,
				GnomeUIHandlerCallbackFunc callback,
				gpointer callback_data,
				int pos)
{
	GnomeUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	parent_path = get_parent_path (path);
	g_return_if_fail (parent_path != NULL);

	item = make_menu_item (path, type, label, hint, dynlist_maxsize,
			       pixmap_type, pixmap_data, accelerator_key, ac_mods,
			       callback, callback_data, pos);

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, pos, item);

	g_free (item);
	g_free (parent_path);
}

/**
 * gnome_ui_handler_menu_new_item_simple:
 */
void
gnome_ui_handler_menu_new_item_simple (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerMenuItemType type,
				       char *label, char *hint,
				       GnomeUIHandlerCallbackFunc callback,
				       gpointer callback_data)
{
	GnomeUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	parent_path = get_parent_path (path);
	g_return_if_fail (parent_path != NULL);

	item = g_new0 (GnomeUIHandlerMenuItem, 1);
	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->callback = callback;
	item->callback_data = callback_data;

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, -1, item);

	g_free (item);
	g_free (parent_path);
}

/*
 * This function removes a menu item from its parent's
 * child list.
 */
static void
remove_parent_menu_entry (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *parent;
	char *parent_path;
	GList *l;

	parent_path = get_parent_path (path);
	parent = get_menu_item (uih, parent_path);
	g_free (parent_path);

	l = g_list_find_custom (parent->children, path, g_str_equal);
	parent->children = g_list_remove_link (parent->children, l);
}

static void
free_menu_item_data (GnomeUIHandlerMenuItem *item)
{
	g_free (item->path);
	item->path = NULL;

	g_free (item->label);
	item->label = NULL;
	
	g_free (item->hint);
	item->hint = NULL;
	
	free_pixmap_data (item->pixmap_type, item->pixmap_data);
}

static void
free_menu_item (GnomeUIHandlerMenuItem *item)
{
	free_menu_item_data (item);
	g_free (item);
}

static gboolean
remove_menu_item_data (GnomeUIHandler *uih, MenuItemInternal *internal_item)
{
	gboolean head_removed;
	GList *l;
	char *path;

	g_return_val_if_fail (internal_item != NULL, FALSE);

	path = g_strdup (internal_item->item->path);

	/*
	 * Get the list of menu items which match this path.  There
	 * may be several menu items matching the same path because
	 * each containee can create a menu item with the same path.
	 * The newest item overrides the older ones.
	 */
	l = g_hash_table_lookup (uih->path_to_menu_item, path);

	if (internal_item == l->data)
		head_removed = TRUE;
	else
		head_removed = FALSE;

	/*
	 * Free the internal data structures associated with this
	 * item.
	 */
	free_menu_item (internal_item->item);
	g_free (internal_item);
	l = g_list_remove (l, internal_item);

	if (l != NULL) {
		g_hash_table_insert (uih->path_to_menu_item, path, l);
	} else {
		g_hash_table_remove (uih->path_to_menu_item, path);

		/*
		 * Remove this path entry from the parent's child
		 * list.
		 */
		remove_parent_menu_entry (uih, path);
	}

	g_free (path);

	return head_removed;
}

static void
remove_all_children (GnomeUIHandler *uih, MenuItemInternal *internal_item)
{
	GList *curr;

	for (curr = internal_item->children; curr != NULL; curr = curr->next) {
		gint child_num_instances, i;
		char *path = (char *) curr->data;

		child_num_instances =
			g_list_length (g_hash_table_lookup (uih->path_to_menu_item, path));

		for (i = 0; i < child_num_instances; i ++) {
			MenuItemInternal *child;

			child = get_menu_item (uih, path);
			remove_menu_item (uih, child, FALSE);
		}
	}
}

static void
remove_menu_item (GnomeUIHandler *uih, MenuItemInternal *internal_remove_me, gboolean replace)
{
	gboolean is_head;
	char *path;
	GList *l;

	path = g_strdup (internal_remove_me->item->path);

	/* Check if this is the currently active menu item. */
	l = g_hash_table_lookup (uih->path_to_menu_item, path);
	if (l->data == internal_remove_me)
		is_head = TRUE;
	else
		is_head = FALSE;

	/*
	 * If we are the top-level UIHandler (which is the same as
	 * having no container), then we have to remove the
	 * widgets associated with this menu item.
	 */
	if (uih->container_uih == NULL) {
		GtkWidget *menu_item;
		MenuItemInternal *internal;

		if (is_head) {
			/*
			 * Destroy the old menu item widget.
			 */
			menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
			g_return_if_fail (menu_item != NULL);

			gtk_widget_destroy (menu_item);

			g_hash_table_remove (uih->top->path_to_menu_widget, path);

			/*
			 * If this item is a subtree, remove its menu shell.
			 */
			g_hash_table_remove (uih->top->path_to_menu_shell, path);

			/*
			 * Create its replacement.
			 */
			if (replace) {
				internal = get_menu_item (uih, path);
				if (internal != NULL) {
					char *parent_path;
					guint keyval;

					/* FIXME: Fuck fuck fuck.  Bind the accelerators. */
					parent_path = get_parent_path (path);
					menu_item = create_gtk_menu_item (uih, parent_path, internal->item);
					g_free (parent_path);

					menu_item_connect_signal (menu_item, internal);
				}
			}
		}

	} else {

		/*
		 * If we are a containee, then we need to propagate
		 * the destruction to our container.  Then, if we have
		 * another menu item which has the same path, we need
		 * to make that menu item replace the deleted one.  So
		 * we tell our container to create the replacement.
		 */
		CORBA_Environment ev;
		GList *l;

		if (is_head) {
			/*
			 * Tell the container to destroy the menu item.
			 */
			CORBA_exception_init (&ev);
			GNOME_UIHandler_menu_item_remove (uih->container_uih, path, &ev);
			/* FIXME: Check the exception. */
			CORBA_exception_free (&ev);

			/*
			 * Now tell the container to create the replacement
			 * menu item, if one exists.  If one doesn't exist,
			 * then we are done.
			 */
			if (replace) {
				l = g_hash_table_lookup (uih->path_to_menu_item, path);
				if (l->data == NULL) {
					g_free (path);
					return;
				}

				menu_item_container_create (uih, (GnomeUIHandlerMenuItem *) l->data, -1);
			}
		}
	}

	/*
	 * If this item is an active subtree with no inactive
	 * replacement, we recursively remove all of its children.
	 */
	if (internal_remove_me->item->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || internal_remove_me->item->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		remove_all_children (uih, internal_remove_me);
	}

	/*
	 * Remove the internal data structures associated with this
	 * menu item.
	 */
	remove_menu_item_data (uih, internal_remove_me);

	g_free (path);
}

/**
 * gnome_ui_handler_menu_remove:
 */
void
gnome_ui_handler_menu_remove (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	internal = get_menu_item (uih, path);
	if (internal == NULL)
		return;

	remove_menu_item (uih, internal, TRUE);
}

/**
 * gnome_ui_handler_menu_set_info:
 */
void
gnome_ui_handler_menu_set_info (GnomeUIHandler *uih, char *path,
				GnomeUIHandlerMenuItem *info)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (info != NULL);

	gnome_ui_handler_menu_remove (uih, path);
	gnome_ui_handler_menu_add_one (uih, path, info);
}

static GnomeUIHandlerMenuItem *
get_container_menu_item (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *item;
	CORBA_Environment ev;

	CORBA_long corba_modifier_type;
	GNOME_UIHandler_MenuType corba_menu_type;
	GNOME_UIHandler_iobuf corba_pixmap_iobuf;
	CORBA_unsigned_long corba_accelerator_key;
	CORBA_long corba_pos;

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	CORBA_exception_init (&ev);

#if 0
	GNOME_UIHandler_menu_item_fetch (gnome_object_corba_objref (GNOME_OBJECT (uih)),
					 path, &corba_menu_type, &item->label,
					 &corba_pixmap_type, corba_pixmap_iobuf,
					 &corba_accelerator_key, &corba_modifier_type,
					 &corba_pos, &ev);
#endif

	item->path = g_strdup (path);
	item->type = corba_to_menu_type (corba_menu_type);
	item->pixmap_type = GNOME_UI_HANDLER_PIXMAP_DATA;
	item->pixmap_data = g_memdup (corba_pixmap_iobuf._buffer, corba_pixmap_iobuf._length);
	item->accelerator_key = corba_accelerator_key;
	item->ac_mods = (GdkModifierType) (corba_modifier_type);
	/* FIXME: pos? */

	return item;
}

/**
 * gnome_ui_handler_menu_fetch_one:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_fetch_one (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *remote_item;

	GnomeUIHandlerMenuItem *retval_item;

	MenuItemInternal *local_item;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	retval_item = NULL;

	/*
	 * Try to grab a local copy of the menu item
	 */
	local_item = get_menu_item (uih, path);

	/*
	 * If we are a containee, grab our container's representation
	 * of the object.
	 */
	remote_item = NULL;
	if (uih->container_uih != NULL) {

		remote_item = get_container_menu_item (uih, path);

		/*
		 * Update our local copy if appropriate.
		 */
		if (local_item != NULL) {
			free_menu_item (local_item->item);
			local_item->item = copy_menu_item (remote_item);
		}

	}

	/*
	 * Choose which data we will return.  The remote data always
	 * overrides the local data, since we presume that our
	 * container has the most up-to-date representation of the
	 * menu item in question.
	 */
	if (remote_item == NULL) {
		if (local_item == NULL)
			return NULL;
		retval_item = local_item->item;
	} else
		retval_item = remote_item;

	retval_item = copy_menu_item (retval_item);

	free_menu_item (remote_item);

	retval_item->children = NULL;

	return retval_item;
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

	free_menu_item (item);
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
		if ((curr->type == GNOME_UI_HANDLER_MENU_SUBTREE
		     || curr->type == GNOME_UI_HANDLER_MENU_RADIOGROUP)
		    && curr->children != NULL)
			gnome_ui_handler_menu_free_list (curr->children);

		free_menu_item_data (curr);
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

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		GnomeUIHandlerMenuItem *curr_child;

		gnome_ui_handler_menu_free_list (tree->children);
	}

	free_menu_item (tree);
}

typedef struct {
	GnomeUIHandlerCallbackFunc callback;
	GList *l;
} find_callback_closure_t;

static gboolean
find_callback (gpointer key, gpointer val, gpointer data)
{
	MenuItemInternal *item = (MenuItemInternal *) val;
	find_callback_closure_t *closure = (find_callback_closure_t *) data;

	if (item->item->callback == closure->callback)
		closure->l = g_list_prepend (closure->l, item->item);

	return FALSE;
}

GList *
gnome_ui_handler_menu_fetch_by_callback	(GnomeUIHandler *uih,
					 GnomeUIHandlerCallbackFunc callback)
{
	find_callback_closure_t *closure;
	GList *l;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (callback != NULL, NULL);

	closure = g_new0 (find_callback_closure_t, 1);
	closure->callback = callback;

	g_hash_table_foreach (uih->path_to_menu_item, find_callback, &l);

	l = closure->l;
	g_free (closure);

	return l;
}


typedef struct {
	gpointer data;
	GList *l;
} find_callback_data_closure_t;

static gboolean
find_callback_data (gpointer key, gpointer val, gpointer data)
{
	MenuItemInternal *item = (MenuItemInternal *) val;
	find_callback_data_closure_t *closure = (find_callback_data_closure_t *) data;

	if (item->item->callback_data == closure->data)
		closure->l = g_list_prepend (closure->l, item->item);

	return FALSE;
}

GList *
gnome_ui_handler_menu_fetch_by_callback_data (GnomeUIHandler *uih,
					      gpointer callback_data)
{
	find_callback_data_closure_t *closure;
	GList *l;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	closure = g_new0 (find_callback_data_closure_t, 1);
	closure->data = callback_data;

	g_hash_table_foreach (uih->path_to_menu_item, find_callback_data, &l);

	l = closure->l;
	g_free (closure);

	return l;
}

/**
 * gnome_ui_handler_menu_get_child_paths:
 */
GList *
gnome_ui_handler_menu_get_child_paths (GnomeUIHandler *uih, char *parent_path)
{
	MenuItemInternal *parent;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (parent_path != NULL, NULL);

	parent = get_menu_item (uih, parent_path);
	g_return_val_if_fail (parent != NULL, NULL);

	return g_list_copy (parent->children);
}

static GnomeUIHandlerMenuItemType
uiinfo_type_to_uih_type (GnomeUIInfoType uii_type)
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

static GnomeUIHandlerPixmapType
uiinfo_pixmap_type_to_uih_pixmap_type (GnomeUIPixmapType ui_type)
{
	switch (ui_type) {
	case GNOME_APP_PIXMAP_NONE:
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	case GNOME_APP_PIXMAP_STOCK:
		return GNOME_UI_HANDLER_PIXMAP_STOCK;
	case GNOME_APP_PIXMAP_DATA:
		return GNOME_UI_HANDLER_PIXMAP_DATA;
	case GNOME_APP_PIXMAP_FILENAME:
		return GNOME_UI_HANDLER_PIXMAP_FILENAME;
	default:
		g_warning ("Unknown GnomeUIPixmapType: %d\n", ui_type);
		return GNOME_UI_HANDLER_PIXMAP_NONE;
	}
}

static void
parse_menu_uiinfo_one (GnomeUIHandlerMenuItem *item, GnomeUIInfo *uii)
{
	item->path = NULL;
	item->type = uiinfo_type_to_uih_type (uii->type);

	if (uii->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uii);

	item->label = g_strdup (uii->label);
	item->hint = g_strdup (uii->hint);

	if (item->type == GNOME_UI_HANDLER_MENU_ITEM
	    || item->type == GNOME_UI_HANDLER_MENU_RADIOITEM
	    || item->type == GNOME_UI_HANDLER_MENU_TOGGLEITEM)
		item->callback = uii->moreinfo;
	item->callback_data = uii->user_data;

	item->pixmap_type = uiinfo_pixmap_type_to_uih_pixmap_type (uii->pixmap_type);
	item->pixmap_data = uii->pixmap_info;
	item->accelerator_key = uii->accelerator_key;
	item->ac_mods = uii->ac_mods;
}

static void
parse_menu_uiinfo_tree (GnomeUIHandlerMenuItem *tree, GnomeUIInfo *uii)
{
	parse_menu_uiinfo_one (tree, uii);

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
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

	parse_menu_uiinfo_one (item, uii);
	
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
	 * Allocate the GnomeUIHandlerMenuItem list.
	 */
	list_len = 0;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++)
		list_len ++;

	list = g_new0 (GnomeUIHandlerMenuItem, list_len + 1);

	curr_uih = list;
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++) {
		GnomeUIHandlerMenuItem *tree_item;

		parse_menu_uiinfo_tree (curr_uih, curr_uii);
	}

	/* Parse the terminal entry. */
	parse_menu_uiinfo_one (curr_uih, curr_uii);

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

	parse_menu_uiinfo_tree (item_tree, uii);

	return item_tree;
}


static void
parse_menu_uiinfo_one_with_data (GnomeUIHandlerMenuItem *item, GnomeUIInfo *uii, void *data)
{
	parse_menu_uiinfo_one (item, uii);

	item->callback_data = data;
}

static void
parse_menu_uiinfo_tree_with_data (GnomeUIHandlerMenuItem *tree, GnomeUIInfo *uii, void *data)
{
	parse_menu_uiinfo_one_with_data (tree, uii, data);

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
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

	parse_menu_uiinfo_one_with_data (item, uii, data);

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
	for (curr_uii = uii; curr_uii->type != GNOME_APP_UI_ENDOFINFO; curr_uii ++, curr_uih ++) {
		GnomeUIHandlerMenuItem *tree_item;

		parse_menu_uiinfo_tree_with_data (curr_uih, curr_uii, data);
	}

	/* Parse the terminal entry. */
	parse_menu_uiinfo_one (curr_uih, curr_uii);

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

	parse_menu_uiinfo_tree_with_data (item_tree, uii, data);

	return item_tree;
}

/**
 * gnome_ui_handler_menu_get_pos:
 */
gint
gnome_ui_handler_menu_get_pos (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);
}

void
gnome_ui_handler_menu_set_sensitivity (GnomeUIHandler *uih, char *path,
				       gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method along to the
	 * container.
	 *
	 * Otherwise, we fetch the widget and set its sensitivity
	 * ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		gtk_widget_set_sensitive (menu_item, sensitive);
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	/*
	 * FIXME: We need to maintain a GnomeUIHandlerMenuItem tree for all the widgets
	 * so we can figure this out without consulting widget->saved_state.
	 */
}

static void
set_menu_label (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal;
	GtkWidget *menu_item;
	GtkWidget *parent_shell;
	char *parent_path;

	menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, item->path);

	parent_path = get_parent_path (item->path);
	parent_shell = g_hash_table_lookup (uih->top->path_to_menu_shell, parent_path);
	g_free (parent_path);

	/*
	 * Remove the old label.
	 */
	remove_menu_label (uih, item, parent_shell, menu_item);

	/*
	 * Create the new one.
	 */
	create_menu_label (uih, item, parent_shell, menu_item);
}

void
gnome_ui_handler_menu_set_label (GnomeUIHandler *uih, char *path,
				 gchar *label_text)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * Update our internal data about this menu item, if
	 * appropriate.
	 */
	internal = get_menu_item (uih, path);
	if (internal != NULL) {
		g_free (internal->item->label);
		internal->item->label = g_strdup (label_text);
	}

	/*
	 * If we are a containee, then the actual GtkMenuItem widget 
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		set_menu_label (uih, internal->item);
	}
}

gchar *
gnome_ui_handler_menu_get_label (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *internal;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	/*
	 * Check to see if we have a local copy of this menu item's
	 * data.
	 */
	internal = get_menu_item (uih, path);

	/*
	 * If we are a containee, then there may be more up-to-date
	 * information about this menu item.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Grab container's information about this menu item. */
	}


	/*
	 * Otherwise, we have the most up-to-date copy of the information
	 * for this menu item.
	 */
	if (internal->item->label != NULL)
		return g_strdup (internal->item->label);
	else
		return NULL;
}

/**
 * gnome_ui_handler_menu_set_hint:
 */
void
gnome_ui_handler_menu_set_hint (GnomeUIHandler *uih, char *path, char *hint)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * Update our internal data about this menu item.
	 */
	internal = get_menu_item (uih, path);
	if (internal != NULL) {
		g_free (internal->item->hint);
		internal->item->hint = g_strdup (hint);
	}

	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		set_menu_hint (uih, internal->item, menu_item);
	}
}

/**
 * gnome_ui_handler_menu_get_hint:
 */
gchar *
gnome_ui_handler_menu_get_hint (GnomeUIHandler *uih, char *path)
{
}

void
gnome_ui_handler_menu_set_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (data != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;
		GtkPixmap *pixmap_widget;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		pixmap_widget = create_pixmap (type, data);
		gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_item),
						 GTK_WIDGET (pixmap_widget));
		/* FIXME: update internal state! */
	}
}

void
gnome_ui_handler_menu_get_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (type != NULL);
	g_return_if_fail (data != NULL);
}

void
gnome_ui_handler_menu_set_accel (GnomeUIHandler *uih, char *path,
				 guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;
		GtkAccelFlags accel_flags;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);


		/* FIXME: this deal with signals */
		accel_flags = 0;
		gtk_widget_add_accelerator (menu_item, "activate_item",
					    uih->top->accelgroup, accelerator_key,
					    ac_mods, accel_flags /* FIXME: ?? */);
					    
		/* FIXME: update state! */
	}
}

void
gnome_ui_handler_menu_get_accel (GnomeUIHandler *uih, char *path,
				 guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (accelerator_key != NULL);
	g_return_if_fail (ac_mods != NULL);

	
}

/**
 * gnome_ui_handler_menu_set_callback:
 */
void
gnome_ui_handler_menu_set_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc callback,
				    gpointer callback_data)
{
	MenuItemInternal *item;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	item = get_menu_item (uih, path);

	item->item->callback = callback;
	item->item->callback_data = callback_data;
}

/**
 * gnome_ui_handler_menu_get_callback:
 */
void
gnome_ui_handler_menu_get_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc *callback,
				    gpointer *callback_data)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (callback_data != NULL);

	internal = get_menu_item (uih, path);
	*callback = internal->item->callback;
	*callback_data = internal->item->callback_data;
}

void
gnome_ui_handler_menu_set_toggle_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), state);
					    
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_toggle_state (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

}

void
gnome_ui_handler_menu_set_radio_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);


	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), state);
					    
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_radio_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
}

/*
 * Dynamic menu lists.
 */
void
gnome_ui_handler_dynlist_set_maxsize (GnomeUIHandler *uih, char *path,
				      gint maxsize)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gint
gnome_ui_handler_dynlist_get_maxsize (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);
}

char *
gnome_ui_handler_dynlist_prepend_item (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *dynlist,
				       GnomeUIHandlerMenuItem *dynlist_item)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (dynlist != NULL, NULL);
	g_return_val_if_fail (dynlist_item != NULL, NULL);
}

char *
gnome_ui_handler_dynlist_append_item (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *dynlist,
				      GnomeUIHandlerMenuItem *dynlist_item)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (dynlist != NULL, NULL);
	g_return_val_if_fail (dynlist_item != NULL, NULL);
}

/*
 * The Toolbar manipulation routines.
 *
 * The name of the toolbar is the first element in the toolbar path.
 * For example:
 *		"Common/Save"
 *		"Graphics Tools/Erase"
 * 
 * Where both "Common" and "Graphics Tools" are the names of 
 * toolbars.
 * 
 */

void
gnome_ui_handler_set_toolbar (GnomeUIHandler *uih, char *name,
			      GtkWidget *toolbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
}

GList *
gnome_ui_handler_get_toolbar_list (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
}

void
gnome_ui_handler_toolbar_create (GnomeUIHandler *uih, char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
}

void
gnome_ui_handler_toolbar_add_one (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_one_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_list (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_list_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_tree (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_tree_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_remove (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_info (GnomeUIHandler *uih, char *path,
				   GnomeUIHandlerToolbarItem *info)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (info != NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_fetch (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback (GnomeUIHandler *uih,
					    GnomeUIHandlerCallbackFunc callback)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback_data (GnomeUIHandler *uih,
						 gpointer callback_data)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
}

gint
gnome_ui_handler_toolbar_get_pos (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, -1);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), -1);
	g_return_val_if_fail (path != NULL, -1);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_one (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (uii != NULL, NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_list (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (uii != NULL, NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_tree (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (uii != NULL, NULL);
}

void
gnome_ui_handler_toolbar_set_sensitivity (GnomeUIHandler *uih, char *path,
					  gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
}

void
gnome_ui_handler_toolbar_set_label (GnomeUIHandler *uih, char *path,
				    gchar *label)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gchar *
gnome_ui_handler_toolbar_get_label (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (path != NULL, NULL);
}

void
gnome_ui_handler_toolbar_set_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_accel (GnomeUIHandler *uih, char *path,
				    guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_accel (GnomeUIHandler *uih, char *path,
				    guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc callback,
				       gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc *callback,
				       gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_toggle_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL,FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
}


void
gnome_ui_handler_toolbar_toggle_set_state (GnomeUIHandler *uih, char *path,
					   gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_radio_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
}

void
gnome_ui_handler_toolbar_radio_set_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

				      
/*
 *
 * UI Groups
 *
 */

gint
gnome_ui_handler_group_create (void)
{
}

void
gnome_ui_handler_group_destroy (gint gid)
{
}

void
gnome_ui_handler_group_add_menu_items (gint gid, char *path1, ...)
{
}

void
gnome_ui_handler_group_add_toolbar_items (gint gid, char *path1, ...)
{
}

GList *
gnome_ui_handler_group_get_members (gint gid)
{
}

void
gnome_ui_handler_group_set_sensitivity (gint gid, gboolean sensitivity)
{

}
