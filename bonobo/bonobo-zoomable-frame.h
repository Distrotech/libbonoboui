/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_ZOOMABLE_FRAME_H_
#define _BONOBO_ZOOMABLE_FRAME_H_

#include <bonobo/bonobo-object.h>

BEGIN_GNOME_DECLS

#define BONOBO_ZOOMABLE_FRAME_TYPE		(bonobo_zoomable_frame_get_type ())
#define BONOBO_ZOOMABLE_FRAME(o)		(GTK_CHECK_CAST ((o), BONOBO_ZOOMABLE_FRAME_TYPE, BonoboZoomableFrame))
#define BONOBO_ZOOMABLE_FRAME_CLASS(k)		(GTK_CHECK_CLASS_CAST((k), BONOBO_ZOOMABLE_FRAME_TYPE, BonoboZoomableFrameClass))
#define BONOBO_IS_ZOOMABLE_FRAME(o)		(GTK_CHECK_TYPE ((o), BONOBO_ZOOMABLE_FRAME_TYPE))
#define BONOBO_IS_ZOOMABLE_FRAME_CLASS(k)	(GTK_CHECK_CLASS_TYPE ((k), BONOBO_ZOOMABLE_FRAME_TYPE))

typedef struct _BonoboZoomableFrame		BonoboZoomableFrame;
typedef struct _BonoboZoomableFramePrivate	BonoboZoomableFramePrivate;
typedef struct _BonoboZoomableFrameClass	BonoboZoomableFrameClass;

struct _BonoboZoomableFrame {
        BonoboObject			object;

	BonoboZoomableFramePrivate	*priv;
};

struct _BonoboZoomableFrameClass {
	BonoboObjectClass		parent;

	void (*zoom_level_changed)	(BonoboZoomableFrame *zframe,
					 float zoom_level);
	void (*zoom_parameters_changed)	(BonoboZoomableFrame *zframe);
};

POA_Bonobo_ZoomableFrame__epv *bonobo_zoomable_frame_get_epv  (void);

GtkType			 bonobo_zoomable_frame_get_type			(void);
Bonobo_ZoomableFrame	 bonobo_zoomable_frame_corba_object_create	(BonoboObject		*object);

BonoboZoomableFrame	*bonobo_zoomable_frame_new			(void);

BonoboZoomableFrame	*bonobo_zoomable_frame_construct		(BonoboZoomableFrame	*zframe,
									 Bonobo_ZoomableFrame	 corba_zframe);

float		 bonobo_zoomable_frame_get_zoom_level			(BonoboZoomableFrame	*zframe);

float		 bonobo_zoomable_frame_get_min_zoom_level		(BonoboZoomableFrame	*zframe);
float		 bonobo_zoomable_frame_get_max_zoom_level		(BonoboZoomableFrame	*zframe);
gboolean	 bonobo_zoomable_frame_has_min_zoom_level		(BonoboZoomableFrame	*zframe);
gboolean	 bonobo_zoomable_frame_has_max_zoom_level		(BonoboZoomableFrame	*zframe);
gboolean	 bonobo_zoomable_frame_is_continuous			(BonoboZoomableFrame	*zframe);

float		*bonobo_zoomable_frame_get_preferred_zoom_levels	(BonoboZoomableFrame	*zframe,
									 int			*pnum_ret);

void		 bonobo_zoomable_frame_set_zoom_level			(BonoboZoomableFrame	*zframe,
									 float			 zoom_level);

void		 bonobo_zoomable_frame_zoom_in				(BonoboZoomableFrame	*zframe);
void		 bonobo_zoomable_frame_zoom_out				(BonoboZoomableFrame	*zframe);
void		 bonobo_zoomable_frame_zoom_to_fit			(BonoboZoomableFrame	*zframe);
void		 bonobo_zoomable_frame_zoom_to_default			(BonoboZoomableFrame	*zframe);

/* Connecting to the remote object */
void		 bonobo_zoomable_frame_bind_to_zoomable			(BonoboZoomableFrame	*zframe,
									 Bonobo_Zoomable	 zoomable);

Bonobo_Zoomable	 bonobo_zoomable_frame_get_zoomable			(BonoboZoomableFrame	*zframe);


END_GNOME_DECLS

#endif /* _BONOBO_ZOOMABLE_FRAME_H_ */

