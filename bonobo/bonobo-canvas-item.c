/*
 * gnome-bonobo-item.c: Canvas item implementation for embedding remote canvas-items
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999 International GNOME Support
 */
#include <config.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-bonobo-item.h>
#include <bonobo/gnome-object.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

static GnomeCanvasItemClass *gbi_parent_class;

struct _GnomeBonoboItemPrivate {
	GNOME_Canvas_Item object;
};

enum {
	ARG_0,
	ARG_CORBA_CANVAS_ITEM,
};

/*
 * Horizontal space saver
 */
#define GBI(x)          GNOME_BONOBO_ITEM(x)
typedef GnomeBonoboItem Gbi;

/*
 * Creates a GNOME_Canvas_SVPSegment structure representing the ArtSVPSeg
 * structure, suitable for sending over the network
 */
static gboolean
art_svp_segment_to_CORBA_SVP_Segment (ArtSVPSeg *seg, GNOME_Canvas_SVPSegment *segment)
{
	int i;
	
	segment->points._buffer = CORBA_sequence_GNOME_Canvas_Point_allocbuf (seg->n_points);
	if (segment->points._buffer == NULL)
		return FALSE;

	segment->points._maximum = seg->n_points;
	segment->points._length = seg->n_points;
	
	if (seg->dir == 0)
		segment->up = CORBA_TRUE;
	else
		segment->up = CORBA_FALSE;

	segment->bbox.x0 = seg->bbox.x0;
	segment->bbox.x1 = seg->bbox.x1;
	segment->bbox.y0 = seg->bbox.y0;
	segment->bbox.y1 = seg->bbox.y1;

	for (i = 0; i < seg->n_points; i++){
		segment->points._buffer [i].x = seg->points [i].x;
		segment->points._buffer [i].y = seg->points [i].y;
	}

	return TRUE;
}

/*
 * Creates a GNOME_Canvas_SVP CORBA structure from the art_svp, suitable
 * for sending over the wire
 */
static GNOME_Canvas_SVP *
art_svp_to_CORBA_SVP (ArtSVP *art_svp)
{
	GNOME_Canvas_SVP *svp;
	int i;
	
	svp = GNOME_Canvas_SVP__alloc ();
	if (!svp)
		return NULL;
	
	if (art_svp){
		svp->_buffer = CORBA_sequence_GNOME_Canvas_SVPSegment_allocbuf (art_svp->n_segs);
		if (svp->_buffer == NULL){
			svp->_length = 0;
			svp->_maximum = 0;
			return svp;
		}
		svp->_maximum = art_svp->n_segs;
		svp->_length = art_svp->n_segs;

		for (i = 0; i < art_svp->n_segs; i++){
			gboolean ok;
			
			ok = art_svp_segment_to_CORBA_SVP_Segment (
				&art_svp->segs [i], &svp->_buffer [i]);
			if (!ok){
				int j;
				
				for (j = 0; j < i; j++)
					CORBA_free (&svp->_buffer [j]);
				CORBA_free (svp);
				return NULL;
			}
		}
	} else {
		svp->_maximum = 0;
		svp->_length = 0;
	}

	return svp;
}
	      
static void
gbi_update (GnomeCanvasItem *item, double *item_affine, ArtSVP *item_clip_path, int item_flags)
{
	Gbi *gbi = GBI (item);
	GNOME_Canvas_affine affine;
	GNOME_Canvas_SVP *clip_path = NULL;
	CORBA_Environment ev;
	CORBA_boolean queue_update;
	CORBA_double x1, y1, x2, y2;
	int i;

	if (getenv ("DEBUG_BI"))
		printf ("gbi_update\n");

	if (gbi_parent_class)
		(*gbi_parent_class->update)(item, item_affine, item_clip_path, item_flags);
	
	for (i = 0; i < 5; i++)
		affine [i] = item_affine [i];

	clip_path = art_svp_to_CORBA_SVP (item_clip_path);
	if (!clip_path)
		return;

	CORBA_exception_init (&ev);
	queue_update = GNOME_Canvas_Item_update (
		gbi->priv->object, affine, clip_path, item_flags,
		&x1, &y1, &x2, &y2,
		&ev);

	if (queue_update)
		gnome_canvas_item_request_update (item);

	item->x1 = x1;
	item->y1 = y1;
	item->x2 = x2;
	item->y2 = y2;

	if (getenv ("DEBUG_BI"))
		printf ("Bbox: %g %g %g %g\n", x1, y1, x2, y2);
	
	CORBA_exception_free (&ev);

	CORBA_free (clip_path);
}

static void
gbi_realize (GnomeCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;

	if (getenv ("DEBUG_BI"))
		printf ("gbi_realize\n");
	
	if (gbi_parent_class)
		(*gbi_parent_class->realize) (item);
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_realize (gbi->priv->object, &ev);
	CORBA_exception_free (&ev);
}

static void
gbi_unrealize (GnomeCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		printf ("gbi_unrealize\n");
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_unrealize (gbi->priv->object, &ev);
	CORBA_exception_free (&ev);

	if (gbi_parent_class)
		(*gbi_parent_class->unrealize) (item);
}

static void
gbi_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		printf ("gbi_draw\n");
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_draw (gbi->priv->object, GDK_WINDOW_XWINDOW (drawable), x, y, width, height, &ev);
	CORBA_exception_free (&ev);
}

static double
gbi_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		printf ("gbi_point %g %g\n", x, y);
	
	CORBA_exception_init (&ev);
	if (GNOME_Canvas_Item_contains (gbi->priv->object, x, y, &ev)){
		CORBA_exception_free (&ev);
		*actual = item;
		return 0.0;
	}
	CORBA_exception_free (&ev);

	*actual = NULL;
	return 1000.0;
}

static void
gbi_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	if (getenv ("DEBUG_BI"))
		printf ("gbi_bounds\n");
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_bounds (gbi->priv->object, x1, y1, x2, y2, &ev);
	CORBA_exception_free (&ev);

	if (getenv ("DEBUG_BI"))
		printf ("gbi_bounds %g %g %g %g\n", *x1, *y1, *x2, *y2);
}

static void
gbi_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	Gbi *gbi = GBI (item);
	GNOME_Canvas_Buf *cbuf;
	CORBA_Environment ev;

	if (getenv ("DEBUG_BI"))
		printf ("gbi_render\n");

	cbuf = GNOME_Canvas_Buf__alloc ();
	if (!cbuf)
		return;

	cbuf->rgb_buf._buffer = buf->buf;
	cbuf->rgb_buf._maximum = buf->buf_rowstride * (buf->rect.y1 - buf->rect.y0);
	cbuf->rgb_buf._length = buf->buf_rowstride * (buf->rect.y1 - buf->rect.y0);
	cbuf->rgb_buf._buffer = buf->buf;
	cbuf->row_stride = buf->buf_rowstride;
	CORBA_sequence_set_release (&cbuf->rgb_buf, FALSE);
	
	cbuf->rect.x0 = buf->rect.x0;
	cbuf->rect.x1 = buf->rect.x1;
	cbuf->rect.y0 = buf->rect.y0;
	cbuf->rect.y1 = buf->rect.y1;
	cbuf->bg_color = buf->bg_color;
	cbuf->flags =
		(buf->is_bg  ? GNOME_Canvas_IS_BG : 0) |
		(buf->is_buf ? GNOME_Canvas_IS_BUF : 0);
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_render (gbi->priv->object, cbuf, &ev);
	memcpy (buf->buf, cbuf->rgb_buf._buffer, cbuf->rgb_buf._length);
	CORBA_exception_free (&ev);
	CORBA_free (cbuf);
}

static GNOME_Gdk_Event *
gdk_event_to_bonobo_event (GdkEvent *event)
{
	GNOME_Gdk_Event *e = GNOME_Gdk_Event__alloc ();

	if (e == NULL)
		return NULL;
			
	switch (event->type){

	case GDK_FOCUS_CHANGE:
		e->_d = GNOME_Gdk_FOCUS;
		e->_u.focus.inside = event->focus_change.in;
		return e;
			
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		e->_d = GNOME_Gdk_KEY;

		if (event->type == GDK_KEY_PRESS)
			e->_u.key.type = GNOME_Gdk_KEY_PRESS;
		else
			e->_u.key.type = GNOME_Gdk_KEY_RELEASE;
		e->_u.key.time =   event->key.time;
		e->_u.key.state =  event->key.state;
		e->_u.key.keyval = event->key.keyval;
		e->_u.key.length = event->key.length;
		e->_u.key.str = CORBA_string_dup (event->key.string);
		return e;

	case GDK_MOTION_NOTIFY:
		e->_d = GNOME_Gdk_MOTION;
		e->_u.motion.time = event->motion.time;
		e->_u.motion.x = event->motion.x;
		e->_u.motion.y = event->motion.x;
		e->_u.motion.x_root = event->motion.x_root;
		e->_u.motion.y_root = event->motion.y_root;
		e->_u.motion.xtilt = event->motion.xtilt;
		e->_u.motion.ytilt = event->motion.ytilt;
		e->_u.motion.state = event->motion.state;
		e->_u.motion.is_hint = event->motion.is_hint != 0;
		return e;
		
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		e->_d = GNOME_Gdk_BUTTON;
		if (event->type == GDK_BUTTON_PRESS)
			e->_u.button.type = GNOME_Gdk_BUTTON_PRESS;
		else if (event->type == GDK_BUTTON_RELEASE)
			e->_u.button.type = GNOME_Gdk_BUTTON_RELEASE;
		else if (event->type == GDK_2BUTTON_PRESS)
			e->_u.button.type = GNOME_Gdk_BUTTON_2_PRESS;
		else if (event->type == GDK_3BUTTON_PRESS)
			e->_u.button.type = GNOME_Gdk_BUTTON_3_PRESS;
		e->_u.button.time = event->button.time;
		e->_u.button.x = event->button.x;
		e->_u.button.y = event->button.y;
		e->_u.button.x_root = event->button.x_root;
		e->_u.button.y_root = event->button.y_root;
		e->_u.button.button = event->button.button;
		return e;

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		e->_d = GNOME_Gdk_CROSSING;
		if (event->type == GDK_ENTER_NOTIFY)
			e->_u.crossing.type = GNOME_Gdk_ENTER;
		else
			e->_u.crossing.type = GNOME_Gdk_LEAVE;
		e->_u.crossing.time = event->crossing.time;
		e->_u.crossing.x = event->crossing.x;
		e->_u.crossing.y = event->crossing.y;
		e->_u.crossing.x_root = event->crossing.x_root;
		e->_u.crossing.y_root = event->crossing.y_root;

		switch (event->crossing.mode){
		case GDK_CROSSING_NORMAL:
			e->_u.crossing.mode = GNOME_Gdk_NORMAL;
			break;

		case GDK_CROSSING_GRAB:
			e->_u.crossing.mode = GNOME_Gdk_GRAB;
			break;
			
		case GDK_CROSSING_UNGRAB:
			e->_u.crossing.mode = GNOME_Gdk_UNGRAB;
			break;
		}
		return e;

	default:
		g_warning ("Unsupported event received");
	}
	return NULL;
}

static gint
gbi_event (GnomeCanvasItem *item, GdkEvent *event)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	GNOME_Gdk_Event *corba_event;
	CORBA_boolean ret;
	
	if (getenv ("DEBUG_BI"))
		printf ("gbi_event\n");
	
	corba_event = gdk_event_to_bonobo_event (event);
	if (corba_event == NULL)
		return FALSE;
	
	CORBA_exception_init (&ev);
	ret = GNOME_Canvas_Item_event (gbi->priv->object, corba_event, &ev);
	CORBA_exception_free (&ev);
	CORBA_free (corba_event);

	return (gint) ret;
}

static void
gbi_init (GnomeBonoboItem *gbi)
{
	gbi->priv = g_new0 (GnomeBonoboItemPrivate, 1);
}

static void
gbi_set_arg (GtkObject *o, GtkArg *arg, guint arg_id)
{
	Gbi *gbi = GBI (o);
	GNOME_Canvas_Item corba_object;

	g_warning ("Get the reference count for canvas items properly done");
	
	switch (arg_id){
	case ARG_CORBA_CANVAS_ITEM:{
		CORBA_Environment ev;
		CORBA_exception_init (&ev);

		if (gbi->priv->object)
			CORBA_Object_release (gbi->priv->object, &ev);
		
		gbi->priv->object = CORBA_OBJECT_NIL;
		corba_object = GTK_VALUE_POINTER (*arg);
		if (corba_object != CORBA_OBJECT_NIL)
			gbi->priv->object = CORBA_Object_duplicate (corba_object, &ev);
		CORBA_exception_free (&ev);
		break;
	}
	}
}

static void
gbi_finalize (GtkObject *object)
{
	Gbi *gbi = GBI (object);

	if (gbi->priv->object != CORBA_OBJECT_NIL){
		CORBA_Environment ev;
		
		CORBA_exception_init (&ev);
		CORBA_Object_release (gbi->priv->object, &ev);
		CORBA_exception_free (&ev);
	}

	g_free (gbi->priv);
	
	(*GTK_OBJECT_CLASS (gbi_parent_class)->finalize)(object);
}

static void
gbi_class_init (GtkObjectClass *object_class)
{
	GnomeCanvasItemClass *item_class = (GnomeCanvasItemClass *) object_class;

	gbi_parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type (
		"GnomeBonoboItem::corba_canvas_item",
		GTK_TYPE_POINTER,
		GTK_ARG_WRITABLE, ARG_CORBA_CANVAS_ITEM);
	
	object_class->set_arg  = gbi_set_arg;
	object_class->finalize = gbi_finalize;
	item_class->update     = gbi_update;
	item_class->realize    = gbi_realize;
	item_class->unrealize  = gbi_unrealize;
	item_class->draw       = gbi_draw;
	item_class->point      = gbi_point;
	item_class->bounds     = gbi_bounds;
	item_class->render     = gbi_render;
	item_class->event      = gbi_event;
}

/**
 * gnome_bonobo_item_get_type:
 *
 * Returns the GtkType associated with a #GnomeBonoboItem canvas item
 */
GtkType
gnome_bonobo_item_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"GnomeBonoboItem",
			sizeof (GnomeBonoboItem),
			sizeof (GnomeBonoboItemClass),
			(GtkClassInitFunc) gbi_class_init,
			(GtkObjectInitFunc) gbi_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_canvas_item_get_type (), &info);
	}

	return type;
}

GnomeCanvasItem *
gnome_bonobo_item_new (GnomeCanvasGroup *parent, GnomeViewFrame *view_frame)
{
	CORBA_Environment ev;
	GnomeCanvasItem *i;
	GNOME_Canvas_Item remote_item;
	gboolean is_aa;
	GNOME_View remote_view;
	
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (parent), NULL);
	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);

	remote_view = view_frame->view;
	
	CORBA_exception_init (&ev);
	is_aa = GNOME_CANVAS_ITEM (parent)->canvas->aa;
	
	remote_item = GNOME_View_get_canvas_item (remote_view, is_aa, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		CORBA_exception_free (&ev);
		return NULL;
	}

	if (remote_item == CORBA_OBJECT_NIL){
		CORBA_exception_free (&ev);
		return NULL;
	}
	
	i = gnome_canvas_item_new (
		parent,
		gnome_bonobo_item_get_type (),
		"corba_canvas_item", remote_item,
		NULL);
	GNOME_BONOBO_ITEM (i)->view_frame = view_frame;
	return i;
}
