/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-selector.c: Bonobo Component Selector
 *
 * Authors:
 *   Richard Hestilow (hestgray@ionet.net)
 *   Miguel de Icaza  (miguel@kernel.org)
 *   Martin Baulig    (martin@home-of-linux.org)
 *   Anders Carlsson  (andersca@gnu.org)
 *   Havoc Pennington (hp@redhat.com)
 *   Dietmar Maurer   (dietmar@ximian.com)
 *   Michael Meeks    (michael@ximian.com)
 *
 * Copyright 1999, 2000 Richard Hestilow, Ximian, Inc,
 *                      Martin Baulig, Anders Carlsson,
 *                      Havoc Pennigton, Dietmar Maurer
 */
#include <config.h>
#include <string.h> /* strcmp */
#include <libgnome/gnome-macros.h>
#include <bonobo/bonobo-selector.h>

GNOME_CLASS_BOILERPLATE (BonoboSelector, bonobo_selector,
			 GtkDialog, GTK_TYPE_DIALOG);

#define DEFAULT_INTERFACE "IDL:Bonobo/Control:1.0"
#define BONOBO_PAD_SMALL 4

struct _BonoboSelectorPrivate {
	BonoboSelectorWidget *selector;
};

enum {
	OK,
	CANCEL,
	LAST_SIGNAL
};

guint bonobo_selector_signals [LAST_SIGNAL] = { 0, 0 };

static void       
bonobo_selector_finalize (GObject *object)
{
	g_return_if_fail (BONOBO_IS_SELECTOR (object));

	g_free (BONOBO_SELECTOR (object)->priv);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
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
	g_return_val_if_fail (BONOBO_IS_SELECTOR (sel), NULL);

	return bonobo_selector_widget_get_id (sel->priv->selector);
}

/**
 * bonobo_selector_get_selected_name:
 * @sel: A BonoboSelector widget.
 *
 * Returns: A newly-allocated string containing the name of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
bonobo_selector_get_selected_name (BonoboSelector *sel)
{
	g_return_val_if_fail (BONOBO_IS_SELECTOR (sel), NULL);

	return bonobo_selector_widget_get_name (sel->priv->selector);
}

/**
 * bonobo_selector_get_selected_description:
 * @sel: A BonoboSelector widget.
 *
 * Returns: A newly-allocated string containing the description of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
bonobo_selector_get_selected_description (BonoboSelector *sel)
{
	g_return_val_if_fail (BONOBO_IS_SELECTOR (sel), NULL);

	return bonobo_selector_widget_get_description (sel->priv->selector);
}

static void
ok_callback (GtkWidget *widget, gpointer data)
{
	char *text = bonobo_selector_get_selected_id (
		BONOBO_SELECTOR (widget));

	gtk_object_set_user_data (GTK_OBJECT (widget), text);
}

/**
 * bonobo_selector_select_id:
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
bonobo_selector_select_id (const gchar  *title,
			   const gchar **interfaces_required)
{
	GtkWidget *sel = bonobo_selector_new (title, interfaces_required);
	gchar     *name = NULL;
	int        n;

	g_return_val_if_fail (sel != NULL, NULL);

	g_signal_connect (GTK_OBJECT (sel), "ok",
			    G_CALLBACK (ok_callback), NULL);
	
	gtk_object_set_user_data (GTK_OBJECT (sel), NULL);
	
	gtk_widget_show (sel);
		
	n = gtk_dialog_run (GTK_DIALOG (sel));

	switch (n) {
	case GTK_RESPONSE_CANCEL:
		name = NULL;
		break;
	case GTK_RESPONSE_APPLY:
	case GTK_RESPONSE_OK:
		name = gtk_object_get_user_data (GTK_OBJECT (sel));
		break;
	default:
		break;
	}
		
	gtk_widget_destroy (sel);

	return name;
}

static void
response_callback (GtkWidget *widget,
		   gint       response_id,
		   gpointer   data) 
{
	switch (response_id) {
		case GTK_RESPONSE_OK:
			g_signal_emit (data, bonobo_selector_signals [OK], 0);
			break;
		case GTK_RESPONSE_CANCEL:
			g_signal_emit (data, bonobo_selector_signals [CANCEL], 0);
		default:
			break;
	}
}

static void
final_select_cb (GtkWidget *widget, BonoboSelector *sel)
{
	gtk_dialog_response (GTK_DIALOG (sel), GTK_RESPONSE_OK);
}

static void
bonobo_selector_instance_init (BonoboSelector *sel)
{
	sel->priv = g_new0 (BonoboSelectorPrivate, 1);
}

static void
bonobo_selector_class_init (BonoboSelectorClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = bonobo_selector_finalize;

	bonobo_selector_signals [OK] =
		g_signal_new ("ok", G_SIGNAL_RUN_LAST,
			      G_TYPE_FROM_CLASS (object_class),
			      G_STRUCT_OFFSET (BonoboSelectorClass, ok),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	
	bonobo_selector_signals [CANCEL] =
		g_signal_new ("cancel", G_SIGNAL_RUN_LAST,
			      G_TYPE_FROM_CLASS (object_class),
			      G_STRUCT_OFFSET (BonoboSelectorClass, cancel),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * bonobo_selector_construct:
 * @sel: the selector to construct
 * @title: the title for the window
 * @selector: the component view widget to put inside it.
 * 
 * Constructs the innards of a bonobo selector window.
 * 
 * Return value: the constructed widget.
 **/
GtkWidget *
bonobo_selector_construct (BonoboSelector       *sel,
			   const gchar          *title,
			   BonoboSelectorWidget *selector)
{
	g_return_val_if_fail (BONOBO_IS_SELECTOR (sel), NULL);
	g_return_val_if_fail (BONOBO_IS_SELECTOR_WIDGET (selector), NULL);

	sel->priv->selector = selector;

	g_signal_connect (GTK_OBJECT (selector), "final_select",
			    G_CALLBACK (final_select_cb), sel);
	
	gtk_window_set_title (GTK_WINDOW (sel), title ? title : "");

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (sel)->vbox),
			    GTK_WIDGET (selector),
			    TRUE, TRUE, BONOBO_PAD_SMALL);
	
	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response (GTK_DIALOG (sel), GTK_RESPONSE_OK);
	
	g_signal_connect (GTK_OBJECT (sel), "response",
			    G_CALLBACK (response_callback), sel);
	
	gtk_window_set_default_size (GTK_WINDOW (sel), 400, 300);
	gtk_widget_show_all  (GTK_DIALOG (sel)->vbox);

	return GTK_WIDGET (sel);
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
	const gchar *query [2] = { DEFAULT_INTERFACE, NULL };
	BonoboSelector *sel;
	BonoboSelectorWidget *selector;

	selector = BONOBO_SELECTOR_WIDGET (bonobo_selector_widget_new ());

	if (!interfaces_required)
		interfaces_required = query;

	bonobo_selector_widget_set_interfaces (selector, interfaces_required);

	sel = g_object_new (bonobo_selector_get_type (), 0);

	return bonobo_selector_construct (sel, title, selector);
}
