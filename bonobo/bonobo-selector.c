/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include "gnome-bonobo-selector.h"
#include <string.h> /* strcmp */

#define DEFAULT_INTERFACE	"IDL:GNOME/Embeddable:1.0"

static GtkDialogClass *parent_class;

struct _GnomeBonoboSelectorPrivate 
{
	GtkWidget *clist;
#ifdef BONOBO_USE_GNOME2
	OAF_ServerInfoList *servers;
#else
	GoadServerList *servers;
	int n_servers;
	const gchar **interfaces_required;
#endif
};

enum {
	OK,
	CANCEL,
	LAST_SIGNAL
};

guint bonobo_selector_signals[LAST_SIGNAL] = { 0, 0 };

static void gnome_bonobo_selector_class_init (GnomeBonoboSelectorClass *klass);
static void gnome_bonobo_selector_init (GtkWidget *widget);
static void gnome_bonobo_selector_destroy (GtkObject *object);
static void button_callback (GtkWidget *widget, gint button_number,
	gpointer data);
static void ok_callback (GtkWidget *widget, gpointer data);
static void cancel_callback (GtkWidget *widget, gpointer data);
#ifdef BONOBO_USE_GNOME2
static void do_oaf_query(GnomeBonoboSelector *widget, const char *requirements, const char **sort_order);
#else
static void add_gnorba_objects (GnomeBonoboSelector *widget); 
static GList *get_filtered_objects (GnomeBonoboSelector *widget);
static gboolean stringlist_contains (gchar **list, const gchar *word);
#endif

/* fixme: revove this as soon it is included in gnome-dialog */
#ifndef BONOBO_USE_GNOME2
void       
gnome_dialog_clicked (GnomeDialog *dialog, gint button_num)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "clicked", button_num);
}              
#endif

static void
gnome_bonobo_selector_class_init (GnomeBonoboSelectorClass *klass)
{
	GtkObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	
	object_class = (GtkObjectClass *) klass;
	parent_class = gtk_type_class (gnome_dialog_get_type ());

	bonobo_selector_signals[OK] =
		gtk_signal_new ("ok", GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GnomeBonoboSelectorClass, ok),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	bonobo_selector_signals[CANCEL] =
		gtk_signal_new ("cancel", GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (GnomeBonoboSelectorClass, cancel),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, bonobo_selector_signals,
		LAST_SIGNAL);
	
	object_class->destroy = gnome_bonobo_selector_destroy;
}

/**
 * gnome_bonobo_selector_get_type:
 *
 * Returns: The GtkType for the GnomeBonoboSelector object class.
 */
GtkType
gnome_bonobo_selector_get_type (void)
{
	static guint bonobo_selector_type = 0;

	if (!bonobo_selector_type)
	{
		GtkTypeInfo bonobo_selector_info =
		{
			"GnomeBonoboSelector",
			sizeof (GnomeBonoboSelector),
			sizeof (GnomeBonoboSelectorClass),
			 (GtkClassInitFunc) gnome_bonobo_selector_class_init,
			 (GtkObjectInitFunc) gnome_bonobo_selector_init,
			 (GtkArgSetFunc) NULL,
			 (GtkArgGetFunc) NULL
		};

		bonobo_selector_type = gtk_type_unique (
			gnome_dialog_get_type (),
			&bonobo_selector_info);
	}

	return bonobo_selector_type;
}

/**
 * gnome_bonobo_selector_new:
 * @title: A string which should go in the title of the
 * GnomeBonoboSelector window.
 * @interfaces_required: A NULL_terminated array of interfaces which a
 * server must support in order to be listed in the selector.  Defaults
 * to "IDL:GNOME/Embeddable:1.0" if no interfaces are listed.
 * @requirements: A query to be made of the OAF database, specifying the constraints for a server.
 * @sort_order: A list of sort specifications, to be used to sort the query results.
 *
 * Creates a new GnomeBonoboSelector widget.  The title of the dialog
 * is set to @title, and the list of selectable servers is populated
 * with those servers which support the interfaces specified in
 * @interfaces_required.
 *
 * Returns: A pointer to the newly-created GnomeBonoboSelector widget.
 */
#ifdef BONOBO_USE_GNOME2
GtkWidget *
gnome_bonobo_selector_new (const gchar *title,
			   const char *requirements,
			   const char **sort_order)
#else
GtkWidget *
gnome_bonobo_selector_new (const gchar *title,
                           const gchar **interfaces_required)
#endif
{
	GnomeBonoboSelector *sel;
	GnomeBonoboSelectorPrivate *priv;

	if (title == NULL) title = "";

	
	sel = gtk_type_new (gnome_bonobo_selector_get_type ());
	priv = sel->priv;
#ifdef BONOBO_USE_GNOME2
	do_oaf_query(sel, requirements, sort_order);
#else
	priv->interfaces_required = interfaces_required;
	add_gnorba_objects (sel);
#endif
	gtk_window_set_title (GTK_WINDOW (sel), title);
	return GTK_WIDGET (sel);
}

static void
gnome_bonobo_selector_destroy (GtkObject *object)
{
	GnomeBonoboSelector *sel;
	GnomeBonoboSelectorPrivate *priv; 
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_SELECTOR (object));

	sel = GNOME_BONOBO_SELECTOR (object);
	priv = sel->priv;

	gtk_widget_destroy (priv->clist);
#ifdef BONOBO_USE_GNOME2
	CORBA_free(priv->servers);
#else
	goad_server_list_free (priv->servers);
#endif
	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		 (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	
}

#ifdef BONOBO_USE_GNOME2
/**
 * gnome_bonobo_selector_get_aid:
 * @sel: A GnomeBonoboSelector widget.
 *
 * Returns: A newly-allocated string containing the GOAD ID of the
 * currently-selected CORBA server (i.e., the corba server whose
 * name is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
gnome_bonobo_selector_get_selected_aid (GnomeBonoboSelector *sel)
#else
/**
 * gnome_bonobo_selector_get_selected_goad_id:
 * @sel: A GnomeBonoboSelector widget.
 *
 * Returns: A newly-allocated string containing the GOAD ID of the
 * currently-selected CORBA server (i.e., the corba server whose
 * name is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
gnome_bonobo_selector_get_selected_goad_id (GnomeBonoboSelector *sel)
#endif
{
	GList *selection;
	gchar *text;
	GnomeBonoboSelectorPrivate *priv; 
	
	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (selection == NULL) return NULL;

#ifdef BONOBO_USE_GNOME2
	{
		char *iid, *host, *user;

		gtk_clist_get_text (GTK_CLIST (priv->clist), GPOINTER_TO_INT(selection->data), 1, &host);
		gtk_clist_get_text (GTK_CLIST (priv->clist), GPOINTER_TO_INT(selection->data), 2, &user);
		gtk_clist_get_text (GTK_CLIST (priv->clist), GPOINTER_TO_INT(selection->data), 3, &iid);
		return g_strdup_printf("OAFAID:[%s,%s,%s]", iid, user, host);
	}
#else
	gtk_clist_get_text (GTK_CLIST (priv->clist), GPOINTER_TO_INT(selection->data), 1, &text);

	return g_strdup (text);
#endif
}

#ifdef BONOBO_USE_GNOME2
/**
 * gnome_bonobo_select_aid:
 * @title: The title to be used for the dialog.
 * @requirements: A query to be made of the OAF database, specifying the constraints for a server.
 * @sort_order: A list of sort specifications, to be used to sort the query results.
 *
 * Calls gnome_bonobo_selector_new() to create a new
 * GnomeBonoboSelector widget with the specified paramters, @title and
 * @interfaces_required.  Then runs the dialog modally and allows
 * the user to make a selection.
 *
 * Returns: The GOAD ID of the selected server, or NULL if no server
 * is selected.  The GOAD ID string has been allocated with g_strdup
 */
gchar *
gnome_bonobo_select_aid (const gchar *title,
			 const char *requirements,
			 const char **sort_order)
#else
/**
 * gnome_bonobo_select_goad_id:
 * @title: The title to be used for the dialog.
 * @interfaces_required: A list of required interfaces.  See
 * gnome_bonobo_selector_new().
 *
 * Calls gnome_bonobo_selector_new() to create a new
 * GnomeBonoboSelector widget with the specified paramters, @title and
 * @interfaces_required.  Then runs the dialog modally and allows
 * the user to make a selection.
 *
 * Returns: The GOAD ID of the selected server, or NULL if no server
 * is selected.  The GOAD ID string has been allocated with g_strdup
 */
gchar *
gnome_bonobo_select_goad_id (const gchar *title,
			     const gchar **interfaces_required)
#endif
{
	GtkWidget *sel;
	char *name = NULL;
	int n;
	GnomeBonoboSelectorPrivate *priv;

#ifdef BONOBO_USE_GNOME2
	sel = gnome_bonobo_selector_new (title, requirements, sort_order);
#else
	sel = gnome_bonobo_selector_new (title, interfaces_required);
#endif
	if (sel == NULL)
		return NULL;
	priv = GNOME_BONOBO_SELECTOR(sel)->priv;

#ifdef BONOBO_USE_GNOME2
	if(priv->servers->_length == 1)
#else
	if(priv->n_servers == 1)
#endif
		{
			char *retval;
#ifdef BONOBO_USE_GNOME2
			retval = g_strdup_printf("OAFAID:[%s,%s,%s]",
						 priv->servers->_buffer[0].iid,
						 priv->servers->_buffer[0].username,
						 priv->servers->_buffer[0].hostname);
#else
			char *goad_id;

			gtk_clist_get_text (GTK_CLIST (priv->clist), 0, 1, &goad_id);
			retval = g_strdup(goad_id);
#endif
			gtk_widget_destroy(GTK_WIDGET(sel));

			return retval;
		}

	gtk_signal_connect (GTK_OBJECT (sel),
		"ok", GTK_SIGNAL_FUNC (ok_callback), NULL);
	gtk_signal_connect (GTK_OBJECT (sel),
		"cancel", GTK_SIGNAL_FUNC (cancel_callback), NULL);
	
	gtk_object_set_user_data (GTK_OBJECT (sel), NULL);
	
	gtk_widget_show (sel);
		
	n = gnome_dialog_run_and_close (GNOME_DIALOG(sel));
	if (n == 0)
		name = gtk_object_get_user_data (GTK_OBJECT (sel));
		
	gtk_widget_destroy (sel);

	return name;
}

static void
button_callback (GtkWidget *widget, gint button_number,
		 gpointer data) 
{
	switch (button_number)
	{
		case 0:
			gtk_signal_emit (GTK_OBJECT (data), 
				bonobo_selector_signals[OK]);
			break;
		case 1:
			gtk_signal_emit (GTK_OBJECT (data),
				bonobo_selector_signals[CANCEL]);
		default:
			break;
	}
}

static void
ok_callback (GtkWidget *widget, gpointer data)
{
	char *text;
#ifdef BONOBO_USE_GNOME2
	text = gnome_bonobo_selector_get_selected_aid (
		GNOME_BONOBO_SELECTOR (widget));
#else
	text = gnome_bonobo_selector_get_selected_goad_id (
		GNOME_BONOBO_SELECTOR (widget));
#endif
	gtk_object_set_user_data (GTK_OBJECT (widget), text);
	gtk_main_quit ();
}

static void
cancel_callback (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void
select_row (GtkCList *clist, gint row, gint col, 
	    GdkEvent *event, GnomeBonoboSelector *sel)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		gnome_dialog_clicked ( GNOME_DIALOG (sel), 0);
	else {
		GtkCListClass *cl;

		cl = gtk_type_class (GTK_TYPE_CLIST);
		if (cl->select_row)
			cl->select_row (clist, row, col, event);
	}
}

static void
gnome_bonobo_selector_init (GtkWidget *widget)
{
	GnomeBonoboSelector *sel = GNOME_BONOBO_SELECTOR (widget);
	GtkWidget *scrolled;
	GnomeBonoboSelectorPrivate *priv;
	int i;

#ifdef BONOBO_USE_GNOME2
	char *titles[] = { N_("Description"), N_("Host"), N_("User"), N_("IID") };
#else
	gchar *titles[] = { N_("Description"),
			    "goadid", NULL };
#endif
	
	g_return_if_fail (widget != NULL);
	
	sel->priv = g_new0 (GnomeBonoboSelectorPrivate, 1);
	priv = sel->priv;
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	for(i = 0; i < sizeof(titles)/sizeof(titles[0]); i++)
		titles[i] = _(titles[i]);

	priv->clist = gtk_clist_new_with_titles (sizeof(titles)/sizeof(titles[0]), titles);
	gtk_clist_set_selection_mode (GTK_CLIST (priv->clist),
		GTK_SELECTION_BROWSE);
	gtk_signal_connect (GTK_OBJECT (priv->clist), "select-row",
			    GTK_SIGNAL_FUNC (select_row), sel);
	gtk_clist_set_column_visibility (GTK_CLIST (priv->clist), 1, FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled), priv->clist);
	
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sel)->vbox), scrolled,
		TRUE, TRUE, 0);
	
	gnome_dialog_append_button (GNOME_DIALOG (sel), GNOME_STOCK_BUTTON_OK);
	gnome_dialog_append_button (GNOME_DIALOG (sel), 
		GNOME_STOCK_BUTTON_CANCEL);
	
	gtk_signal_connect (GTK_OBJECT (sel),
		"clicked", GTK_SIGNAL_FUNC (button_callback), sel);
	gtk_signal_connect (GTK_OBJECT (sel), "close",
		GTK_SIGNAL_FUNC (button_callback), sel);
	
	gtk_widget_set_usize (priv->clist, 200, 200);
	gtk_widget_show (priv->clist);
	gtk_widget_show (scrolled);
}

#ifdef BONOBO_USE_GNOME2
static void
do_oaf_query(GnomeBonoboSelector *widget, const char *requirements, const char **sort_order)
{
	GnomeBonoboSelectorPrivate *priv = widget->priv;
	CORBA_Environment ev;
	int i;
	char *text[4];
	GSList *langs = gnome_i18n_get_language_list(NULL);

	CORBA_exception_init(&ev);
	priv->servers = oaf_query(requirements, sort_order, &ev);
	if(ev._major != CORBA_NO_EXCEPTION) {
		priv->servers = NULL;
		return;
	}

	gtk_clist_freeze(priv->clist);

	for(i = 0; i < priv->servers->_length; i++) {
		text[0] = oaf_server_info_attr_lookup(&priv->servers->_buffer[i], "description", langs);
		text[1] = priv->servers->_buffer[i].hostname;
		text[2] = priv->servers->_buffer[i].username;
		text[3] = priv->servers->_buffer[i].iid;
		gtk_clist_append (GTK_CLIST(priv->clist), text);
	}

	gtk_clist_thaw(priv->clist);
}

#else
static void
add_gnorba_objects (GnomeBonoboSelector *widget) 
{
	gchar *text[3];
	GList *list = NULL;
	GnomeBonoboSelectorPrivate *priv;
	
	text[2] = NULL;
	
	g_return_if_fail (widget != NULL);
	
	priv = widget->priv;
	
	g_return_if_fail (priv->clist != NULL);
	
	gtk_clist_freeze (GTK_CLIST (priv->clist));
	
	priv->n_servers = 0;
	list = get_filtered_objects (widget);
	
	if (priv->servers == NULL) 
	{
		gtk_clist_thaw (GTK_CLIST (priv->clist));
		return;
	}
	
	while (list != NULL)  
	{
		text[0] = ( (GoadServer *)list->data)->description;
		text[1] = ( (GoadServer *)list->data)->server_id;
		gtk_clist_append (GTK_CLIST (priv->clist), text);
		priv->n_servers++;
		list = list->next;  	
	}

	gtk_clist_thaw (GTK_CLIST (priv->clist));
}

static GList *
get_filtered_objects (GnomeBonoboSelector *widget) 
{
	int i = 0, j = 0, num = 0;
	const gchar **inters;
	GList *objects = NULL;
	int n_inters = 0;
	int n_objects = 0;
	GnomeBonoboSelectorPrivate *priv;
	
	g_return_val_if_fail (widget != NULL, NULL);

	priv = widget->priv;
	
	priv->servers = goad_server_list_get ();
	if (priv->servers == NULL) return NULL;

	if (priv->interfaces_required == NULL) 
	{
		inters = g_malloc (sizeof (gpointer) * 2);
		inters[0] = DEFAULT_INTERFACE;
		inters[1] = NULL;
		n_inters = 1;
	} 
	else 
	{	
		inters = priv->interfaces_required;
		while (inters[i] != NULL)
		{
			n_inters++;
			i++;
		}
	}
	
	while (priv->servers->list[i].repo_id != NULL)
	{
		num = 0;
		
		for (j = 0; j < n_inters; j++) 
		{
			if (stringlist_contains (
				priv->servers->list[i].repo_id, inters[j]))
			{
				num++;
			}
		}
		if (num == n_inters) /* We have a match! */
		{
			objects = g_list_prepend (objects, 
				&priv->servers->list[i]);
			n_objects++;
		}
		i++;
	}
	objects = g_list_reverse (objects);

	/* Free our temporary criteria */
	if (priv->interfaces_required == NULL) g_free (inters); 
	
	return objects;
}

static gboolean
stringlist_contains (gchar **list, const gchar *word)
{
	int i = 0;
	
	if (list == NULL) return FALSE;
	
	while (list[i] != NULL) 
	{
		if (strcmp (list[i], word) == 0) 
		{
			return TRUE;
		}
		i++;
	}
	return FALSE;
}
#endif
