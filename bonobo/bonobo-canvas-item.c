/*
 * gnome-bonobo-item.c: Canvas item implementation for embedding remote canvas-items
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * (C) 1999 International GNOME Support
 */
#include <config.h>
#include <bonobo/gnome-bonobo-item.h>
#include <bonobo/gnome-object.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/*
 * Just to simplify typing
 */
#define GBI(x)          GNOME_BONOBO_ITEM(x)
typedef GnomeBonoboItem Gbi;

/*
 * Creates a GNOME_Canvas_SVPSegment structure representing the ArtSVPSeg
 * structure, suitable for sending over the network
 */
static GNOME_Canvas_SVPSegment *
art_svp_segment_to_CORBA_SVP_Segment (ArtSVPSeg *seg, GNOME_Canvas_SVPSegment *segment)
{
	segment->points = GNOME_Canvas_Points__alloc ();
	if (segment->points == NULL){
		CORBA_free (segment);
		return NULL;
	}
	segment->points->_buffer = CORBA_sequence_GNOME_Canvas_Point_allocbuf (seg->n_points);
	if (segment->points->_buffer == NULL){
		CORBA_free (segment->points);
		CORBA_free (segment);
		return NULL;
	}
	segment->points->_maximum = seg->n_points;
	segment->points->_length = seg->n_points;
	
	if (seg->dir == 0)
		segment->up = CORBA_TRUE;
	else
		segment->up = CORBA_FALSE;

	segment->bbox.x0 = seg->bbox.x0;
	segment->bbox.x1 = seg->bbox.x1;
	segment->bbox.y0 = seg->bbox.y0;
	segment->bbox.y1 = seg->bbox.y1;

	for (i = 0; i < seg->n_points; i++){
		segment->points->_buffer [i].x = seg->points [i].x;
		segment->points->_buffer [i].y = seg->points [i].y;
	}

	return segment;
}

/*
 * Creates a GNOME_Canvas_SVP CORBA structure from the art_svp, suitable
 * for sending over the wire
 */
static GNOME_Canvas_SVP *
art_svp_to_CORBA_SVP (ArtSVP *art_svp)
{
	GNOME_Canvas_SVP *svp;
	
	svp = GNOME_Canvas_SVP__alloc ();
	if (!svp)
		return NULL;
	
	if (art_svp){
		svp->_buffer = CORBA_sequence_GNOME_Canvas_SVPSegment_allocbuf (svp->n_segs);
		if (svp->_buffer == NULL){
			svp->_length = 0;
			svp->_maximum = 0;
			return svp;
		}
		svp->_maximum = art_svp->n_segs;
		svp->_length = art_svp->n_segs;

		for (i = 0; i < art_svp->n_segs; i++){
			r = art_svp_segment_to_CORBA_SVP_Segment (svp->segs [i], svp->_buffer [i]);
			if (r == NULL){
				int j;
				
				for (i = 0; i < j; i++)
					CORBA_free (svp->_buffer [i]);
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
	
	for (i = 0; i < 5; i++)
		affine [i] = item_affine;

	clip_path = art_svp_to_CORBA (item_clip_path);
	if (!clip_path)
		return;

	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_update (gbi->priv->object, affine, clip_path, flags, &ev);
	CORBA_exception_free (&ev);
				   
	CORBA_free (clip_path);
}

static void
gbi_realize (GnomeCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_realize (gbi->priv->object, &ev);
	CORBA_exception_free (&ev);
}

static void
gbi_unrealize (GnomeCanvasItem *item)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_unrealize (gbi->priv->object, &ev);
	CORBA_exception_free (&ev);
}

static void
gbi_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_draw (gbi->priv->object, GDK_WINDOW_XWINDOW (drawable), x, y, width, height, &ev);
	CORBA_exception_free (&ev);
}

static double
gbi_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	
	CORBA_exception_init (&ev);
	if (GNOME_Canvas_Item_contains (gbi->priv->object, GDK_WINDOW_XWINDOW (drawable), x, y, &ev)){
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
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_bounds (gbi->priv->object, GDK_WINDOW_XWINDOW (drawable), x, y, width, height, &ev);
	CORBA_exception_free (&ev);
}

static void
gbi_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	Gbi *gbi = GBI (item);
	GNOME_Canvas_Buf *cbuf;
	CORBA_Environment ev;

	cbuf = GNOME_Canvas_Buf__alloc ();
	if (!cbuf)
		return;

	cbuf->rgb_buf = GNOME_Canvas_pixbuf__alloc ();
	if (cbuf->rgb_buf == NULL){
		CORBA_free (cbuf);
		return;
	}
		
	cbuf->rgb_buf->_buffer = buf->buf;
	cbuf->rgb_buf->_maximum = buf->row_stride * (buf->rect.y1 - buf->rect.y0);
	cbuf->rgb_buf->_lengt = buf->row_stride * (buf->rect.y1 - buf->rect.y0);
	cbuf->rgb_buf->_buffer = buf->buf;
	cbuf->row_stride = buf->row_stride;
	CORBA_sequence_set_release (cbuf->rgb_buf, FALSE);
	
	cbuf->rect.x0 = buf->rect.x0;
	cbuf->rect.x1 = buf->rect.x1;
	cbuf->rect.y0 = buf->rect.y0
	cbuf->rect.y1 = buf->rect.y1;
	cbuf->bg_color = buf->bg_color;
	cbuf->flags =
		(buf->is_bg  ? GNOME_CANVAS_IS_BG : 0) |
		(buf->is_buf ? GNOME_CANVAS_IS_BUF : 0);
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_render (gbi->priv->object, cbuf, &ev);
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
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		e->_d = GNOME_Gdk_KEY;

		if (event->type == GDK_KEY_PRESS)
			e->_u->key.type = GNOME_Gdk_KEY_PRESS;
		else
			e->_u->key.type = GNOME_Gdk_KEY_RELEASE;
		e->_u->key.time =   event->key.time;
		e->_u->key.state =  event->key.state;
		e->_u->key.keyval = event->key.keyval;
		e->_u->key.length = event->key.length;
		e->_u->key.str = CORBA_string_dup (event->str);
		return e;

	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		e->_d = GNOME_Gdk_BUTTON;
		if (event->type == GDK_BUTTON_PRESS)
			e->_u->button.type = GNOME_Gdk_BUTTON_PRESS;
		else if (event->type == GDK_BUTTON_RELEASE)
			e->_u->button.type = GNOME_Gdk_BUTTON_RELEASE;
		else if (event->type == GDK_2BUTTON_PRESS)
			e->_u->button.type = GNOME_Gdk_BUTTON_2_PRESS;
		else if (event->type == GDK_3BUTTON_PRESS)
			e->_u->button.type = GNOME_Gdk_BUTTON_3_PRESS;
		e->_u->button.time = event->button.time;
		e->_u->button.x = event->button.x;
		e->_u->button.y = event->button.y;
		e->_u->button.root_x = event->button.root_x;
		e->_u->button.root_y = event->button.root_y;
		e->_u->button.button = event->button.button;
		return e;

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		e->_d = GNOME_Gdk_CROSSING;
		if (event->type == GDK_ENTER_NOTIFY)
			e->_u.crossing.type = GNOME_Gdk_ENTER;
		else
			e->_u.crossing.type = GNOME_Gdk_LEAVE;
		e->_u->crossing->time = event->crossing.time;
		e->_u->crossing->x = event->crossing.x;
		e->_u->crossing->y = event->crossing.y;
		e->_u->crossing->x_root = event->crossing.root_x;
		e->_u->crossing->y_root = event->crossing.root_y;

		if (event->crossing.mode == 
		e->_u->crossing->mode = event->x;

	}
}

static gint
gbi_event (GnomeCanvasItem *item, GdkEvent *event)
{
	Gbi *gbi = GBI (item);
	CORBA_Environment ev;
	GNOME_Gdk_Event *corba_event;

	corba_event = gdk_event_to_bonobo_event (event);
	if (corba_event == NULL)
		return FALSE;
	
	CORBA_exception_init (&ev);
	GNOME_Canvas_Item_event (gbi->priv->object, GDK_WINDOW_XWINDOW (drawable), x, y, width, height, &ev);
	CORBA_exception_free (&ev);
}

static void
gnome_bonobo_item_class_init (GtkObjectClass *object_class)
{
	GnomeCanvasItemClass *item_class = (GnomeCanvasItemClass *) object_class;

	item_class->update    = gbi_update;
	item_class->realize   = gbi_realize;
	item_class->unrealize = gbi_unrealize;
	item_class->draw      = gbi_draw;
	item_class->point     = gbi_point;
	item_class->bounds    = gbi_bounds;
	item_class->render    = gbi_render;
	item_class->event     = gbi_event;
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
			(GtkClassInitFunc) gnome_bonobo_item_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_canvas_item_get_type (), &info);
	}

	return type;
}
