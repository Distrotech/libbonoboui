/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_WIN_H_
#define _BONOBO_WIN_H_

#include <gnome.h>
#include <bonobo/bonobo-object.h>

#define BONOBO_WIN_TYPE        (bonobo_win_get_type ())
#define BONOBO_WIN(o)          (GTK_CHECK_CAST ((o), BONOBO_WIN_TYPE, BonoboWin))
#define BONOBO_WIN_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_WIN_TYPE, BonoboWinClass))
#define BONOBO_IS_WIN(o)       (GTK_CHECK_TYPE ((o), BONOBO_WIN_TYPE))
#define BONOBO_IS_WIN_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_WIN_TYPE))

typedef struct _BonoboWin        BonoboWin;
typedef struct _BonoboWinPrivate BonoboWinPrivate;
typedef struct _BonoboWinClass   BonoboWinClass;

struct _BonoboWin {
	GtkWindow          parent;
	
	BonoboWinPrivate  *priv;
};

struct _BonoboWinClass {
	GtkWindowClass    parent_class;
};

GtkType              bonobo_win_get_type            (void);

GtkWidget           *bonobo_win_new                 (const char *win_name,
						     const char *title);

void                 bonobo_win_set_contents        (BonoboWin  *win,
						     GtkWidget  *contents);
GtkWidget           *bonobo_win_get_contents        (BonoboWin  *win);

gboolean             bonobo_win_xml_merge           (BonoboWin  *win,
						     const char *path,
						     const char *xml,
						     const char *component);

void                 bonobo_win_xml_merge_tree      (BonoboWin  *win,
						     const char *path,
						     xmlNode    *tree,
						     const char *component);

char                *bonobo_win_xml_get             (BonoboWin  *win,
						     const char *path,
						     gboolean    node_only);

gboolean             bonobo_win_xml_node_exists     (BonoboWin  *win,
						     const char *path);

void                 bonobo_win_xml_rm              (BonoboWin  *win,
						     const char *path,
						     const char *by_component);

void                 bonobo_win_object_set          (BonoboWin  *win,
						     const char *path,
						     Bonobo_Unknown object,
						     CORBA_Environment *ev);

Bonobo_Unknown       bonobo_win_object_get          (BonoboWin  *win,
						     const char *path,
						     CORBA_Environment *ev);

GtkAccelGroup       *bonobo_win_get_accel_group     (BonoboWin  *win);

void                 bonobo_win_dump                (BonoboWin  *win,
						     const char *msg);

void                 bonobo_win_register_component   (BonoboWin     *win,
						      const char    *name,
						      Bonobo_Unknown component);

void                 bonobo_win_deregister_component (BonoboWin     *win,
						      const char    *name);

Bonobo_Unknown       bonobo_win_component_get        (BonoboWin     *win,
						      const char    *name);

void                 bonobo_win_add_popup            (BonoboWin     *win,
						      GtkMenu       *popup,
						      const char    *path);

/*
 * NB. popups are automaticaly removed on destroy.
 */
void                 bonobo_win_remove_popup         (BonoboWin     *win,
						      const char    *path);

#endif /* _BONOBO_WIN_H_ */
