/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include "gnome-bonobo-selector.h"
#include <string.h> /* strcmp */

#define DEFAULT_INTERFACE	"IDL:GNOME/Embeddable:1.0"

static GtkDialogClass *parent_class;

struct _GnomeBonoboSelectorPrivate 
{
	GtkWidget *clist;
	GoadServerList *servers;
	int n_servers;
	const gchar **interfaces_required;
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
static void add_gnorba_objects (GnomeBonoboSelector *widget); 
static GList *get_filtered_objects (GnomeBonoboSelector *widget);
static gboolean stringlist_contains (gchar **list, const gchar *word);


/* fixme: revove this as soon it is included in gnome-dialog */
void       
gnome_dialog_clicked (GnomeDialog *dialog, gint button_num)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "clicked", button_num);
}              


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
 *
 * Creates a new GnomeBonoboSelector widget.  The title of the dialog
 * is set to @title, and the list of selectable servers is populated
 * with those servers which support the interfaces specified in
 * @interfaces_required.
 *
 * Returns: A pointer to the newly-created GnomeBonoboSelector widget.
 */
GtkWidget *
gnome_bonobo_selector_new (const gchar *title,
			   const gchar **interfaces_required)
{
	GnomeBonoboSelector *sel;
	GnomeBonoboSelectorPrivate *priv;

	if (title == NULL) title = "";

	
	sel = gtk_type_new (gnome_bonobo_selector_get_type ());
	priv = sel->priv;
	priv->interfaces_required = interfaces_required;
	add_gnorba_objects (sel);
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
	goad_server_list_free (priv->servers);
	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		 (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	
}

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
{
	GList *selection;
	gchar *text;
	GnomeBonoboSelectorPrivate *priv; 
	
	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (selection == NULL) return NULL;
	gtk_clist_get_text (GTK_CLIST (priv->clist), (int) selection->data,
			1, &text);
	return g_strdup (text);
}

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
{
	GtkWidget *sel = gnome_bonobo_selector_new (title, interfaces_required);
	gchar *name = NULL;
	int n;

	if (sel == NULL)
		return NULL;

	gtk_signal_connect (GTK_OBJECT (sel),
		"ok", GTK_SIGNAL_FUNC (ok_callback), NULL);
	gtk_signal_connect (GTK_OBJECT (sel),
		"cancel", GTK_SIGNAL_FUNC (cancel_callback), NULL);
	
	gtk_object_set_user_data (GTK_OBJECT (sel), NULL);
	
	gtk_widget_show (sel);
		
	n = gnome_dialog_run (GNOME_DIALOG(sel));
	if (n == -1)
		return NULL;
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
	char *text = gnome_bonobo_selector_get_selected_goad_id (
		GNOME_BONOBO_SELECTOR (widget));
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
	gchar *titles[] = { N_("Bonobo object description"), "goadid", NULL };
	
	g_return_if_fail (widget != NULL);
	
	sel->priv = g_new0 (GnomeBonoboSelectorPrivate, 1);
	priv = sel->priv;
	
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	titles[0] = gettext(titles[0]);

	priv->clist = gtk_clist_new_with_titles (2, titles);
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
