/* $Id */
/*
  Sample-Container Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  Copyright (C) 2000 Helix Code, Inc.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  Authors:
     ÉRDI Gergõ <cactus@cactus.rulez.org>
     Michael Meeks <michael@helixcode.com>
*/

#include <bonobo.h>

#include "container-io.h"
#include "component-io.h"

#define STORAGE_TYPE "vfs"

static Bonobo_Stream
create_stream (Bonobo_Storage     storage,
	       char              *path,
	       CORBA_Environment *ev)
{
	Bonobo_Storage corba_storage = storage;

	Bonobo_Storage_create_stream (corba_storage, path, ev);
	if (ev->_major != CORBA_NO_EXCEPTION)
		return CORBA_OBJECT_NIL;

	return Bonobo_Storage_open_stream (storage, path,
					   Bonobo_Storage_WRITE, ev);
}

#define GOAD_FILE "goad.id"
#define DATA_FILE "data"

static void
save_component (BonoboStorage *storage, Component *component, int index)
{
	char *curr_dir = g_strdup_printf ("%08d", index);

	Bonobo_Storage corba_storage =
		bonobo_object_corba_objref (BONOBO_OBJECT (storage));
	Bonobo_Storage corba_subdir;

	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	corba_subdir = Bonobo_Storage_create_storage (corba_storage,
						      curr_dir, &ev);
	if (!corba_subdir)
		g_warning ("Can't create '%s'", curr_dir);
	else {
		Bonobo_Stream corba_stream;

		corba_stream = create_stream (corba_subdir, GOAD_FILE, &ev);
		component_save_id (component, corba_stream, &ev);
		Bonobo_Stream_close (corba_stream, &ev);
		Bonobo_Unknown_unref (corba_stream, &ev);
		CORBA_Object_release (corba_stream, &ev);

		corba_stream = create_stream (corba_subdir, DATA_FILE, &ev);
		component_save (component, corba_stream, &ev);
		Bonobo_Stream_close (corba_stream, &ev);
		Bonobo_Unknown_unref (corba_stream, &ev);
		CORBA_Object_release (corba_stream, &ev);

		Bonobo_Unknown_unref (subdir, &ev);
		CORBA_Object_release (subdir, &ev);
	}

	g_free (curr_dir);

	CORBA_exception_free (&ev);
}

static char *
load_component_id_stream_read (Bonobo_Stream      stream,
			       CORBA_Environment *ev)
{
	Bonobo_Stream_iobuf *buffer;
	GString             *str;
	char                *ans;

	str = g_string_sized_new (32);

	/* We will read the data in chunks of the specified size */
#define READ_CHUNK_SIZE 65536
	do {
		int i;

		Bonobo_Stream_read (stream, READ_CHUNK_SIZE, &buffer, ev);
		if (ev->_major != CORBA_NO_EXCEPTION)
			return NULL;

		for (i = 0; i < buffer->_length; i++)
			g_string_append_c (str, buffer->_buffer [i]);

		if (buffer->_length <= 0)
			break;
		CORBA_free (buffer);
	} while (1);
#undef READ_CHUNK_SIZE
	CORBA_free (buffer);

	ans = str->str;
	g_string_free (str, FALSE);

	return ans;
}

static char *
load_component_id (Bonobo_Storage     storage,
		   CORBA_Environment *ev)
{
	Bonobo_Stream  corba_stream;
	char *goad_id;

	corba_stream = Bonobo_Storage_open_stream (storage, GOAD_FILE,
						   Bonobo_Storage_READ, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		return NULL;

	if (corba_stream) {
		goad_id = load_component_id_stream_read (corba_stream, ev);
		Bonobo_Unknown_unref (corba_stream, ev);
		CORBA_Object_release (corba_stream, ev);
	} else {
		g_warning ("Can't find '%s'", GOAD_FILE);
		goad_id = NULL;
	}

	return goad_id;
}

static void
load_component (SampleApp *inst, BonoboStorage *storage, int index)
{
	char *curr_dir = g_strdup_printf ("%08d", index);
	char *goad_id;
	Bonobo_Storage corba_subdir;
	Bonobo_Storage corba_storage =
	    bonobo_object_corba_objref (BONOBO_OBJECT (storage));
	Component *component;

	CORBA_Environment ev;
	CORBA_exception_init (&ev);

	corba_subdir = Bonobo_Storage_open_storage (corba_storage,
						    curr_dir,
						    Bonobo_Storage_READ,
						    &ev);
	goad_id = load_component_id (corba_subdir, &ev);
	if (goad_id) {
		Bonobo_Stream corba_stream;

		component = sample_app_add_component (inst, goad_id);

		if (component) {
			corba_stream = Bonobo_Storage_open_stream (corba_subdir, DATA_FILE,
								   Bonobo_Storage_READ, &ev);

			if (ev._major != CORBA_NO_EXCEPTION)
				return;

			component_load (component, corba_stream, &ev);
		} else
			g_warning ("Component '%s' activation failed", goad_id);

		g_free (goad_id);
	}

	CORBA_exception_free (&ev);

	g_free (curr_dir);
}


void
sample_container_load (SampleApp *inst, const char *filename)
{
	CORBA_Environment ev;
	BonoboStorage *storage;
	Bonobo_Storage corba_storage;
	Bonobo_Storage_directory_list *list;
	int i;

	storage = bonobo_storage_open (STORAGE_TYPE, filename,
				       BONOBO_SS_RDWR | BONOBO_SS_CREATE,
				       0664);
	g_return_if_fail (storage);

	CORBA_exception_init (&ev);

	corba_storage =
	    bonobo_object_corba_objref (BONOBO_OBJECT (storage));

	list = Bonobo_Storage_list_contents (corba_storage, "/", &ev);

	if (!list) {
		CORBA_exception_free (&ev);
		return;
	}

	for (i = 0; i < list->_length; i++)
		load_component (inst, storage, i);

	CORBA_free (list);
	CORBA_exception_free (&ev);
}

void
sample_container_save (SampleApp *inst, const char *filename)
{
	CORBA_Environment ev;
	BonoboStorage *storage;
	Bonobo_Storage corba_storage;
	GList *components;
	int i;

	unlink (filename);
	storage = bonobo_storage_open (STORAGE_TYPE, filename,
				       BONOBO_SS_RDWR | BONOBO_SS_CREATE,
				       0664);
	g_return_if_fail (storage);

	CORBA_exception_init (&ev);

	corba_storage =
	    bonobo_object_corba_objref (BONOBO_OBJECT (storage));

	i = 0;
	for (components = g_list_first (inst->components);
	     components; components = g_list_next (components))
		save_component (storage, components->data, i++);

	Bonobo_Storage_commit (corba_storage, &ev);

	CORBA_exception_free (&ev);

	bonobo_object_unref (BONOBO_OBJECT (storage));
}
