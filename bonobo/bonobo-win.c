#include "config.h"
#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

#include "Bonobo.h"
#include "bonobo-ui-xml.h"
#include "bonobo-ui-util.h"
#include "bonobo-app.h"
#include "bonobo-app-item.h"
#include "bonobo-app-toolbar.h"

GtkObjectClass *bonobo_app_parent_class = NULL;

#define BONOBO_APP_WINDOW_KEY "Bonobo::BonoboApp"

#define	BINDING_MOD_MASK()				\
	(gtk_accelerator_get_default_mod_mask () | GDK_RELEASE_MASK)

POA_Bonobo_UIContainer__vepv bonobo_app_vepv;

#define XML_FREE(a) (a?xmlFree(a):a)

struct _BonoboAppPrivate {
	GtkWidget     *window;
	GnomeDock     *dock;

	GnomeDockItem *menu_item;
	GtkMenuBar    *menu;

	BonoboUIXml   *tree;

	GtkAccelGroup *accel_group;

	char          *name;		/* App name */
	char          *prefix;		/* App prefix */

	GHashTable    *radio_groups;
	GHashTable    *keybindings;
	GSList        *components;

	GtkWidget     *main_vbox;

	GtkBox        *status;
	GtkStatusbar  *main_status;

	GtkWidget     *client_area;
};

static inline BonoboApp *
bonobo_app_from_servant (PortableServer_Servant servant)
{
	return BONOBO_APP (bonobo_object_from_servant (servant));
}

typedef struct {
	BonoboUIXmlData parent;

	GtkWidget      *widget;
	Bonobo_Unknown  object;
} NodeInfo;

static void
info_dump_fn (BonoboUIXmlData *a)
{
	fprintf (stderr, " '%s' widget %p\n",
		 (char *)a->id, ((NodeInfo *)a)->widget);
}

static BonoboUIXmlData *
info_new_fn (void)
{
	NodeInfo *info = g_new0 (NodeInfo, 1);

	info->object = CORBA_OBJECT_NIL;

	return (BonoboUIXmlData *) info;
}

static void
info_free_fn (BonoboUIXmlData *data)
{
	NodeInfo *info = (NodeInfo *) data;

	if (info->object != CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (info->object, NULL);
		info->object = CORBA_OBJECT_NIL;
	}
	info->widget = NULL;

	g_free (data);
}

/*
 *   Placeholders have no widget, so we have to go above them
 * to fetch the real parent.
 */
static GtkWidget *
node_get_parent_widget (BonoboUIXml *tree, xmlNode *node)
{
	NodeInfo *info;

	if (!node)
		return NULL;

	do {
		info = bonobo_ui_xml_get_data (tree, node->parent);
		if (info->widget)
			return info->widget;
	} while ((node = node->parent));

	return NULL;
}

static char *
node_get_id (xmlNode *node)
{
	xmlChar *txt;
	char    *ret;

	g_return_val_if_fail (node != NULL, NULL);

	if (!(txt = xmlGetProp (node, "id")))
		txt = xmlGetProp (node, "verb");

	if (txt) {
		ret = g_strdup (txt);
		xmlFree (txt);
	} else
		ret = NULL;

	return ret;
}

static char *
node_get_id_or_path (xmlNode *node)
{
	xmlChar *txt;

	g_return_val_if_fail (node != NULL, NULL);

	if ((txt = node_get_id (node))) {
		char *ret;
		ret = g_strdup (txt);
		xmlFree (txt);
		return ret;
	}

	return bonobo_ui_xml_make_path (node);
}

/*
 *  This indirection is needed so we can serialize user settings
 * on a per component basis in future.
 */
typedef struct {
	char          *name;
	Bonobo_Unknown object;
} AppComponent;

static AppComponent *
app_component_get (BonoboAppPrivate *priv, const char *name)
{
	AppComponent *component;
	GSList       *l;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (priv != NULL, NULL);

	for (l = priv->components; l; l = l->next) {
		component = l->data;
		
		if (!strcmp (component->name, name))
			return component;
	}
	
	component = g_new (AppComponent, 1);
	component->name = g_strdup (name);
	component->object = CORBA_OBJECT_NIL;

	priv->components = g_slist_prepend (priv->components, component);

	return component;
}

static Bonobo_Unknown
app_component_objref (BonoboAppPrivate *priv, const char *name)
{
	AppComponent *component = app_component_get (priv, name);

	g_return_val_if_fail (component != NULL, CORBA_OBJECT_NIL);

	return component->object;
}

static void
app_component_destroy (BonoboAppPrivate *priv, AppComponent *component)
{
	priv->components = g_slist_remove (priv->components, component);

	if (component) {
		g_free (component->name);
		if (component->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (component->object, NULL);
		g_free (component);
	}
}

static void
impl_register_component (PortableServer_Servant   servant,
			 const CORBA_char        *component_name,
			 const Bonobo_Unknown     object,
			 CORBA_Environment       *ev)
{
	AppComponent *component;
	BonoboApp    *app = bonobo_app_from_servant (servant);

	if ((component = app_component_get (app->priv, component_name))) {
		if (component->object != CORBA_OBJECT_NIL)
			bonobo_object_release_unref (component->object, NULL);
	}

	component->object = bonobo_object_dup_ref (object, NULL);
}

static void
impl_deregister_component (PortableServer_Servant servant,
			   const CORBA_char      *component_name,
			   CORBA_Environment     *ev)
{
	AppComponent *component;
	BonoboApp    *app = bonobo_app_from_servant (servant);

	if ((component = app_component_get (app->priv, component_name))) {
		bonobo_app_xml_rm (app, "/", component->name);
		app_component_destroy (app->priv, component);
	} else
		g_warning ("Attempting to deregister non-registered "
			   "component '%s'", component_name);
}

static xmlNode *
get_cmd_state (BonoboAppPrivate *priv, const char *cmd_name)
{
	char    *path;
	xmlNode *ret;

	g_return_val_if_fail (priv != NULL, NULL);

	if (!cmd_name)
		return NULL;

	path = g_strconcat ("/commands/cmd/#", cmd_name, NULL);
	ret  = bonobo_ui_xml_get_path (priv->tree, path);
	g_free (path);

	return ret;
}

static void
set_cmd_dirty (BonoboAppPrivate *priv, xmlNode *cmd_node)
{
	char *cmd_name;
	xmlNode *node;
	BonoboUIXmlData *data;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (cmd_node != NULL);

	if (!(cmd_name = node_get_id (cmd_node)))
		return;

	node = get_cmd_state (priv, cmd_name);

	data = bonobo_ui_xml_get_data (priv->tree, node);
	data->dirty = TRUE;

	g_free (cmd_name);
}

static void
widget_set_state (GtkWidget *widget, xmlNode *node)
{
	char *txt;

	if ((txt = xmlGetProp (node, "sensitive"))) {
		gtk_widget_set_sensitive (widget, atoi (txt));
		xmlFree (txt);
	}

	if ((txt = xmlGetProp (node, "state"))) {

		if (BONOBO_IS_APP_ITEM (widget))
			bonobo_app_item_set_state (
				BONOBO_APP_ITEM (widget), txt);

		else if (GTK_IS_CHECK_MENU_ITEM (widget))
			gtk_check_menu_item_set_active (
				GTK_CHECK_MENU_ITEM (widget), 
				atoi (txt));
		else
			g_warning ("TESTME: strange, setting "
				   "state '%s' on wierd object", txt);
		xmlFree (txt);
	}
}

static void
update_cmd_state (BonoboAppPrivate *priv, xmlNode *search,
		  xmlNode *state, const char *search_id)
{
	xmlNode    *l;
	char *id = node_get_id (search);

	g_return_if_fail (search_id != NULL);

	if (id && !strcmp (search_id, id)) { /* Sync its state */
		NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, search);

		if (info->widget)
			widget_set_state (info->widget, state);
	}

	for (l = search->childs; l; l = l->next)
		update_cmd_state (priv, l, state, search_id);

	g_free (id);
}

static void
update_commands_state (BonoboAppPrivate *priv)
{
	xmlNode *cmds, *l;

	cmds = bonobo_ui_xml_get_path (priv->tree, "/commands");

	if (!cmds)
		return;

	for (l = cmds->childs; l; l = l->next) {
		BonoboUIXmlData *data = bonobo_ui_xml_get_data (priv->tree, l);
		char *cmd_name;

		cmd_name = xmlGetProp (l, "name");
		if (!cmd_name)
			g_warning ("Internal error; cmd with no id");

		else if (data->dirty)
			update_cmd_state (priv, priv->tree->root, l, cmd_name);

		data->dirty = FALSE;
		XML_FREE (cmd_name);
	}
}

/**
 * set_cmd_state:
 * @priv: environment data
 * @cmd_node: the node containing either the command name or a plain boring widget
 * @prop: the name of the property to set
 * @value: the value of the property
 * @immediate_update: whether to immediately re-sync all dependant nodes.
 * 
 *  Set the state of the XML node representing this command.
 **/
static void
set_cmd_state (BonoboAppPrivate *priv, xmlNode *cmd_node, const char *prop,
	       const char *value, gboolean immediate_update)
{
	xmlNode *node;
	char    *cmd_name;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (prop != NULL);
	g_return_if_fail (value != NULL);
	g_return_if_fail (cmd_node != NULL);

	if (!(cmd_name = node_get_id (cmd_node))) { /* A non cmd widget */
		NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, cmd_node);

		widget_set_state (info->widget, cmd_node);
		xmlSetProp (cmd_node, prop, value);
		return;
	}

	node = get_cmd_state (priv, cmd_name);

	xmlSetProp (node, prop, value);

	if (immediate_update)
		update_cmd_state (priv, priv->tree->root, node, cmd_name);
	else {
		BonoboUIXmlData *data =
			bonobo_ui_xml_get_data (priv->tree, node);

		data->dirty = TRUE;
	}

	g_free (cmd_name);
}

static void
real_emit_ui_event (BonoboAppPrivate *priv, const char *component_name,
		    const char *id, int type, const char *new_state)
{
	Bonobo_UIComponent component;

	g_return_if_fail (id != NULL);
	g_return_if_fail (new_state != NULL);

	if (!component_name) /* Auto-created entry, no-one can listen to it */
		return;

	component = app_component_objref (priv, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_ui_event (
			component, id, type, new_state, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			/* FIXME: so if it is a sys exception do we de-merge ? */
			g_warning ("Exception emitting state change to %d '%s' '%s'",
				   type, id, new_state);
		}
		
		CORBA_exception_free (&ev);
	} else
		g_warning ("NULL Corba handle of name '%s'", component_name);
}

static void
override_fn (GtkObject *object, xmlNode *node, BonoboAppPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	/* To stop stale pointers floating in the overrides */
	info->widget = NULL;

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_OVERRIDDEN, "");

/*	fprintf (stderr, "XOverride '%s'\n", id);*/

	g_free (id);
}

static void
reinstate_fn (GtkObject *object, xmlNode *node, BonoboAppPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	g_assert (info->widget == NULL);

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_REINSTATED, "");

/*	fprintf (stderr, "XReinstate '%s'\n", id);*/

	g_free (id);
}

static void
remove_fn (GtkObject *object, xmlNode *node, BonoboAppPrivate *priv)
{
	char     *id = node_get_id_or_path (node);
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	real_emit_ui_event (priv, info->parent.id, id,
			    Bonobo_UIComponent_REMOVED, "");

	if (info->widget)
		gtk_widget_destroy (info->widget);
	info->widget = NULL;

/*	fprintf (stderr, "XRemove '%s'\n", id);*/

	g_free (id);
}

/*
 * Doesn't the GtkRadioMenuItem API suck badly !
 */
#define MAGIC_RADIO_GROUP_KEY "Bonobo::RadioGroupName"

static void
radio_group_remove (GtkRadioMenuItem *menuitem,
		    char             *group_name)
{
	GtkRadioMenuItem *master;
	char             *orig_key;
	GSList           *l;
	BonoboAppPrivate *priv =
		gtk_object_get_data (GTK_OBJECT (menuitem),
				     MAGIC_RADIO_GROUP_KEY);

	if (!g_hash_table_lookup_extended
	    (priv->radio_groups, group_name, (gpointer *)&orig_key,
	     (gpointer *)&master)) {
		g_warning ("Radio group hash inconsistancy");
		return;
	}
	
	l = master->group;
	while (l && l->data == menuitem)
		l = l->next;
	
	g_hash_table_remove (priv->radio_groups, group_name);
	g_free (orig_key);

	if (l) { /* Entries left in group */
		g_hash_table_insert (priv->radio_groups,
				     group_name, l->data);
	} else /* alloced in signal_connect; grim hey */
		g_free (group_name);
}

static void
radio_group_add (BonoboAppPrivate *priv,
		 GtkRadioMenuItem *menuitem,
		 const char       *group_name)
{
	GtkRadioMenuItem *master;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (menuitem != NULL);
	g_return_if_fail (group_name != NULL);

	if (!(master = g_hash_table_lookup (priv->radio_groups, group_name)))
		g_hash_table_insert (priv->radio_groups, g_strdup (group_name),
				     menuitem);
	else
		gtk_radio_menu_item_set_group (
			menuitem, gtk_radio_menu_item_group (master));

	gtk_object_set_data (GTK_OBJECT (menuitem),
			     MAGIC_RADIO_GROUP_KEY, priv);

	gtk_signal_connect (GTK_OBJECT (menuitem), "destroy",
			    (GtkSignalFunc) radio_group_remove,
			    g_strdup (group_name));
}

/**
 *  The widget / container sweep is neccessary since info->widget is a 'submenu' for
 * menu construction, and yet the GtkMenuItem needs to be destroyed which is
 * stored in the container.
 **/
static void
container_destroy_siblings (BonoboUIXml *tree, GtkWidget *widget, xmlNode *node)
{
	xmlNode *l;

	for (l = node; l; l = l->next) {
		NodeInfo *info;

		info = bonobo_ui_xml_get_data (tree, node);

		container_destroy_siblings (tree, info->widget, node->childs);

		if (info->widget)
			gtk_widget_destroy (info->widget);
		/* else freshly merged and no widget yet */
		info->widget = NULL;
	}

	if (GTK_IS_CONTAINER (widget))
		gtk_container_foreach (GTK_CONTAINER (widget),
				       (GtkCallback) gtk_widget_destroy,
				       NULL);
}

#define BONOBO_APP_PRIV_KEY "BonoboApp::Priv"

static void
real_exec_verb (BonoboAppPrivate *priv,
		const char       *component_name,
		const char       *verb)
{
	Bonobo_UIComponent component;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (verb != NULL);
	g_return_if_fail (component_name != NULL);

	component = app_component_objref (priv, component_name);

	if (component != CORBA_OBJECT_NIL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);

		Bonobo_UIComponent_exec_verb (
			component, verb, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			/* FIXME: so if it is a sys exception do we de-merge ? */
			g_warning ("Exception executing verb '%s' on '%s'",
				   verb, component_name);
		}
		
		CORBA_exception_free (&ev);
	} else
		g_warning ("NULL Corba handle of name '%s'", component_name);
}

static gint
exec_verb_cb (GtkWidget *item, xmlNode *node)
{
	CORBA_char        *verb;
	BonoboUIXmlData   *data;
	BonoboAppPrivate  *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_APP_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	if (!data->id)
		return FALSE;

	verb = xmlGetProp (node, "verb");
	if (!verb) {
		g_warning ("No verb on '%s' '%s'",
			   node->name, xmlGetProp (node, "name"));
		return FALSE;
	}

	real_exec_verb (priv, data->id, verb);

	xmlFree (verb);

	return FALSE;
}

static gint
menu_toggle_emit_ui_event (GtkCheckMenuItem *item, xmlNode *node)
{
	char             *id, *state;
	BonoboUIXmlData  *data;
	BonoboAppPrivate *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_APP_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	if (item->active)
		state = "1";
	else
		state = "0";
	
	if ((id = node_get_id (node))) {
		set_cmd_state (priv, node, "state", state, TRUE);
	} else {
		id = bonobo_ui_xml_make_path (node);
		xmlSetProp (node, "state", state);
	}

	real_emit_ui_event (priv, data->id, id,
			    Bonobo_UIComponent_STATE_CHANGED,
			    state);

	g_free (id);

	return FALSE;
}

static gint
app_item_emit_ui_event (BonoboAppItem *item, const char *state, xmlNode *node)
{
	char             *id;
	BonoboUIXmlData  *data;
	BonoboAppPrivate *priv;

	g_return_val_if_fail (node != NULL, FALSE);

	priv = gtk_object_get_data (GTK_OBJECT (item), BONOBO_APP_PRIV_KEY);
	g_return_val_if_fail (priv != NULL, FALSE);

	data = bonobo_ui_xml_get_data (NULL, node);
	g_return_val_if_fail (data != NULL, FALSE);

	if ((id = node_get_id (node))) {
		set_cmd_state (priv, node, "state", state, TRUE);
	} else {
		id = bonobo_ui_xml_make_path (node);
		xmlSetProp (node, "state", state);
	}

	real_emit_ui_event (priv, data->id, id,
			    Bonobo_UIComponent_STATE_CHANGED,
			    state);
	g_free (id);

	return FALSE;
}

#define BONOBO_APP_MENU_ITEM_KEY "BonoboApp::Priv"

static void
put_hint_in_statusbar (GtkWidget *menuitem, xmlNode *node)
{
	BonoboAppPrivate *priv;
	char *hint;

	priv = gtk_object_get_data (GTK_OBJECT (menuitem),
				    BONOBO_APP_MENU_ITEM_KEY);

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	hint = xmlGetProp (node, "descr");
	g_return_if_fail (hint != NULL);

	if (priv->main_status) {
		guint id;

		id = gtk_statusbar_get_context_id (priv->main_status,
						  "BonoboApp:menu-hint");
		gtk_statusbar_push (priv->main_status, id, hint);
	}
	xmlFree (hint);
}

static void
remove_hint_from_statusbar (GtkWidget *menuitem, xmlNode *node)
{
	BonoboAppPrivate *priv;

	priv = gtk_object_get_data (GTK_OBJECT (menuitem),
				    BONOBO_APP_MENU_ITEM_KEY);

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	if (priv->main_status) {
		guint id;

		id = gtk_statusbar_get_context_id (priv->main_status,
						  "BonoboApp:menu-hint");
		gtk_statusbar_pop (priv->main_status, id);
	}
}

static GtkWidget *
menu_item_create (BonoboAppPrivate *priv, xmlNode *node)
{
	GtkWidget *menu_widget;
	NodeInfo  *info;
	char      *type;
	char      *hint;

	info = bonobo_ui_xml_get_data (priv->tree, node);

	/* Create menu item */
	if ((type = xmlGetProp (node, "type"))) {
		char *state;
		
		if (!strcmp (type, "radio")) {
			char *group = xmlGetProp (node, "group");

			menu_widget = gtk_radio_menu_item_new (NULL);

			if (group)
				radio_group_add (
					priv,
					GTK_RADIO_MENU_ITEM (menu_widget),
					group);

			g_free (group);
		} else if (!strcmp (type, "toggle"))
			menu_widget = gtk_check_menu_item_new ();

		else {
			g_warning ("Unhandled type of menu '%s'", type);
			return NULL;
		}

		info->widget = menu_widget;
			
		gtk_check_menu_item_set_show_toggle (
			GTK_CHECK_MENU_ITEM (menu_widget), TRUE);

		if (!(state = xmlGetProp (node, "state")))
			state = "0";
		set_cmd_state (priv, node, "state", state, FALSE);
		
		gtk_object_set_data (GTK_OBJECT (menu_widget), BONOBO_APP_PRIV_KEY, priv);
		gtk_signal_connect (GTK_OBJECT (menu_widget), "toggled",
				    (GtkSignalFunc) menu_toggle_emit_ui_event,
				    node);

		xmlFree (type);
	} else {
		char *txt;

		if ((txt = xmlGetProp (node, "pixtype"))) {
			GtkWidget *pixmap;

			menu_widget = gtk_pixmap_menu_item_new ();

			pixmap = bonobo_ui_util_xml_get_pixmap (menu_widget, node);
			
			if (pixmap) {
				gtk_widget_show (GTK_WIDGET (pixmap));
				gtk_pixmap_menu_item_set_pixmap (
					GTK_PIXMAP_MENU_ITEM (menu_widget),
					GTK_WIDGET (pixmap));
			}
			xmlFree (txt);
		} else
			menu_widget = gtk_menu_item_new ();

		info->widget = menu_widget;
	}

	if ((hint = xmlGetProp (node, "descr"))) {
		gtk_object_set_data (GTK_OBJECT (menu_widget),
				     BONOBO_APP_MENU_ITEM_KEY, priv);

		gtk_signal_connect (GTK_OBJECT (menu_widget),
				    "select",
				    GTK_SIGNAL_FUNC (put_hint_in_statusbar),
				    node);
		
		gtk_signal_connect (GTK_OBJECT (menu_widget),
				    "deselect",
				    GTK_SIGNAL_FUNC (remove_hint_from_statusbar),
				    node);
	}

	return menu_widget;
}

static void
menu_item_set_label (BonoboAppPrivate *priv, xmlNode *node,
		     GtkWidget *parent, GtkWidget *menu_widget)
{
	char *label_text;

	if ((label_text = xmlGetProp (node, "label"))) {
		GtkWidget *label;
		guint      keyval;

		label = gtk_accel_label_new (label_text);

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
		gtk_accel_label_set_accel_widget (
			GTK_ACCEL_LABEL (label), menu_widget);
	
		keyval = gtk_label_parse_uline (GTK_LABEL (label), label_text);

		if (keyval != GDK_VoidSymbol) {
			if (GTK_IS_MENU (parent))
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					gtk_menu_ensure_uline_accel_group (
						GTK_MENU (parent)),
					keyval, 0, 0);

			else if (GTK_IS_MENU_BAR (parent) &&
				 priv->accel_group != NULL)
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					priv->accel_group,
					keyval, GDK_MOD1_MASK, 0);
			else
				g_warning ("Adding accelerator went bananas");
		}
		xmlFree (label_text);
	}
}

static void
menu_item_set_global_accels (BonoboAppPrivate *priv, xmlNode *node,
			     GtkWidget *menu_widget)
{
	char *text;

	if ((text = xmlGetProp (node, "accel"))) {
		guint           key;
		GdkModifierType mods;

		gtk_accelerator_parse (text, &key, &mods);
		xmlFree (text);

		if (!key)
			return;

		gtk_widget_add_accelerator (menu_widget,
					    "activate",
					    priv->accel_group,
					    key, mods,
					    GTK_ACCEL_VISIBLE);
	}
}

/*
 * Insert slightly cleverly.
 *  NB. it is no use inserting into the default placeholder here
 *  since this will screw up path addressing and subsequent merging.
 */
static void
add_node_fn (xmlNode *parent, xmlNode *child)
{
	xmlNode *insert = parent;
	char    *pos;
/*	xmlNode *l;
	if (!xmlGetProp (child, "noplace"))
		for (l = parent->childs; l; l = l->next) {
			if (!strcmp (l->name, "placeholder") &&
			    !xmlGetProp (l, "name")) {
			    insert = l;
				g_warning ("Found default placeholder");
			}
		}*/

	if (insert->childs &&
	    (pos = xmlGetProp (child, "pos"))) {
		if (!strcmp (pos, "top")) {
			g_warning ("TESTME: unused code branch");
			xmlAddPrevSibling (insert->childs, child);
		} else /* FIXME: we could have 'middle'; is it useful ? */
			xmlAddChild (insert, child);
		xmlFree (pos);
	} else /* just add to bottom */
		xmlAddChild (insert, child);
}

static void build_menu_widget (BonoboAppPrivate *priv, xmlNode *node);

static void
build_menu_placeholder (BonoboAppPrivate *priv, xmlNode *node, GtkWidget *parent)
{
	xmlNode   *l;
	char      *delimit;
	gboolean   top = FALSE, bottom = FALSE;
	GtkWidget *sep;

	if ((delimit = xmlGetProp (node, "delimit"))) {
		if (!strcmp (delimit, "top") ||
		    !strcmp (delimit, "both"))
			top = (node->childs != NULL) && (node->prev != NULL);

		if (!strcmp (delimit, "bottom") ||
		    !strcmp (delimit, "both"))
			bottom = (node->childs != NULL) && (node->next != NULL);
		xmlFree (delimit);
	}

	if (top) {
		sep = gtk_menu_item_new ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		gtk_menu_shell_append (GTK_MENU_SHELL (parent), sep);
	}

/*	g_warning ("Building placeholder %d %d %p %p %p", top, bottom,
	node->childs, node->prev, node->next);*/
		
	for (l = node->childs; l; l = l->next)
		build_menu_widget (priv, l);

	if (bottom) {
		sep = gtk_menu_item_new ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		gtk_menu_shell_append (GTK_MENU_SHELL (parent), sep);
	}
}

static void
build_menu_widget (BonoboAppPrivate *priv, xmlNode *node)
{
	NodeInfo  *info;
	GtkWidget *parent, *menu_widget;
	char      *verb, *sensitive;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->name != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_parent_widget (priv->tree, node);

	if (!strcmp (node->name, "placeholder")) {
		build_menu_placeholder (priv, node, parent);
		return;
	}

	menu_widget = menu_item_create (priv, node);
	if (!menu_widget)
		return;

	menu_item_set_label (priv, node, parent, menu_widget);

	menu_item_set_global_accels (priv, node, menu_widget);

	if (!strcmp (node->name, "submenu")) {
		xmlNode      *l;
		GtkMenuShell *shell;
		GtkMenu      *menu;
		GtkWidget    *tearoff;

		if (parent == NULL)
			shell = GTK_MENU_SHELL (priv->menu);
		else
			shell = GTK_MENU_SHELL (parent);

		/* Create the menu shell. */
		menu = GTK_MENU (gtk_menu_new ());

		gtk_menu_set_accel_group (menu, priv->accel_group);

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
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget),
					   GTK_WIDGET (menu));

		info->widget = GTK_WIDGET (menu);

		gtk_menu_shell_append (shell, menu_widget);
		
		for (l = node->childs; l; l = l->next)
			build_menu_widget (priv, l);

		gtk_widget_show (GTK_WIDGET (menu));
		gtk_widget_show (GTK_WIDGET (shell));

	} else if (!strcmp (node->name, "menuitem")) {
		g_return_if_fail (parent != NULL);

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else if (!strcmp (node->name, "control")) {
		g_return_if_fail (info->object != CORBA_OBJECT_NIL);

		menu_widget = bonobo_app_item_new_control (info->object);
		g_return_if_fail (menu_widget != NULL);

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else {
		g_warning ("Unknown name type '%s'", node->name);
		return;
	}

	if ((sensitive = xmlGetProp (node, "sensitive"))) {
		set_cmd_state (priv, node, "sensitive", sensitive, FALSE);
		xmlFree (sensitive);
	}
	
	set_cmd_dirty (priv, node);

	gtk_widget_show (menu_widget);
	
	if ((verb = xmlGetProp (node, "verb"))) {
		gtk_object_set_data (GTK_OBJECT (menu_widget),
				     BONOBO_APP_PRIV_KEY, priv);
		gtk_signal_connect (GTK_OBJECT (menu_widget), "activate",
				    (GtkSignalFunc) exec_verb_cb, node);
		xmlFree (verb);
	}
}

static void
update_menus (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode  *l;
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	if (!info->widget)
		info->widget = GTK_WIDGET (priv->menu);

	container_destroy_siblings (priv->tree, info->widget, node->childs);

	for (l = node->childs; l; l = l->next)
		build_menu_widget (priv, l);
}

static void build_toolbar_widget (BonoboAppPrivate *priv, xmlNode *node);

static void
build_toolbar_placeholder (BonoboAppPrivate *priv, xmlNode *node, GtkWidget *parent)
{
	xmlNode   *l;
	char      *delimit;
	gboolean   top = FALSE, bottom = FALSE;
	GtkWidget *sep;

	if ((delimit = xmlGetProp (node, "delimit"))) {
		if (!strcmp (delimit, "top") ||
		    !strcmp (delimit, "both"))
			top = (node->childs != NULL) && (node->prev != NULL);

		if (!strcmp (delimit, "bottom") ||
		    !strcmp (delimit, "both"))
			bottom = (node->childs != NULL) && (node->next != NULL);
		xmlFree (delimit);
	}

	if (top) {
		sep = bonobo_app_item_new_separator ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		bonobo_app_toolbar_add (BONOBO_APP_TOOLBAR (parent), sep);
	}

/*	g_warning ("Building placeholder %d %d %p %p %p", top, bottom,
	node->childs, node->prev, node->next);*/
		
	for (l = node->childs; l; l = l->next)
		build_toolbar_widget (priv, l);

	if (bottom) {
		sep = bonobo_app_item_new_separator ();
		gtk_widget_set_sensitive (sep, FALSE);
		gtk_widget_show (sep);
		bonobo_app_toolbar_add (BONOBO_APP_TOOLBAR (parent), sep);
	}
}

/*
 * see menu_toplevel_item_create_widget.
 */
static void
build_toolbar_widget (BonoboAppPrivate *priv, xmlNode *node)
{
	NodeInfo   *info;
	GtkWidget  *parent;
	char *type, *verb, *sensitive, *state, *txt, *label;
	GtkWidget  *pixmap;
	GtkWidget  *item;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_parent_widget (priv->tree, node);

	if (!strcmp (node->name, "placeholder")) {
		build_toolbar_placeholder (priv, node, parent);
		return;
	}

	/* Create toolbar item */
	if ((type = xmlGetProp (node, "pixtype"))) {
		pixmap = bonobo_ui_util_xml_get_pixmap (parent, node);
		if (pixmap) 
			gtk_widget_show (GTK_WIDGET (pixmap));
		xmlFree (type);
	} else
		pixmap = NULL;

	type = xmlGetProp (node, "type");
	label = xmlGetProp (node, "label");
	if (!type || !strcmp (type, "std"))
		item = bonobo_app_item_new_item (label, pixmap);
	
	else if (!strcmp (type, "toggle"))
		item = bonobo_app_item_new_toggle (label, pixmap);
	
	else if (!strcmp (type, "separator"))
		item = bonobo_app_item_new_separator ();
	
	else {
		/* FIXME: Implement radio-toolbars */
		g_warning ("Invalid type '%s'", type);
		return;
	}
	XML_FREE (type);
	XML_FREE (label);
	
	bonobo_app_toolbar_add (BONOBO_APP_TOOLBAR (parent), item);

	bonobo_app_item_set_tooltip (
		BONOBO_APP_ITEM (item),
		bonobo_app_toolbar_get_tooltips (
			BONOBO_APP_TOOLBAR (parent)),
		(txt = xmlGetProp (node, "descr")));
	XML_FREE (txt);

	info->widget = GTK_WIDGET (item);
	gtk_widget_show (info->widget);

	if ((verb = xmlGetProp (node, "verb"))) {
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    (GtkSignalFunc) exec_verb_cb, node);
		xmlFree (verb);
	}

	gtk_object_set_data (GTK_OBJECT (item), BONOBO_APP_PRIV_KEY, priv);
	gtk_signal_connect (GTK_OBJECT (item), "state_altered",
			    (GtkSignalFunc) app_item_emit_ui_event, node);

	if ((sensitive = xmlGetProp (node, "sensitive"))) {
		set_cmd_state (priv, node, "sensitive", sensitive, FALSE);
		xmlFree (sensitive);
	}

	if ((state = xmlGetProp (node, "state"))) {
		set_cmd_state (priv, node, "state", state, FALSE);
		xmlFree (state);
	}

	set_cmd_dirty (priv, node);
}

static void
update_dockitem (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode       *l;
	NodeInfo      *info = bonobo_ui_xml_get_data (priv->tree, node);
	char          *txt;
	guint          dummy;
	char          *dockname = xmlGetProp (node, "name");
	GnomeDockItem *item;
	BonoboAppToolbar *toolbar;
	gboolean          tooltips;

	item = gnome_dock_get_item_by_name (priv->dock,
					    dockname,
					    &dummy, &dummy,
					    &dummy, &dummy);
	
	if (!item) {
		GnomeDockItemBehavior beh = GNOME_DOCK_ITEM_BEH_EXCLUSIVE;
		
		if (!gnome_preferences_get_toolbar_detachable())
			beh |= GNOME_DOCK_ITEM_BEH_LOCKED;

		item = GNOME_DOCK_ITEM (gnome_dock_item_new (
			dockname, beh));

		gnome_dock_add_item (priv->dock, item,
				     GNOME_DOCK_TOP,
				     1, 0, 0, TRUE);
		gtk_widget_show (GTK_WIDGET (item));
	}

	if (GTK_BIN (item)->child)
		gtk_widget_destroy (GTK_BIN (item)->child);

	toolbar = BONOBO_APP_TOOLBAR (bonobo_app_toolbar_new ());
	info->widget = GTK_WIDGET (toolbar);

	gtk_container_add (GTK_CONTAINER (item),
			   info->widget);
	gtk_widget_show (info->widget);

	for (l = node->childs; l; l = l->next) {
		if (!strcmp (l->name, "toolitem"))
			build_toolbar_widget (priv, l);
	}

	if ((txt = xmlGetProp (node, "look"))) {
		if (!strcmp (txt, "both"))
			bonobo_app_toolbar_set_style (
				toolbar, GTK_TOOLBAR_BOTH);
		else
			bonobo_app_toolbar_set_style (
				toolbar, GTK_TOOLBAR_ICONS);
		xmlFree (txt);
	}

	if ((txt = xmlGetProp (node, "relief"))) {

		if (!strcmp (txt, "normal"))
			bonobo_app_toolbar_set_relief (
				toolbar, GTK_RELIEF_NORMAL);

		else if (!strcmp (txt, "half"))
			bonobo_app_toolbar_set_relief (
				toolbar, GTK_RELIEF_HALF);
		else
			bonobo_app_toolbar_set_relief (
				toolbar, GTK_RELIEF_NONE);
		xmlFree (txt);
	}

	tooltips = TRUE;
	if ((txt = xmlGetProp (node, "tips"))) {
		tooltips = atoi (txt);
		xmlFree (txt);
	}
	
	bonobo_app_toolbar_set_tooltips (toolbar, tooltips);

	xmlFree (dockname);
}

typedef struct {
	guint           key;
	GdkModifierType mods;
	xmlNode        *node;
} Binding;

static gboolean
keybindings_free (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	g_free (key);

	return TRUE;
}

/*
 * Shamelessly stolen from gtkbindings.c
 */
static guint
keybinding_hash_fn (gconstpointer  key)
{
  register const Binding *e = key;
  register guint h;

  h = e->key;
  h ^= e->mods;

  return h;
}

static gint
keybinding_compare_fn (gconstpointer a,
		       gconstpointer b)
{
	register const Binding *ba = a;
	register const Binding *bb = b;

	return (ba->key == bb->key && ba->mods == bb->mods);
}

static void
update_keybindings (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode *l;

	g_hash_table_foreach_remove (priv->keybindings, keybindings_free, NULL);

	for (l = node->childs; l; l = l->next) {
		guint           key;
		GdkModifierType mods;
		char           *name;
		Binding        *binding;
		
		name = xmlGetProp (l, "name");
		if (!name)
			continue;
		
		gtk_accelerator_parse (name, &key, &mods);
		xmlFree (name);

		binding       = g_new0 (Binding, 1);
		binding->mods = mods & BINDING_MOD_MASK ();
		binding->key  = gdk_keyval_to_lower (key);
		binding->node = l;

		g_hash_table_insert (priv->keybindings, binding, binding);
	}
}

static void
update_status (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode *l;

	gtk_container_foreach (GTK_CONTAINER (priv->status),
			       (GtkCallback) gtk_widget_destroy,
			       NULL);
	priv->main_status = NULL;

	for (l = node->childs; l; l = l->next) {
		char *name;
		GtkWidget *widget;
		
		name = xmlGetProp (l, "name");
		if (!name)
			continue;

		if (!strcmp (name, "main")) {
			char *txt;

			widget = gtk_statusbar_new ();
			priv->main_status = GTK_STATUSBAR (widget);
			gtk_widget_show (GTK_WIDGET (widget));
			gtk_box_pack_end (priv->status, widget, TRUE, TRUE, 0);
			
			if ((txt = xmlNodeGetContent (l))) {
				guint id;

				id = gtk_statusbar_get_context_id (priv->main_status,
								   "BonoboApp:data");

				gtk_statusbar_push (priv->main_status, id, txt);
				xmlFree (txt);
			}
		} else {
			NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, l);

			g_warning ("TESTME: untested code path");

			if (!info->object) {
				g_warning ("Unknown status bar object");
				xmlFree (name);
				continue;
			}

			widget = bonobo_app_item_new_control (info->object);
			g_return_if_fail (widget != NULL);
			gtk_widget_show (GTK_WIDGET (widget));
			
			gtk_box_pack_end (priv->status, widget, TRUE, TRUE, 0);
		}
		xmlFree (name);
	}

	gtk_widget_show (GTK_WIDGET (priv->status));
}

typedef enum {
	UI_UPDATE_MENU,
	UI_UPDATE_DOCKITEM
} UIUpdateType;

static void
seek_dirty (BonoboAppPrivate *priv, xmlNode *node, UIUpdateType type)
{
	BonoboUIXmlData *info;

	if (!node)
		return;

	info = bonobo_ui_xml_get_data (priv->tree, node);
	if (info->dirty) { /* Rebuild tree from here down */
		
		bonobo_ui_xml_set_dirty (priv->tree, node, FALSE);

		switch (type) {
		case UI_UPDATE_MENU:
			update_menus (priv, node);
			break;
		case UI_UPDATE_DOCKITEM:
			update_dockitem (priv, node);
			break;
		default:
			g_warning ("Looking for unhandled super type");
			break;
		}
	} else {
		xmlNode *l;

		for (l = node->childs; l; l = l->next)
			seek_dirty (priv, l, type);
	}

/*	gtk_widget_show_all (GTK_WIDGET (priv->window));*/
}

static void
update_widgets (BonoboAppPrivate *priv)
{
	xmlNode *node;

	for (node = priv->tree->root->childs; node; node = node->next) {
		if (!node->name)
			continue;

		if (!strcmp (node->name, "menu")) {
			seek_dirty (priv, node, UI_UPDATE_MENU);

		} else if (!strcmp (node->name, "dockitem")) {
			seek_dirty (priv, node, UI_UPDATE_DOCKITEM);

		} else if (!strcmp (node->name, "keybindings")) {
			update_keybindings (priv, node);

		} else if (!strcmp (node->name, "status")) {
			update_status (priv, node);

		} /* else unknown */
	}

	update_commands_state (priv);
}

void
bonobo_app_set_contents (BonoboApp *app,
			 GtkWidget *contents)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->client_area != NULL);

	gtk_container_add (GTK_CONTAINER (app->priv->client_area), contents);
}

GtkWidget *
bonobo_app_get_contents (BonoboApp *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app->priv != NULL, NULL);
	g_return_val_if_fail (app->priv->dock != NULL, NULL);

	return GTK_BIN (app->priv->client_area)->child;
}

GtkWidget *
bonobo_app_get_window (BonoboApp *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app->priv != NULL, NULL);
	g_return_val_if_fail (app->priv->dock != NULL, NULL);

	return app->priv->window;
}

static gboolean
radio_group_destroy (gpointer	key,
		     gpointer	value,
		     gpointer	user_data)
{
	g_free (key);
	g_slist_free (value);

	return TRUE;
}

static void
destroy_priv (BonoboAppPrivate *priv)
{
	gtk_widget_destroy (priv->window);
		
	gtk_object_unref (GTK_OBJECT (priv->tree));

	g_free (priv->name);
	g_free (priv->prefix);

	g_hash_table_foreach_remove (priv->radio_groups,
				     radio_group_destroy, NULL);
	g_hash_table_destroy (priv->radio_groups);

	g_hash_table_foreach_remove (priv->keybindings,
				     keybindings_free, NULL);
	g_hash_table_destroy (priv->keybindings);

	while (priv->components)
		app_component_destroy (priv, priv->components->data);
	
	g_free (priv);
}

static void
bonobo_app_destroy (GtkObject *object)
{
	BonoboApp *app = (BonoboApp *)object;
	
	if (app) {
		if (app->priv)
			destroy_priv (app->priv);
		app->priv = NULL;
	}
	GTK_OBJECT_CLASS (bonobo_app_parent_class)->destroy (object);
}

static void
impl_node_set (PortableServer_Servant   servant,
	       const CORBA_char        *path,
	       const CORBA_char        *xml,
	       const CORBA_char        *component_name,
	       CORBA_Environment       *ev)
{
	BonoboApp *app = bonobo_app_from_servant (servant);
	AppComponent *component = app_component_get (app->priv, component_name);

	if (!bonobo_app_xml_merge (app, path, xml, component->name))
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_UIContainer_MalFormedXML, NULL);
}

static CORBA_char *
impl_node_get (PortableServer_Servant servant,
	       const CORBA_char      *path,
	       const CORBA_boolean    nodeOnly,
	       CORBA_Environment     *ev)
{
	BonoboApp  *app = bonobo_app_from_servant (servant);
	xmlDoc     *doc;
	xmlChar    *mem = NULL;
	xmlNode    *children;
	int         size;
	CORBA_char *ret;

	doc = xmlNewDoc ("1.0");
	g_return_val_if_fail (
		doc != NULL,
		CORBA_string_dup ("<Error name=\"memory\"/>"));

	doc->root = bonobo_ui_xml_get_path (app->priv->tree, path);
	g_return_val_if_fail (
		doc->root != NULL,
		CORBA_string_dup ("<Error name=\"tree\"/>"));

	children = doc->root->childs;

	if (nodeOnly)
		doc->root->childs = NULL;

	xmlDocDumpMemory (doc, &mem, &size);

	if (nodeOnly)
		doc->root->childs = children;

	g_return_val_if_fail (
		mem != NULL,
		CORBA_string_dup ("<Error name=\"dump\"/>"));

	doc->root = NULL;
	xmlFreeDoc (doc);

	ret = CORBA_string_dup (mem);
	xmlFree (mem);

	return ret;
}

static void
impl_node_remove (PortableServer_Servant servant,
		  const CORBA_char      *path,
		  const CORBA_char      *component_name,
		  CORBA_Environment     *ev)
{
	BonoboApp *app = bonobo_app_from_servant (servant);
	AppComponent *component = app_component_get (app->priv, component_name);

	bonobo_app_xml_rm (app, path, component->name);
}

static CORBA_boolean
impl_node_exists (PortableServer_Servant servant,
		  const CORBA_char      *path,
		  CORBA_Environment     *ev)
{
	BonoboApp *app = bonobo_app_from_servant (servant);

	return bonobo_ui_xml_exists (app->priv->tree, path);
}

static void
impl_object_set (PortableServer_Servant servant,
		 const CORBA_char      *path,
		 const Bonobo_Unknown   control,
		 CORBA_Environment     *ev)
{
	BonoboApp *app = bonobo_app_from_servant (servant);
	xmlNode   *node;
	NodeInfo  *info;

	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);

	node = bonobo_ui_xml_get_path (app->priv->tree, path);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (app->priv->tree, node);

	if (info->object)
		bonobo_object_release_unref (info->object, ev);

	info->object = bonobo_object_dup_ref (control, ev);
}

static Bonobo_Unknown
impl_object_get (PortableServer_Servant servant,
		 const CORBA_char      *path,
		 CORBA_Environment     *ev)
{
	BonoboApp *app = bonobo_app_from_servant (servant);
	xmlNode   *node;
	NodeInfo  *info;

	g_return_val_if_fail (app != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (app->priv != NULL, CORBA_OBJECT_NIL);

	node = bonobo_ui_xml_get_path (app->priv->tree, path);
	g_return_val_if_fail (node != NULL, CORBA_OBJECT_NIL);

	info = bonobo_ui_xml_get_data (app->priv->tree, node);

	return bonobo_object_dup_ref (info->object, ev);
}

static gint
bonobo_app_binding_handle (GtkWidget        *widget,
			   GdkEventKey      *event,
			   BonoboAppPrivate *priv)
{
	Binding  lookup, *binding;

	lookup.key  = gdk_keyval_to_lower (event->keyval);
	lookup.mods = event->state & BINDING_MOD_MASK ();

	if (!(binding = g_hash_table_lookup (priv->keybindings, &lookup)))
		return FALSE;
	else {
		BonoboUIXmlData *data;
		char *verb;
		
		data = bonobo_ui_xml_get_data (priv->tree, binding->node);
		g_return_val_if_fail (data != NULL, FALSE);
		
		real_exec_verb (priv, data->id,
				(verb = xmlGetProp (binding->node, "verb")));
		XML_FREE (verb);
		
		return TRUE;
	}
	
	return FALSE;
}

/**
 * bonobo_app_get_epv:
 */
POA_Bonobo_UIContainer__epv *
bonobo_app_get_epv (void)
{
	POA_Bonobo_UIContainer__epv *epv;

	epv = g_new0 (POA_Bonobo_UIContainer__epv, 1);

	epv->register_component   = impl_register_component;
	epv->deregister_component = impl_deregister_component;

	epv->node_set    = impl_node_set;
	epv->node_get    = impl_node_get;
	epv->node_remove = impl_node_remove;
	epv->node_exists = impl_node_exists;

	epv->object_set  = impl_object_set;
	epv->object_get  = impl_object_get;

	return epv;
}

static void
bonobo_app_corba_class_init ()
{
	bonobo_app_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_app_vepv.Bonobo_UIContainer_epv = bonobo_app_get_epv ();
}

static void
bonobo_app_class_init (BonoboAppClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_app_parent_class =
		gtk_type_class (bonobo_object_get_type ());

	object_class->destroy = bonobo_app_destroy;

	bonobo_app_corba_class_init ();
}

static BonoboAppPrivate *
construct_priv (BonoboApp  *app,
		const char *app_name,
		const char *title)
{
	BonoboAppPrivate *priv;
	GnomeDockItemBehavior behavior;

	priv = g_new0 (BonoboAppPrivate, 1);

	priv->name   = g_strdup (app_name);
	priv->prefix = g_strconcat ("/", app_name, "/", NULL);

	priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if (title)
		gtk_window_set_title (GTK_WINDOW (priv->window), title);
	gtk_object_set_data (GTK_OBJECT (priv->window),
			     BONOBO_APP_WINDOW_KEY, app);

	/* Keybindings; the gtk_binding stuff is just too evil */
	gtk_signal_connect (GTK_OBJECT (priv->window), "key_press_event",
			    (GtkSignalFunc) bonobo_app_binding_handle, priv);

	priv->dock   = GNOME_DOCK (gnome_dock_new ());
	gtk_container_add (GTK_CONTAINER (priv->window),
			   GTK_WIDGET    (priv->dock));

	behavior = (GNOME_DOCK_ITEM_BEH_EXCLUSIVE
		    | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	if (!gnome_preferences_get_menubar_detachable ())
		behavior |= GNOME_DOCK_ITEM_BEH_LOCKED;

	priv->menu_item = GNOME_DOCK_ITEM (gnome_dock_item_new (
		"menu", behavior));
	priv->menu      = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_container_add (GTK_CONTAINER (priv->menu_item),
			   GTK_WIDGET    (priv->menu));
	gnome_dock_add_item (priv->dock, priv->menu_item,
			     GNOME_DOCK_TOP, 0, 0, 0, TRUE);

	priv->main_vbox = gtk_vbox_new (FALSE, 0);
	gnome_dock_set_client_area (priv->dock, priv->main_vbox);

	priv->client_area = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->client_area, TRUE, TRUE, 0);

	priv->status = GTK_BOX (gtk_hbox_new (FALSE, 0));
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), GTK_WIDGET (priv->status), FALSE, FALSE, 0);

	priv->tree = bonobo_ui_xml_new (NULL,
					info_new_fn,
					info_free_fn,
					info_dump_fn,
					add_node_fn);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "override",
			    (GtkSignalFunc) override_fn, priv);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "reinstate",
			    (GtkSignalFunc) reinstate_fn, priv);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "remove",
			    (GtkSignalFunc) remove_fn, priv);

	priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (priv->window),
				    priv->accel_group);

	gtk_widget_show_all (GTK_WIDGET (priv->dock));
	gtk_widget_hide (GTK_WIDGET (priv->status));

	priv->radio_groups = g_hash_table_new (
		g_str_hash, g_str_equal);

	priv->keybindings = g_hash_table_new (keybinding_hash_fn, 
					      keybinding_compare_fn);	

	return priv;
}

/**
 * bonobo_app_get_type:
 *
 * Returns: The GtkType for the BonoboApp class.
 */
GtkType
bonobo_app_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboApp",
			sizeof (BonoboApp),
			sizeof (BonoboAppClass),
			(GtkClassInitFunc) bonobo_app_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_app_corba_object_create:
 * @object: The GtkObject that will wrap the CORBA object.
 *
 * Creates an activates the CORBA object that is wrapped
 * by the BonoboObject @object.
 *
 * Returns: An activated object reference to the created object or
 * %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_UIContainer
bonobo_app_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_UIContainer *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_UIContainer *)g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_app_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_UIContainer__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);

	return bonobo_object_activate_servant (object, servant);
}

BonoboApp *
bonobo_app_construct (BonoboApp  *app,
		      Bonobo_UIContainer  corba_app,
		      const char *app_name,
		      const char *title)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app_name != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_APP (app), NULL);
	g_return_val_if_fail (corba_app != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (app), corba_app);
	
	app->priv = construct_priv (app, app_name, title);

	return app;
}

BonoboApp *
bonobo_app_new (const char   *app_name,
		const char   *title)
{
	Bonobo_UIContainer corba_app;
	BonoboApp *app;

	app = gtk_type_new (BONOBO_APP_TYPE);

	corba_app = bonobo_app_corba_object_create (BONOBO_OBJECT (app));
	if (corba_app == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (app));
		return NULL;
	}
	
	return bonobo_app_construct (app, corba_app, app_name, title);
}

void
bonobo_app_xml_merge_tree (BonoboApp  *app,
			   const char *path,
			   xmlNode    *tree,
			   gpointer    listener)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->tree != NULL);

	if (!tree || !tree->name)
		return;

	/*
	 *  Because peer to peer merging makes the code hard, and
	 * paths non-inituitive and since we want to merge root
	 * elements as peers to save lots of redundant CORBA calls
	 * we special case root.
	 */
	if (!strcmp (tree->name, "Root")) {
		bonobo_ui_xml_strip (tree);
		bonobo_ui_xml_merge (app->priv->tree, path,
				     tree->childs, listener);
	} else
		bonobo_ui_xml_merge (app->priv->tree, path, tree, listener);

	update_widgets (app->priv);
}

gboolean
bonobo_app_xml_merge (BonoboApp  *app,
		      const char *path,
		      const char *xml,
		      gpointer    listener)
{
	xmlDoc  *doc;

	g_return_val_if_fail (app != NULL, FALSE);
	g_return_val_if_fail (xml != NULL, FALSE);
	g_return_val_if_fail (app->priv != NULL, FALSE);
	g_return_val_if_fail (app->priv->tree != NULL, FALSE);

	doc = xmlParseDoc ((char *)xml);

	g_return_val_if_fail (doc != NULL, FALSE);

	bonobo_app_xml_merge_tree (app, path, doc->root, listener);
	doc->root = NULL;
	
	xmlFreeDoc (doc);

	return TRUE;
}

void
bonobo_app_xml_rm (BonoboApp  *app,
		   const char *path,
		   gpointer    by_listener)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->tree != NULL);

	bonobo_ui_xml_rm (app->priv->tree, "/", by_listener);
	update_widgets (app->priv);
}

void
bonobo_app_dump (BonoboApp  *app,
		 const char *msg)
{
	g_return_if_fail (BONOBO_IS_APP (app));

	bonobo_ui_xml_dump (app->priv->tree, app->priv->tree->root, msg);
}

BonoboApp *
bonobo_app_from_window (GtkWindow *window)
{
	return gtk_object_get_data (GTK_OBJECT (window),
				    BONOBO_APP_WINDOW_KEY);
}

GtkAccelGroup *
bonobo_app_get_accel_group (BonoboApp *app)
{
	g_return_val_if_fail (BONOBO_IS_APP (app), NULL);

	return app->priv->accel_group;
}
