/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo accessibility wrappers
 *
 * FIXME: this implementation sucks almost as badly
 *        as the AtkObjectFactory design does.
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */

#include <atk/atkregistry.h>
#include <atk/atkobjectfactory.h>
#include <gtk/gtkwidget.h>
#include <bonobo-a11y.h>

typedef AtkObject * (*BonoboA11YGetAccessible) (GtkWidget *widget);

/* To chain to parent get_accessible methods */
static GHashTable *chain_hash = NULL;

/* To map a11y types to GtkWidget types */
static GHashTable *type_hash = NULL;

static AtkObject *
bonobo_a11y_get_accessible (GtkWidget *widget)
{
	AtkObject *accessible;

	static GQuark quark_accessible_object = 0;

	if (!quark_accessible_object)
		quark_accessible_object = g_quark_from_static_string (
			"gtk-accessible-object");

	accessible = g_object_get_qdata (G_OBJECT (widget), 
					 quark_accessible_object);
	if (!accessible) {
		GType t;

		t = (GType) g_hash_table_lookup (
			type_hash, (gpointer) G_TYPE_FROM_INSTANCE (widget));

		if (t) {
			accessible = g_object_new (t, NULL);

			atk_object_initialize (accessible, widget);

			g_object_set_qdata (G_OBJECT (widget), 
					    quark_accessible_object,
					    accessible);
		} else {
			BonoboA11YGetAccessible parent_impl_fn;

			parent_impl_fn = g_hash_table_lookup (
				chain_hash, (gpointer) G_TYPE_FROM_INSTANCE (widget));

			if (!parent_impl_fn)
				g_error ("Serious chain hash error '%s'",
					 g_type_name_from_instance (
						 (GTypeInstance *) widget));
			else
				accessible = parent_impl_fn (widget);
		}
	}

	return accessible;
}

static void
recursive_a11y_clobber (GType type)
{
	guint n_children, i;
	GType *children;
	GtkWidgetClass *w_class;

	w_class = g_type_class_peek (type);

	if (!w_class)
		/* not instantiated yet, no problems */
		return;

	g_hash_table_insert (chain_hash, (gpointer) type,
			     w_class->get_accessible);

	w_class->get_accessible = bonobo_a11y_get_accessible;

	children = g_type_children (type, &n_children);

	for (i = 0; i < n_children; i++)
		recursive_a11y_clobber (children [i]);
}

/**
 * bonobo_a11y_clobber_atk_junk_code:
 * @void: 
 * 
 *   This code is here because AtkObjectFactory sucks
 * great rocks through a very small straw, and shows
 * signs of a total lack of thought / design.
 *
 *   It seems the only way to work around the acute
 * lack of forethought is to clobber everything
 * derived from GtkWidget so it can't hurt us.
 **/
void
bonobo_a11y_clobber_atk_junk_code (void)
{
	GType widget_type;

	if (type_hash)
		return;

	type_hash = g_hash_table_new (NULL, NULL);
	chain_hash = g_hash_table_new (NULL, NULL);

	widget_type = gtk_widget_get_type ();

	/*
	 * Leak this - but we need to keep that
	 * instantiated, since we don't want it
	 * re-constructed with the original method
	 */
	g_type_class_ref (widget_type);

	recursive_a11y_clobber (widget_type);
}

void
bonobo_a11y_register_type_for (GType atk_object_type,
			       GType gtk_widget_type)
{
	g_return_if_fail (type_hash != NULL);

	g_hash_table_insert (type_hash,
			     (gpointer) atk_object_type,
			     (gpointer) gtk_widget_type);
}

GType
bonobo_a11y_get_derived_type_for (GType          widget_type,
				  GClassInitFunc class_init)
{
	GType type;
	GType parent_widget_type;
	GType parent_atk_type;
	GTypeInfo tinfo = { 0 };
	GTypeQuery query;
	char *type_name;

	parent_widget_type = g_type_parent (widget_type);

	/* Find the AtkObject type to derive from */
	parent_atk_type = (GType) g_hash_table_lookup (
		type_hash, (gpointer) parent_widget_type);
	if (!parent_atk_type) {
		AtkObjectFactory *factory;

		factory = atk_registry_get_factory (
			atk_get_default_registry (),
			parent_widget_type);
		parent_atk_type = atk_object_factory_get_accessible_type (factory);
	}
		
	/*
	 * Figure out the size of the class and instance 
	 * we are deriving from
	 */
	g_type_query (parent_atk_type, &query);

	tinfo.class_init    = class_init;
	tinfo.class_size    = query.class_size;
	tinfo.instance_size = query.instance_size;

	/* Make up a name */
	type_name = g_strconcat (g_type_name (widget_type),
				 "Accessible", NULL);

	/* Register the type */
	type = g_type_register_static (
		parent_atk_type, type_name, &tinfo, 0);

	g_free (type_name);
		
	return type;
}

