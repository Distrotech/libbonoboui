#include <config.h>
#include <gtk/gtk.h>
#include <bonobo/bonobo-ui-private.h>

typedef struct {
	GtkToolbar     parent;

	gboolean       got_size;
	GtkRequisition full_size;
} InternalToolbar;

typedef struct {
	GtkToolbarClass parent_class;
} InternalToolbarClass;

GType internal_toolbar_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE(InternalToolbar, internal_toolbar, GTK_TYPE_TOOLBAR);

enum {
	PROP_0,
	PROP_IS_FLOATING,
	PROP_ORIENTATION,
	PROP_PREFERRED_WIDTH,
	PROP_PREFERRED_HEIGHT
};

static void
get_full_size (InternalToolbar *toolbar)
{
	if (!toolbar->got_size) {
		gboolean show_arrow;

		toolbar->got_size = TRUE;

		show_arrow = gtk_toolbar_get_show_arrow (GTK_TOOLBAR (toolbar));
		if (show_arrow) /* Not an elegant approach, sigh. */
			g_object_set (toolbar, "show_arrow", FALSE, NULL);

		gtk_widget_size_request (GTK_WIDGET (toolbar), &toolbar->full_size);

		if (show_arrow)
			g_object_set (toolbar, "show_arrow", TRUE, NULL);
	}
}

static void
invalidate_size (InternalToolbar *toolbar)
{
	toolbar->got_size = FALSE;	
}

static void
impl_get_property (GObject    *object,
		   guint       property_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	InternalToolbar *toolbar = (InternalToolbar *) object;

	get_full_size (toolbar);

	switch (property_id) {
	case PROP_PREFERRED_WIDTH:
		g_value_set_uint (value, toolbar->full_size.width);
		break;
	case PROP_PREFERRED_HEIGHT:
		g_value_set_uint (value, toolbar->full_size.height);
		break;
	default:
		break;
	};
}

static void
impl_set_property (GObject      *object,
		   guint         property_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	GtkToolbar *toolbar = GTK_TOOLBAR (object);

	invalidate_size ((InternalToolbar *) toolbar);

	switch (property_id) {
	case PROP_ORIENTATION:
		gtk_toolbar_set_orientation (toolbar, 
					     g_value_get_enum (value));
		break;
	case PROP_IS_FLOATING:
		gtk_toolbar_set_show_arrow (toolbar, !g_value_get_boolean (value));
		break;
	default:
		break;
	};
}

static void
internal_toolbar_class_init (InternalToolbarClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;

	gobject_class->get_property = impl_get_property;
	gobject_class->set_property = impl_set_property;

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_WIDTH,
		g_param_spec_uint ("preferred_width", NULL, NULL,
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_HEIGHT,
		g_param_spec_uint ("preferred_height",
				   NULL, NULL,
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_IS_FLOATING,
		g_param_spec_boolean ("is_floating",
				      NULL, NULL,
				      FALSE, G_PARAM_WRITABLE));
}

static void
internal_toolbar_init (InternalToolbar *toolbar)
{
	g_signal_connect (toolbar, "add",
			  G_CALLBACK (invalidate_size), NULL);
	g_signal_connect (toolbar, "remove",
			  G_CALLBACK (invalidate_size), NULL);
	g_signal_connect (toolbar, "orientation_changed",
			  G_CALLBACK (invalidate_size), NULL);
}

GtkWidget *
bonobo_ui_internal_toolbar_new (void)
{
	return g_object_new (internal_toolbar_get_type(), NULL);
}
