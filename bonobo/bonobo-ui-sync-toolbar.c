/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * bonobo-ui-sync-toolbar.h: The Bonobo UI/XML sync engine for toolbars
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include <config.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-preferences.h>

#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-engine.h>
#include <bonobo/bonobo-ui-engine-config.h>
#include <bonobo/bonobo-ui-sync.h>
#include <bonobo/bonobo-ui-sync-toolbar.h>

#include <bonobo/bonobo-ui-toolbar.h>
#include <bonobo/bonobo-ui-toolbar-button-item.h>
#include <bonobo/bonobo-ui-toolbar-toggle-button-item.h>
#include <bonobo/bonobo-ui-toolbar-separator-item.h>
#include <bonobo/bonobo-ui-toolbar-popup-item.h>
#include <bonobo/bonobo-ui-toolbar-control-item.h>

static GtkObjectClass *parent_class = NULL;

#define PARENT_TYPE bonobo_ui_sync_get_type ()


static GdkPixbuf *
cmd_get_toolbar_pixbuf (BonoboUINode     *node,
			BonoboUINode     *cmd_node)
{
	GdkPixbuf *icon_pixbuf;
	char      *type;

	if ((type = bonobo_ui_node_get_attr (node, "pixtype"))) {
		icon_pixbuf = bonobo_ui_util_xml_get_icon_pixbuf (node, FALSE);
		bonobo_ui_node_free_string (type);
		return icon_pixbuf;
	}

	if ((type = bonobo_ui_node_get_attr (cmd_node, "pixtype"))) {
		icon_pixbuf = bonobo_ui_util_xml_get_icon_pixbuf (cmd_node, FALSE);
		bonobo_ui_node_free_string (type);
		return icon_pixbuf;
	}

	return NULL;
}

static BonoboUIToolbarControlDisplay
decode_control_disp (const char *txt)
{
	if (!txt || !strcmp (txt, "control"))
		return BONOBO_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL;

	else if (!strcmp (txt, "button"))
		return BONOBO_UI_TOOLBAR_CONTROL_DISPLAY_BUTTON;

	else if (!strcmp (txt, "none"))
		return BONOBO_UI_TOOLBAR_CONTROL_DISPLAY_NONE;

	else
		return BONOBO_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL;
}

static gboolean 
string_array_contains (char **str_array, const char *match)
{
	int i = 0;
	char *string;

	while ((string = str_array [i++]))
		if (strcmp (string, match) == 0)
			return TRUE;

	return FALSE;
}

static void
impl_bonobo_ui_sync_toolbar_state (BonoboUISync     *sync,
				   BonoboUINode     *node,
				   BonoboUINode     *cmd_node,
				   GtkWidget        *widget,
				   GtkWidget        *parent)
{
	char *type, *label, *txt;
	char *min_width;
	char *behavior;
	char **behavior_array;
	GdkPixbuf    *icon_pixbuf;

	/* FIXME: to debug control problem */
	gtk_widget_show (widget);

	if ((behavior = bonobo_ui_engine_get_attr (node, cmd_node, "behavior"))) {
		
		behavior_array = g_strsplit (behavior, ",", -1);
		bonobo_ui_node_free_string (behavior);

		bonobo_ui_toolbar_item_set_expandable (
			BONOBO_UI_TOOLBAR_ITEM (widget),
			string_array_contains (behavior_array, "expandable"));

		bonobo_ui_toolbar_item_set_pack_end (
			BONOBO_UI_TOOLBAR_ITEM (widget),
			string_array_contains (behavior_array, "pack-end"));

		g_strfreev (behavior_array);
	}

	icon_pixbuf = cmd_get_toolbar_pixbuf (node, cmd_node);

	type  = bonobo_ui_engine_get_attr (node, cmd_node, "type");
	label = bonobo_ui_engine_get_attr (node, cmd_node, "label");
	
	if (!type || !strcmp (type, "toggle")) {

		if (icon_pixbuf) {
			bonobo_ui_toolbar_button_item_set_icon (
				BONOBO_UI_TOOLBAR_BUTTON_ITEM (widget), icon_pixbuf);
			gdk_pixbuf_unref (icon_pixbuf);
		}

		if (label) {
			gboolean err;
			char *txt = bonobo_ui_util_decode_str (label, &err);
			if (err) {
				g_warning ("Encoding error in label on '%s', you probably forgot to "
					   "put an '_' before label in your xml file",
					   bonobo_ui_xml_make_path (node));
				return;
			}

			bonobo_ui_toolbar_button_item_set_label (
				BONOBO_UI_TOOLBAR_BUTTON_ITEM (widget), txt);

			g_free (txt);
		}
	}

	bonobo_ui_node_free_string (type);
	bonobo_ui_node_free_string (label);

	if (bonobo_ui_node_has_name (node, "control")) {
		char *txt;
		BonoboUIToolbarControlDisplay hdisp, vdisp;
		
		txt = bonobo_ui_node_get_attr (node, "hdisplay");
		hdisp = decode_control_disp (txt);
		bonobo_ui_node_free_string (txt);

		txt = bonobo_ui_node_get_attr (node, "vdisplay");
		vdisp = decode_control_disp (txt);
		bonobo_ui_node_free_string (txt);

		bonobo_ui_toolbar_control_item_set_display (
			BONOBO_UI_TOOLBAR_CONTROL_ITEM (widget), hdisp, vdisp);
	}

	if ((min_width = bonobo_ui_engine_get_attr (node, cmd_node, "min_width"))) {
		bonobo_ui_toolbar_item_set_minimum_width (BONOBO_UI_TOOLBAR_ITEM (widget),
							  atoi (min_width));
		bonobo_ui_node_free_string (min_width);
	}
	
	if ((txt = bonobo_ui_engine_get_attr (node, cmd_node, "tip"))) {
		gboolean err;
		char *decoded_txt;

		decoded_txt = bonobo_ui_util_decode_str (txt, &err);
		if (err) {
			g_warning ("Encoding error in tip on '%s', you probably forgot to "
				   "put an '_' before tip in your xml file",
				   bonobo_ui_xml_make_path (node));
		} else {
			bonobo_ui_toolbar_item_set_tooltip (
				BONOBO_UI_TOOLBAR_ITEM (widget),
				bonobo_ui_toolbar_get_tooltips (
					BONOBO_UI_TOOLBAR (parent)), decoded_txt);
		}

		g_free (decoded_txt);
		bonobo_ui_node_free_string (txt);
	}

	bonobo_ui_engine_queue_update (
		sync->engine, widget, node, cmd_node);
}

static gint
exec_verb_cb (GtkWidget *item, BonoboUIEngine *engine)
{
	bonobo_ui_engine_emit_verb_on_w (engine, GTK_WIDGET (item));

	return FALSE;
}

static gint
win_item_emit_ui_event (BonoboUIToolbarItem *item,
			const char          *state,
			BonoboUIEngine      *engine)
{
	BonoboUINode     *node = bonobo_ui_engine_widget_get_node (
		GTK_WIDGET (item));

	g_return_val_if_fail (node != NULL, FALSE);

	bonobo_ui_engine_emit_event_on (engine, node, state);

	return FALSE;
}


static GtkWidget *
toolbar_build_control (BonoboUISync     *sync,
		       BonoboUINode     *node,
		       BonoboUINode     *cmd_node,
		       int              *pos,
		       GtkWidget        *parent)
{
	GtkWidget  *item;
	
	g_return_val_if_fail (sync != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	if ((item = bonobo_ui_engine_node_get_widget (
		sync->engine, node))) {

		g_assert (item->parent == NULL);

	} else {
		Bonobo_Control control;

		control = bonobo_ui_engine_node_get_object (
			sync->engine, node);

		if (control != CORBA_OBJECT_NIL) {
			item = bonobo_ui_toolbar_control_item_new (
				bonobo_object_dup_ref (control, NULL));

			if (!item)
				return NULL;

			bonobo_ui_engine_stamp_custom (
				sync->engine, node);
		} else
			return NULL;
	}

	gtk_widget_show (item);

	bonobo_ui_toolbar_insert (BONOBO_UI_TOOLBAR (parent),
				  BONOBO_UI_TOOLBAR_ITEM (item),
				  (*pos)++);

	return item;
}

static GtkWidget *
toolbar_build_widget (BonoboUISync *sync,
		      BonoboUINode *node,
		      BonoboUINode *cmd_node,
		      int          *pos,
		      GtkWidget    *parent)
{
	char         *type;
	GtkWidget    *item;

	g_return_val_if_fail (sync != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	type = bonobo_ui_engine_get_attr (node, cmd_node, "type");

	if (bonobo_ui_node_has_name (node, "separator")) {
		item = bonobo_ui_toolbar_separator_item_new ();
		gtk_widget_set_sensitive (item, FALSE);

	} else if (!type)
		item = bonobo_ui_toolbar_button_item_new (NULL, NULL);
	
	else if (!strcmp (type, "toggle"))
		item = bonobo_ui_toolbar_toggle_button_item_new (NULL, NULL);
	
	else {
		/* FIXME: Implement radio-toolbars */
		g_warning ("Invalid type '%s'", type);
		return NULL;
	}

	bonobo_ui_node_free_string (type);
	
	bonobo_ui_toolbar_insert (BONOBO_UI_TOOLBAR (parent),
				  BONOBO_UI_TOOLBAR_ITEM (item),
				  (*pos)++);
	gtk_widget_show (item);

	return item;
}

static GtkWidget *
impl_bonobo_ui_sync_toolbar_build (BonoboUISync     *sync,
				   BonoboUINode     *node,
				   BonoboUINode     *cmd_node,
				   int              *pos,
				   GtkWidget        *parent)
{
	GtkWidget *widget;
	char      *verb;
	
	if (bonobo_ui_node_has_name (node, "control"))
		widget = toolbar_build_control (
			sync, node, cmd_node, pos, parent);
	else
		widget = toolbar_build_widget (
			sync, node, cmd_node, pos, parent);

	if (widget) {
		/* FIXME: What about "id"s ! ? */
		if ((verb = bonobo_ui_engine_get_attr (node, NULL, "verb"))) {
			gtk_signal_connect (GTK_OBJECT (widget), "activate",
					    (GtkSignalFunc) exec_verb_cb,
					    sync->engine);
			bonobo_ui_node_free_string (verb);
		}
		
		gtk_signal_connect (GTK_OBJECT (widget), "state_altered",
				    (GtkSignalFunc) win_item_emit_ui_event,
				    sync->engine);
	}

	return widget;
}

static GtkWidget *
impl_bonobo_ui_sync_toolbar_build_placeholder (BonoboUISync     *sync,
					       BonoboUINode     *node,
					       BonoboUINode     *cmd_node,
					       int              *pos,
					       GtkWidget        *parent)
{
	GtkWidget *widget;

	widget = bonobo_ui_toolbar_separator_item_new ();
	gtk_widget_set_sensitive (widget, FALSE);
	
	bonobo_ui_toolbar_insert (BONOBO_UI_TOOLBAR (parent),
				  BONOBO_UI_TOOLBAR_ITEM (widget),
				  (*pos)++);

	return widget;
}

static GnomeDockItem *
get_dock_item (BonoboUISyncToolbar *sync,
	       const char          *dockname)
{
	guint dummy;
	
	g_return_val_if_fail (dockname != NULL, NULL);

	return gnome_dock_get_item_by_name (sync->dock,
					    dockname,
					    &dummy, &dummy,
					    &dummy, &dummy);
}

static GList *
impl_bonobo_ui_sync_toolbar_get_widgets (BonoboUISync *sync,
					 BonoboUINode *node)
{
	char          *dockname;
	GnomeDockItem *item;

	dockname = bonobo_ui_node_get_attr (node, "name");
	item = get_dock_item (BONOBO_UI_SYNC_TOOLBAR (sync), dockname);
	bonobo_ui_node_free_string (dockname);

	if (!item) {
		g_warning ("Serious internal error building toolbar");
		return NULL;
	}

	return bonobo_ui_toolbar_get_children (
		BONOBO_UI_TOOLBAR (GTK_BIN (item)->child));
}

static void
impl_bonobo_ui_sync_toolbar_state_update (BonoboUISync *sync,
					  GtkWidget    *widget,
					  const char   *new_state)
{
	g_return_if_fail (widget != NULL);

	if (new_state) {
		if (BONOBO_IS_UI_TOOLBAR_ITEM (widget))
			bonobo_ui_toolbar_item_set_state (
				BONOBO_UI_TOOLBAR_ITEM (widget), new_state);
		
		else
			g_warning ("TESTME: strange, setting "
				   "state '%s' on weird object '%s'",
				   new_state, gtk_type_name (GTK_OBJECT (
					   widget)->klass->type));
	}
}

static void
impl_destroy (GtkObject *object)
{
	BonoboUISyncToolbar *sync;

	sync = BONOBO_UI_SYNC_TOOLBAR (object);

	parent_class->destroy (object);
}

static gboolean
impl_bonobo_ui_sync_toolbar_ignore_widget (BonoboUISync *sync,
					   GtkWidget    *widget)
{
	return BONOBO_IS_UI_TOOLBAR_POPUP_ITEM (widget);
}

static BonoboUIToolbarStyle
parse_look (const char *look)
{
	if (look) {
		if (!strcmp (look, "both"))
			return BONOBO_UI_TOOLBAR_STYLE_ICONS_AND_TEXT;

		if (!strcmp (look, "icon"))
			return BONOBO_UI_TOOLBAR_STYLE_ICONS_ONLY;

		if (!strcmp (look, "text"))
			return BONOBO_UI_TOOLBAR_STYLE_PRIORITY_TEXT;	
	}

	return gnome_preferences_get_toolbar_labels ()
		? BONOBO_UI_TOOLBAR_STYLE_ICONS_AND_TEXT
		: BONOBO_UI_TOOLBAR_STYLE_ICONS_ONLY;
}

static GnomeDockItem *
create_dockitem (BonoboUISyncToolbar *sync,
		 BonoboUINode        *node,
		 const char          *dockname)
{
	GnomeDockItem *item;
	GnomeDockItemBehavior beh = 0;
	char *prop;
	char **behavior_array;
	gboolean force_detachable = FALSE;
	GnomeDockPlacement placement = GNOME_DOCK_TOP;
	gint band_num = 1;
	gint position = 0;
	guint offset = 0;
	gboolean in_new_band = TRUE;

	if ((prop = bonobo_ui_node_get_attr (node, "behavior"))) {
		if (!strcmp (prop, "detachable"))
			force_detachable = TRUE;
		bonobo_ui_node_free_string (prop);
	}

	if ((prop = bonobo_ui_node_get_attr (node, "behavior"))) {
		behavior_array = g_strsplit (prop, ",", -1);
		bonobo_ui_node_free_string (prop);
	
		if (string_array_contains (behavior_array, "detachable"))
			force_detachable = TRUE;

		if (string_array_contains (behavior_array, "exclusive"))
			beh |= GNOME_DOCK_ITEM_BEH_EXCLUSIVE;

		if (string_array_contains (behavior_array, "never vertical"))
			beh |= GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL;

		if (string_array_contains (behavior_array, "never floating"))
			beh |= GNOME_DOCK_ITEM_BEH_NEVER_FLOATING;

		if (string_array_contains (behavior_array, "never horizontal"))
			beh |= GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL;

		g_strfreev (behavior_array);
	}

	if (!force_detachable && !gnome_preferences_get_toolbar_detachable())
		beh |= GNOME_DOCK_ITEM_BEH_LOCKED;

	item = GNOME_DOCK_ITEM (gnome_dock_item_new (
		dockname, beh));

	gtk_container_set_border_width (GTK_CONTAINER (item), 2);

	if ((prop = bonobo_ui_node_get_attr (node, "placement"))) {
		if (!strcmp (prop, "top"))
			placement = GNOME_DOCK_TOP;
		else if (!strcmp (prop, "right"))
			placement = GNOME_DOCK_RIGHT;
		else if (!strcmp (prop, "bottom"))
			placement = GNOME_DOCK_BOTTOM;
		else if (!strcmp (prop, "left"))
			placement = GNOME_DOCK_LEFT;
		else if (!strcmp (prop, "floating"))
			placement = GNOME_DOCK_FLOATING;
		bonobo_ui_node_free_string (prop);
	}

	if ((prop = bonobo_ui_node_get_attr (node, "band_num"))) {
		band_num = atoi (prop);
		bonobo_ui_node_free_string (prop);
	}

	if ((prop = bonobo_ui_node_get_attr (node, "position"))) {
		position = atoi (prop);
		bonobo_ui_node_free_string (prop);
	}

	if ((prop = bonobo_ui_node_get_attr (node, "offset"))) {
		offset = atoi (prop);
		bonobo_ui_node_free_string (prop);
	}

	if ((prop = bonobo_ui_node_get_attr (node, "in_new_band"))) {
		in_new_band = atoi (prop);
		bonobo_ui_node_free_string (prop);
	}	

	gnome_dock_add_item (sync->dock, item,
			     placement, band_num,
			     position, offset, in_new_band);

#ifdef CONFIG_TOOLBARS
	{
		char *path;
		static const char *popup_xml =
			"<popups>"
			"<popup>"
			"<menuitem name=\"hide\" _label=\"_Hide\" dep_attr=\"hidden:bool:0\"/>"
/* Drat we want to hide / show depending ... */
			"<menuitem name=\"show\" _label=\"_Show\" dep_attr=\"hidden:bool:1\"/>"
			"<menuitem name=\"Configure\" verb=\"\" _label=\"_Configure\"/>"
			"</popup>"
			"</popups>";
/*		static const char *unused = {
			N_("_Hide"),
			N_("_Show")
			};*/

		bonobo_ui_engine_config_connect (
			GTK_WIDGET (item), sync->parent.engine,
			(path = bonobo_ui_xml_make_path (node)),
			popup_xml);
		g_free (path);
	}
#endif

	return item;
}

static void
impl_bonobo_ui_sync_toolbar_remove_root (BonoboUISync *sync,
					 BonoboUINode *node)
{
	char *name = bonobo_ui_node_get_attr (node, "name");

	if (name) {
		GnomeDockItem *item;

		item = get_dock_item (BONOBO_UI_SYNC_TOOLBAR (sync), name);

		if (item)
			gtk_widget_destroy (GTK_WIDGET (item));
	}

	bonobo_ui_node_free_string (name);
}

static void
impl_bonobo_ui_sync_toolbar_update_root (BonoboUISync *sync,
					 BonoboUINode *node)
{
	char          *txt;
	char          *dockname = bonobo_ui_node_get_attr (node, "name");
	GnomeDockItem *item;
	BonoboUIToolbar *toolbar;
	BonoboUIToolbarStyle look;

	g_return_if_fail (dockname != NULL);

	item = get_dock_item (BONOBO_UI_SYNC_TOOLBAR (sync), dockname);
	
	if (!item) {
		item = create_dockitem (BONOBO_UI_SYNC_TOOLBAR (sync),
					node, dockname);
		
		toolbar = BONOBO_UI_TOOLBAR (bonobo_ui_toolbar_new ());

		gtk_container_add (GTK_CONTAINER (item),
				   GTK_WIDGET (toolbar));
		gtk_widget_show (GTK_WIDGET (toolbar));
	} else
		toolbar = BONOBO_UI_TOOLBAR (GTK_BIN (item)->child);

	bonobo_ui_engine_stamp_root (sync->engine, node, GTK_WIDGET (toolbar));

	if ((txt = bonobo_ui_node_get_attr (node, "look"))) {
		look = parse_look (txt);
		bonobo_ui_toolbar_set_hv_styles (toolbar, look, look);
		bonobo_ui_node_free_string (txt);

	} else {
		BonoboUIToolbarStyle vlook, hlook;

		txt = bonobo_ui_node_get_attr (node, "hlook");
		hlook = parse_look (txt);
		bonobo_ui_node_free_string (txt);

		txt = bonobo_ui_node_get_attr (node, "vlook");
		vlook = parse_look (txt);
		bonobo_ui_node_free_string (txt);

		bonobo_ui_toolbar_set_hv_styles (toolbar, hlook, vlook);
	}		

#if 0
	if ((txt = bonobo_ui_node_get_attr (node, "relief"))) {

		if (!strcmp (txt, "normal"))
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_NORMAL);

		else if (!strcmp (txt, "half"))
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_HALF);
		else
			bonobo_ui_toolbar_set_relief (
				toolbar, GTK_RELIEF_NONE);
		bonobo_ui_node_free_string (txt);
	}
#endif

#if 0
	tooltips = TRUE;
	if ((txt = bonobo_ui_node_get_attr (node, "tips"))) {
		tooltips = atoi (txt);
		bonobo_ui_node_free_string (txt);
	}
	
	bonobo_ui_toolbar_set_tooltips (toolbar, tooltips);
#endif

       /*
	* FIXME: It shouldn't be necessary to explicitly resize the
	* dock, since resizing a widget is supposed to resize it's parent,
	* but the dock is not resized correctly on dockitem show / hides.
	*/
	if (bonobo_ui_sync_do_show_hide (sync, node, NULL, GTK_WIDGET (item)))
		gtk_widget_queue_resize (GTK_WIDGET (
			BONOBO_UI_SYNC_TOOLBAR (sync)->dock));

	bonobo_ui_node_free_string (dockname);
}

static gboolean
impl_bonobo_ui_sync_toolbar_can_handle (BonoboUISync *sync,
					BonoboUINode *node)
{
	return bonobo_ui_node_has_name (node, "dockitem");
}

static void
class_init (BonoboUISyncClass *sync_class)
{
	GtkObjectClass *object_class;

	parent_class = gtk_type_class (BONOBO_TYPE_UI_SYNC);

	object_class = GTK_OBJECT_CLASS (sync_class);
	object_class->destroy  = impl_destroy;

	sync_class->sync_state = impl_bonobo_ui_sync_toolbar_state;
	sync_class->build      = impl_bonobo_ui_sync_toolbar_build;
	sync_class->build_placeholder = impl_bonobo_ui_sync_toolbar_build_placeholder;

	sync_class->get_widgets   = impl_bonobo_ui_sync_toolbar_get_widgets;
	sync_class->ignore_widget = impl_bonobo_ui_sync_toolbar_ignore_widget;
	sync_class->remove_root   = impl_bonobo_ui_sync_toolbar_remove_root;
	sync_class->update_root   = impl_bonobo_ui_sync_toolbar_update_root;

	sync_class->state_update  = impl_bonobo_ui_sync_toolbar_state_update;
	sync_class->can_handle    = impl_bonobo_ui_sync_toolbar_can_handle;
}

GtkType
bonobo_ui_sync_toolbar_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"BonoboUISyncToolbar",
			sizeof (BonoboUISyncToolbar),
			sizeof (BonoboUISyncToolbarClass),
			(GtkClassInitFunc)  class_init,
			(GtkObjectInitFunc) NULL,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (PARENT_TYPE, &info);
	}

	return type;
}

BonoboUISync *
bonobo_ui_sync_toolbar_new (BonoboUIEngine  *engine,
			    GnomeDock       *dock)
{
	BonoboUISyncToolbar *sync;

	g_return_val_if_fail (BONOBO_IS_UI_ENGINE (engine), NULL);

	sync = gtk_type_new (BONOBO_TYPE_UI_SYNC_TOOLBAR);

	sync->dock = dock;

	return bonobo_ui_sync_construct (
		BONOBO_UI_SYNC (sync), engine, FALSE, TRUE);
}