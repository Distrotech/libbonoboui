/* $Id$ */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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

#include "hello-object-io.h"

/* This function implements the Bonobo::PersistStream:load method. */
void
hello_object_pstream_load (BonoboPersistStream       *ps,
			   const Bonobo_Stream        stream,
			   Bonobo_Persist_ContentType type,
			   void                      *data,
			   CORBA_Environment         *ev)
{
	Hello *obj = (Hello *) data;
	char  *str;

	/* 0. Check the Content Type? FIXME */

	/* 1. Free the old data */
	hello_model_clear (obj);

	/* 2. Read the new text data. */
	bonobo_stream_client_read_string (stream, &str, ev);
	if (ev->_major == CORBA_NO_EXCEPTION) {
		hello_model_set_text (obj, str);
		g_free (str);
	}
}

/* This function implements the Bonobo::PersistStream:save method. */
void
hello_object_pstream_save (BonoboPersistStream * ps,
			   const Bonobo_Stream stream,
			   Bonobo_Persist_ContentType type,
			   void *data, CORBA_Environment *ev)
{
	Hello *obj = data;

	bonobo_stream_client_write_string (stream,
					   obj->text?obj->text:"",
					   TRUE, ev);
}

CORBA_long
hello_object_pstream_get_max_size (BonoboPersistStream *ps, void *data,
				   CORBA_Environment *ev)
{
	Hello *obj = data;

	return strlen (obj->text);
}

Bonobo_Persist_ContentTypeList *
hello_object_pstream_get_types (BonoboPersistStream *ps, void *closure,
				CORBA_Environment *ev)
{
	/* FIXME */
	return bonobo_persist_generate_content_types (1, "");
}
