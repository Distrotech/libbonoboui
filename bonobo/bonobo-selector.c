/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include "bonobo-selector.h"
#include <string.h> /* strcmp */
#include <gnome.h>
#include "bonobo-object-directory.h"
#include "bonobo-insert-component.xpm"

#define DEFAULT_INTERFACE	"IDL:Bonobo/Embeddable:1.0"

static GtkDialogClass *parent_class;

struct _BonoboSelectorPrivate 
{
	GtkWidget *clist;
	GtkWidget *desc_label;
	GList *servers;
	int n_servers;
	const gchar **interfaces_required;
};

enum {
	OK,
	CANCEL,
	LAST_SIGNAL
};

guint bonobo_selector_signals[LAST_SIGNAL] = { 0, 0 };

static void bonobo_selector_class_init (BonoboSelectorClass *klass);
static void bonobo_selector_init (GtkWidget *widget);
static void bonobo_selector_destroy (GtkObject *object);
static void button_callback (GtkWidget *widget, gint button_number,
			     gpointer data);
static void ok_callback (GtkWidget *widget, gpointer data);
static void cancel_callback (GtkWidget *widget, gpointer data);
static void add_objects (BonoboSelector *widget); 
static GList *get_filtered_objects (BonoboSelector *widget);


/* fixme: revove this as soon it is included in gnome-dialog */
static void       
gnome_dialog_clicked (GnomeDialog *dialog, gint button_num)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "clicked", button_num);
}              


static void
bonobo_selector_class_init (BonoboSelectorClass *klass)
{
	GtkObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	
	object_class = (GtkObjectClass *) klass;
	parent_class = gtk_type_class (gnome_dialog_get_type ());

	bonobo_selector_signals[OK] =
		gtk_signal_new ("ok", GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (BonoboSelectorClass, ok),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	bonobo_selector_signals[CANCEL] =
		gtk_signal_new ("cancel", GTK_RUN_LAST, object_class->type,
		GTK_SIGNAL_OFFSET (BonoboSelectorClass, cancel),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, bonobo_selector_signals,
		LAST_SIGNAL);
	
	object_class->destroy = bonobo_selector_destroy;
}

/**
 * bonobo_selector_get_type:
 *
 * Returns: The GtkType for the BonoboSelector object class.
 */
GtkType
bonobo_selector_get_type (void)
{
	static guint bonobo_selector_type = 0;

	if (!bonobo_selector_type)
	{
		GtkTypeInfo bonobo_selector_info =
		{
			"BonoboSelector",
			sizeof (BonoboSelector),
			sizeof (BonoboSelectorClass),
			 (GtkClassInitFunc) bonobo_selector_class_init,
			 (GtkObjectInitFunc) bonobo_selector_init,
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
 * bonobo_selector_new:
 * @title: A string which should go in the title of the
 * BonoboSelector window.
 * @interfaces_required: A NULL_terminated array of interfaces which a
 * server must support in order to be listed in the selector.  Defaults
 * to "IDL:Bonobo/Embeddable:1.0" if no interfaces are listed.
 *
 * Creates a new BonoboSelector widget.  The title of the dialog
 * is set to @title, and the list of selectable servers is populated
 * with those servers which support the interfaces specified in
 * @interfaces_required.
 *
 * Returns: A pointer to the newly-created BonoboSelector widget.
 */
GtkWidget *
bonobo_selector_new (const gchar *title,
		     const gchar **interfaces_required)
{
	BonoboSelector *sel;
	BonoboSelectorPrivate *priv;

	if (title == NULL) title = "";
	
	sel = gtk_type_new (bonobo_selector_get_type ());
	priv = sel->priv;
	priv->interfaces_required = interfaces_required;
	add_objects (sel);
	gtk_window_set_title (GTK_WINDOW (sel), title);
	return GTK_WIDGET (sel);
}

static void
bonobo_selector_destroy (GtkObject *object)
{
	BonoboSelector *sel;
	BonoboSelectorPrivate *priv; 
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_SELECTOR (object));

	sel = BONOBO_SELECTOR (object);
	priv = sel->priv;

	gtk_widget_destroy (priv->clist);
	od_server_list_free (priv->servers);
	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		 (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);	
}

/**
 * bonobo_selector_get_selected_id:
 * @sel: A BonoboSelector widget.
 *
 * Returns: A newly-allocated string containing the ID of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this. It will give an oaf iid back.
 */
gchar *
bonobo_selector_get_selected_id (BonoboSelector *sel)
{
	GList *selection;
	gchar *text;
	BonoboSelectorPrivate *priv; 

	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (selection == NULL) return NULL;
	gtk_clist_get_text (GTK_CLIST (priv->clist), (int) selection->data,
			1, &text);
	return g_strdup (text);
}

/**
 * gnome_bonobo_select_id:
 * @title: The title to be used for the dialog.
 * @interfaces_required: A list of required interfaces.  See
 * bonobo_selector_new().
 *
 * Calls bonobo_selector_new() to create a new
 * BonoboSelector widget with the specified paramters, @title and
 * @interfaces_required.  Then runs the dialog modally and allows
 * the user to make a selection.
 *
 * Returns: The Oaf IID of the selected server, or NULL if no server is
 * selected.  The ID string has been allocated with g_strdup.
 */
gchar *
bonobo_selector_select_id (const gchar *title,
			   const gchar **interfaces_required)
{
	GtkWidget *sel = bonobo_selector_new (title, interfaces_required);
	gchar *name = NULL;
	int n;

	g_return_val_if_fail (sel != NULL, NULL);

	gtk_signal_connect (GTK_OBJECT (sel), "ok",
			    GTK_SIGNAL_FUNC (ok_callback), NULL);
	gtk_signal_connect (GTK_OBJECT (sel), "cancel",
			    GTK_SIGNAL_FUNC (cancel_callback), NULL);
	
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
	switch (button_number) {
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
	char *text = bonobo_selector_get_selected_id (
		BONOBO_SELECTOR (widget));
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
	    GdkEvent *event, BonoboSelector *sel)
{
	if (event && event->type == GDK_2BUTTON_PRESS)
		gnome_dialog_clicked ( GNOME_DIALOG (sel), 0);
	else {
		GtkCListClass *cl;
		gchar *text;
		
		gtk_clist_get_text (GTK_CLIST (clist), row,
				    2, &text);
		gtk_label_set_text (GTK_LABEL (sel->priv->desc_label), text);
		
		cl = gtk_type_class (GTK_TYPE_CLIST);

		if (cl->select_row)
			cl->select_row (clist, row, col, event);
	}
}

static void
bonobo_selector_init (GtkWidget *widget)
{
	BonoboSelector *sel = BONOBO_SELECTOR (widget);
	GtkWidget *scrolled, *pixmap;
	GtkWidget *hbox;
	GtkWidget *frame;
	
	BonoboSelectorPrivate *priv;
	gchar *titles[] = { N_("Name"), "Description", "ID", NULL };
	
	g_return_if_fail (widget != NULL);

	titles[0] = gettext (titles[0]);
	sel->priv = g_new0 (BonoboSelectorPrivate, 1);
	priv = sel->priv;

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	priv->clist = gtk_clist_new_with_titles (3, titles);
	gtk_clist_set_selection_mode (GTK_CLIST (priv->clist),
		GTK_SELECTION_BROWSE);
	gtk_signal_connect (GTK_OBJECT (priv->clist), "select-row",
			    GTK_SIGNAL_FUNC (select_row), sel);
	gtk_clist_set_column_visibility (GTK_CLIST (priv->clist), 1, FALSE);
	gtk_clist_set_column_visibility (GTK_CLIST (priv->clist), 2, FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled), priv->clist);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sel)->vbox), scrolled, TRUE, TRUE, 0);

	frame = gtk_frame_new (_("Description"));
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (sel)->vbox), frame, FALSE, TRUE, 0);

	
	priv->desc_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (priv->desc_label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->desc_label), TRUE);
	gtk_label_set_justify (GTK_LABEL (priv->desc_label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new (FALSE, 0);

	pixmap = gnome_pixmap_new_from_xpm_d (bonobo_insert_component_xpm);
	gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, TRUE, GNOME_PAD_SMALL);
	
	gtk_box_pack_start (GTK_BOX (hbox), priv->desc_label, TRUE, TRUE, GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	
	gnome_dialog_append_button (GNOME_DIALOG (sel), GNOME_STOCK_BUTTON_OK);
	gnome_dialog_append_button (GNOME_DIALOG (sel), 
		GNOME_STOCK_BUTTON_CANCEL);
	
	gtk_signal_connect (GTK_OBJECT (sel),
		"clicked", GTK_SIGNAL_FUNC (button_callback), sel);
	gtk_signal_connect (GTK_OBJECT (sel), "close",
		GTK_SIGNAL_FUNC (button_callback), sel);
	
	gtk_widget_set_usize (widget, 400, 300); 
	gtk_widget_show_all (GNOME_DIALOG (sel)->vbox);
}

static void
add_objects (BonoboSelector *widget) 
{
	const gchar *text [4];
	GList *list = NULL;
	BonoboSelectorPrivate *priv;

	text[3] = NULL;

	g_return_if_fail (widget != NULL);
	
	priv = widget->priv;
	
	g_return_if_fail (priv->clist != NULL);
	
	gtk_clist_freeze (GTK_CLIST (priv->clist));
	
	priv->n_servers = 0;
	list = get_filtered_objects (widget);
	
	if (priv->servers == NULL) {
		gtk_clist_thaw (GTK_CLIST (priv->clist));
		return;
	}
	
	while (list != NULL) {
		text[0] = od_server_info_get_name(list->data);
		text[1] = od_server_info_get_id(list->data);
		text[2] = od_server_info_get_description(list->data);
		
		gtk_clist_append (GTK_CLIST (priv->clist), (gchar**)text);
		priv->n_servers++;
		list = list->next;  	
	}

	gtk_clist_thaw (GTK_CLIST (priv->clist));
}

static gint
server_list_compare (gconstpointer a, gconstpointer b)
{
	return strcmp (od_server_info_get_name ((ODServerInfo *)a),
		       od_server_info_get_name ((ODServerInfo *)b));

}

static GList *
get_filtered_objects (BonoboSelector *widget) 
{
	int i = 0;
	const gchar **inters;
	BonoboSelectorPrivate *priv;
	int n_inters = 0;
	
	g_return_val_if_fail (widget != NULL, NULL);

	priv = widget->priv;

	if (priv->interfaces_required == NULL) {
		inters = g_malloc (sizeof (gpointer) * 2);
		inters[0] = DEFAULT_INTERFACE;
		inters[1] = NULL;
		n_inters = 1;
	} else {	
		inters = priv->interfaces_required;
		while (inters[i] != NULL) {
			n_inters++;
			i++;
		}
	}

	priv->servers = od_get_server_list(inters);

	/* Free our temporary criteria */
	if (priv->interfaces_required == NULL)
		g_free (inters);
	
	return g_list_sort (priv->servers, server_list_compare);
}
