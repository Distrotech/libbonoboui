/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-selector-widget.c: Bonobo Component Selector widget
 *
 * Authors:
 *   Michael Meeks    (michael@ximian.com)
 *   Richard Hestilow (hestgray@ionet.net)
 *   Miguel de Icaza  (miguel@kernel.org)
 *   Martin Baulig    (martin@
 *   Anders Carlsson  (andersca@gnu.org)
 *   Havoc Pennington (hp@redhat.com)
 *   Dietmar Maurer   (dietmar@maurer-it.com)
 *
 * Copyright 1999, 2001 Richard Hestilow, Ximian, Inc,
 *                      Martin Baulig, Anders Carlsson,
 *                      Havoc Pennigton, Dietmar Maurer
 */
#include <config.h>
#include <string.h> /* strcmp */
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnome/gnome-i18n.h>
#include <bonobo/bonobo-i18n.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-selector-widget.h>
#include <bonobo/bonobo-ui-preferences.h>

#include "bonobo-insert-component.xpm"

#define GET_CLASS(o) BONOBO_SELECTOR_WIDGET_CLASS (GTK_OBJECT_GET_CLASS (o))

static GtkHBoxClass *parent_class;

enum {
	FINAL_SELECT,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

struct _BonoboSelectorWidgetPrivate {
	GtkWidget    *clist;
	GtkWidget    *desc_label;
};

static char *
build_id_query_fragment (const char **required_ids)
{
        const char **required_ids_iter;
	const char **query_components_iter;
        char       **query_components;
	char        *query;
        guint        n_required = 0;

        /* We need to build a query up from the required_ids */
        required_ids_iter = required_ids;

        while (required_ids && *required_ids_iter) {
                ++n_required;
                ++required_ids_iter;
        }

        query_components = g_new0 (gchar*, n_required + 1);

        query_components_iter = (const gchar **) query_components;
        required_ids_iter = required_ids;

        while (*required_ids_iter) {
                *query_components_iter = g_strconcat ("repo_ids.has('",
                                                      *required_ids_iter,
                                                      "')",
                                                      NULL);
                ++query_components_iter;
                ++required_ids_iter;
        }

        query = g_strjoinv (" AND ", query_components);

        g_strfreev (query_components);

	return query;
}

static GSList *
get_lang_list (void)
{
	const GList   *l;
	static GSList *ret = NULL;

	if (!ret) {
		for (l = gnome_i18n_get_language_list (NULL); l; l = l->next)
			ret = g_slist_prepend (ret, l->data);
	}

	return ret;
}

static void
get_filtered_objects (BonoboSelectorWidgetPrivate *priv,
		      const gchar **required_ids)
{
        guint                  i;
        gchar                 *query;
        CORBA_Environment      ev;
        Bonobo_ServerInfoList *servers;
	GSList                *lang_list;
        
        g_return_if_fail (required_ids != NULL);
        g_return_if_fail (*required_ids != NULL);

	query = build_id_query_fragment (required_ids);

	/* FIXME: sorting ? can we get oaf to do it ? - would be nice. */

        CORBA_exception_init (&ev);
        servers = bonobo_activation_query (query, NULL, &ev);
        g_free (query);
        CORBA_exception_free (&ev);

        if (!servers)
                return;

	lang_list = get_lang_list ();

	for (i = 0; i < servers->_length; i++) {
                Bonobo_ServerInfo *oafinfo = &servers->_buffer[i];
		const gchar *name = NULL, *desc = NULL;
		char *text [4];

		name = bonobo_server_info_prop_lookup (oafinfo, "name", lang_list);
		desc = bonobo_server_info_prop_lookup (oafinfo, "description", lang_list);

		if (!name && !desc)
			name = desc = oafinfo->iid;

		if (!name)
			name = desc;

		if (!desc)
			desc = name;

		text [0] = (char *)name;
		text [1] = (char *)oafinfo->iid;
		text [2] = (char *)desc;
		text [3] = NULL;
			
		gtk_clist_append (GTK_CLIST (priv->clist), (gchar **) text);
        }

        CORBA_free (servers);
}

static void
bonobo_selector_widget_finalize (GObject *object)
{
	g_return_if_fail (BONOBO_IS_SELECTOR_WIDGET (object));

	g_free (BONOBO_SELECTOR_WIDGET (object)->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
impl_get_id (BonoboSelectorWidget *sel)
{
	GList *selection;
	gchar *text;
	BonoboSelectorWidgetPrivate *priv; 

	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (!selection)
		return NULL;

	gtk_clist_get_text (GTK_CLIST (priv->clist),
			    GPOINTER_TO_INT (selection->data),
			    1, &text);

	return g_strdup (text);	
}

/**
 * bonobo_selector_widget_get_id:
 * @sel: A BonoboSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the ID of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this. It will give an oaf iid back.
 */
gchar *
bonobo_selector_widget_get_id (BonoboSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_id (sel);
}

static gchar *
impl_get_name (BonoboSelectorWidget *sel)
{
	GList *selection;
	gchar *text;
	BonoboSelectorWidgetPrivate *priv; 

	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (!selection)
		return NULL;

	gtk_clist_get_text (GTK_CLIST (priv->clist),
			    GPOINTER_TO_INT (selection->data),
			    0, &text);

	return g_strdup (text);
}

/**
 * bonobo_selector_widget_get_name:
 * @sel: A BonoboSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the name of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
bonobo_selector_widget_get_name (BonoboSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_name (sel);
}

static gchar *
impl_get_description (BonoboSelectorWidget *sel)
{
	GList *selection;
	gchar *text;
	BonoboSelectorWidgetPrivate *priv; 

	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	
	selection = GTK_CLIST (priv->clist)->selection;
	
	if (!selection)
		return NULL;

	gtk_clist_get_text (GTK_CLIST (priv->clist),
			    GPOINTER_TO_INT (selection->data),
			    2, &text);

	return g_strdup (text);
}

/**
 * bonobo_selector_widget_get_description:
 * @sel: A BonoboSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the description of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
bonobo_selector_widget_get_description (BonoboSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_description (sel);
}

static void
select_row (GtkCList *clist, gint row, gint col, 
	    GdkEvent *event, BonoboSelectorWidget *sel)
{
	if (event && event->type == GDK_2BUTTON_PRESS) {
		gtk_signal_emit (GTK_OBJECT (sel), signals [FINAL_SELECT],
				 NULL);

	} else {
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
bonobo_selector_widget_init (GtkWidget *widget)
{
	BonoboSelectorWidget *sel = BONOBO_SELECTOR_WIDGET (widget);
	GtkWidget *scrolled, *pixmap;
	GtkWidget *hbox;
	GtkWidget *frame;
	BonoboSelectorWidgetPrivate *priv;
	gchar *titles [] = { N_("Name"), "Description", "ID", NULL };
	GdkPixbuf *pixbuf;
	
	g_return_if_fail (sel != NULL);

	titles [0] = gettext (titles [0]);
	priv = sel->priv = g_new0 (BonoboSelectorWidgetPrivate, 1);

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
	gtk_clist_column_titles_passive (GTK_CLIST (priv->clist));

	gtk_container_add (GTK_CONTAINER (scrolled), priv->clist);
	gtk_box_pack_start (GTK_BOX (sel), scrolled, TRUE, TRUE, 0);

	frame = gtk_frame_new (_("Description"));
	gtk_box_pack_start (GTK_BOX (sel), frame, FALSE, TRUE, 0);

	
	priv->desc_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (priv->desc_label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->desc_label), TRUE);
	gtk_label_set_justify (GTK_LABEL (priv->desc_label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new (FALSE, 0);

	pixbuf = gdk_pixbuf_new_from_xpm_data (bonobo_insert_component_xpm);
	pixmap = gtk_image_new_from_pixbuf (pixbuf);
	gdk_pixbuf_unref (pixbuf);

	gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, TRUE, BONOBO_UI_PAD_SMALL);
	
	gtk_box_pack_start (GTK_BOX (hbox), priv->desc_label, TRUE, TRUE, BONOBO_UI_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	
	gtk_widget_set_usize (widget, 400, 300); 
	gtk_widget_show_all (widget);
}

static void
impl_set_interfaces (BonoboSelectorWidget *widget,
		     const char           **required_interfaces)
{
	BonoboSelectorWidgetPrivate *priv;
	
	g_return_if_fail (widget != NULL);
	
	priv = widget->priv;
	
	g_return_if_fail (priv->clist != NULL);
	
	gtk_clist_freeze (GTK_CLIST (priv->clist));

	gtk_clist_clear (GTK_CLIST (priv->clist));

	get_filtered_objects (priv, required_interfaces);

	gtk_clist_thaw (GTK_CLIST (priv->clist));
}

void
bonobo_selector_widget_set_interfaces (BonoboSelectorWidget *widget,
				       const char           **required_interfaces)
{
	GET_CLASS (widget)->set_interfaces (widget, required_interfaces);
}

/**
 * bonobo_selector_widget_new:
 *
 * Creates a new BonoboSelectorWidget widget, this contains
 * a CList and a description pane for each component.
 *
 * Returns: A pointer to the newly-created BonoboSelectorWidget widget.
 */
GtkWidget *
bonobo_selector_widget_new (void)
{
	return gtk_type_new (bonobo_selector_widget_get_type ());
}

static void
bonobo_selector_widget_class_init (BonoboSelectorWidgetClass *klass)
{
	GObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	
	object_class = (GObjectClass *) klass;
	parent_class = gtk_type_class (gtk_vbox_get_type ());

	klass->get_id          = impl_get_id;
	klass->get_name        = impl_get_name;
	klass->get_description = impl_get_description;
	klass->set_interfaces  = impl_set_interfaces;

	signals [FINAL_SELECT] = gtk_signal_new (
		"final_select", GTK_RUN_FIRST, GTK_CLASS_TYPE (object_class),
		GTK_SIGNAL_OFFSET (BonoboSelectorWidgetClass, final_select),
		gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);

	object_class->finalize = bonobo_selector_widget_finalize;
}

/**
 * bonobo_selector_widget_get_type:
 *
 * Returns: The GtkType for the BonoboSelectorWidget object class.
 */
GtkType
bonobo_selector_widget_get_type (void)
{
	static GtkType bonobo_selector_widget_type = 0;

	if (!bonobo_selector_widget_type) {
		GtkTypeInfo bonobo_selector_widget_info = {
			"BonoboSelectorWidget",
			sizeof (BonoboSelectorWidget),
			sizeof (BonoboSelectorWidgetClass),
			(GtkClassInitFunc)  bonobo_selector_widget_class_init,
			(GtkObjectInitFunc) bonobo_selector_widget_init,
			NULL,
			NULL
		};

		bonobo_selector_widget_type = gtk_type_unique (
			gtk_vbox_get_type (),
			&bonobo_selector_widget_info);
	}

	return bonobo_selector_widget_type;
}
