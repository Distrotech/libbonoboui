/* $Id */
/*
  Sample-Container Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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
*/

#include <bonobo.h>

#include "container-io.h"
#include "component-io.h"

#define STORAGE_TYPE "vfs"

static Bonobo_Stream
open_stream (Bonobo_Storage storage, char *path,
	     Bonobo_Storage_OpenMode mode)
{
	Bonobo_Storage corba_storage = storage;
	Bonobo_Stream corba_stream;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	corba_stream = Bonobo_Storage_open_stream (corba_storage,
						   path, mode, &ev);

	CORBA_exception_free (&ev);

	return corba_stream;
}

static Bonobo_Stream
create_stream (Bonobo_Storage storage, char *path)
{
	Bonobo_Storage corba_storage = storage;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_Storage_create_stream (corba_storage, path, &ev);

	CORBA_exception_free (&ev);

	return open_stream (storage, path, Bonobo_Storage_WRITE);
}

#define GOAD_FILE "goad.id"
#define DATA_FILE "data"

static void
save_component (BonoboStorage * storage, Component * component, int index)
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
		component_save_id (component,
				   create_stream (corba_subdir, GOAD_FILE));
		component_save (component,
				create_stream (corba_subdir, DATA_FILE));
	}

	CORBA_exception_free (&ev);

	g_free (curr_dir);
/*    bonobo_object_unref (BONOBO_OBJECT (subdir));*/
}

static char *
load_component_id_stream_read (Bonobo_Stream stream)
{
	Bonobo_Stream_iobuf *buffer;
	CORBA_Environment    ev;
	CORBA_long           bytes_read;
	GString             *str;
	char                *ans;

	str = g_string_sized_new (32);
	CORBA_exception_init (&ev);

	/* We will read the data in chunks of the specified size */
#define READ_CHUNK_SIZE 65536
	do {
		int i;

		bytes_read =
		    Bonobo_Stream_read (stream, READ_CHUNK_SIZE, &buffer,
					&ev);
		for (i = 0; i < bytes_read && i < buffer->_length; i++)
			g_string_append_c (str, buffer->_buffer [i]);

		CORBA_free (buffer);

	} while (bytes_read > 0);
#undef READ_CHUNK_SIZE

	CORBA_exception_free (&ev);

	ans = str->str;
	g_string_free (str, FALSE);

	return ans;
}

static char *
load_component_id (Bonobo_Storage storage)
{
	Bonobo_Storage corba_storage = storage;
	Bonobo_Stream corba_stream;
	char *goad_id;

	corba_stream = open_stream (corba_storage, GOAD_FILE,
				    Bonobo_Storage_READ);

	if (corba_stream)
		goad_id = load_component_id_stream_read (corba_stream);
	else {
		g_warning ("Can't find '%s'", GOAD_FILE);
		goad_id = NULL;
	}

	return goad_id;
}

static void
load_component (SampleApp * inst, BonoboStorage * storage, int index)
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
	goad_id = load_component_id (corba_subdir);
	if (goad_id) {
		Bonobo_Stream corba_stream;

		component = sample_app_add_component (inst, goad_id);

		if (component) {
			corba_stream = open_stream (corba_subdir, DATA_FILE,
						    Bonobo_Storage_READ);

			component_load (component, corba_stream);
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

	CORBA_exception_free (&ev);

	Bonobo_Storage_commit (corba_storage, &ev);

}
