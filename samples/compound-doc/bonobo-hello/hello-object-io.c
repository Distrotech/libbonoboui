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

/* Load data from a BonoboStream -- implementation */
static void
hello_object_stream_read (Bonobo_Stream stream, Hello * obj,
			  CORBA_Environment *ev)
{
	Bonobo_Stream_iobuf *buffer;
	CORBA_long bytes_read;
	gchar *charbuf = g_new0 (gchar, 0);
	size_t last_len = 0;

	/* We will read the data in chunks of the specified size */
#define READ_CHUNK_SIZE 65536
	do {
		bytes_read =
		    Bonobo_Stream_read (stream, READ_CHUNK_SIZE, &buffer, ev);
		if (ev->_major != CORBA_NO_EXCEPTION)
			return;

		charbuf = g_realloc (charbuf, last_len + buffer->_length);
		memcpy (charbuf + last_len, buffer->_buffer,
			buffer->_length);
		last_len += buffer->_length;

		CORBA_free (buffer);
	} while (bytes_read > 0);
#undef READ_CHUNK_SIZE

	charbuf[last_len] = '\0';
	hello_model_set_text (obj, charbuf);
	g_free (charbuf);
}

/* This function implements the Bonobo::PersistStream:load method. */
void
hello_object_pstream_load (BonoboPersistStream * ps,
			   const Bonobo_Stream stream,
			   Bonobo_Persist_ContentType type,
			   void *data, CORBA_Environment *ev)
{
	Hello *obj = (Hello *) data;

	/* 0. Check the Content Type? FIXME */

	/* 1. Free the old data */
	hello_model_clear (obj);

	/* 2. Read the new text data. */
	hello_object_stream_read (stream, obj, ev);
}

/* This function implements the Bonobo::PersistStream:save method. */
void
hello_object_pstream_save (BonoboPersistStream * ps,
			   const Bonobo_Stream stream,
			   Bonobo_Persist_ContentType type,
			   void *data, CORBA_Environment *ev)
{
	Hello *obj = data;
	Bonobo_Stream_iobuf *buffer;
	size_t pos = 0, length = strlen (obj->text);

	/* FIXME: Should check content type */

	/* Write the text data into the stream. */
	buffer = Bonobo_Stream_iobuf__alloc ();

	buffer->_buffer = obj->text;
	buffer->_length = length;

	while (pos < length) {
		CORBA_long bytes_written;

		bytes_written = Bonobo_Stream_write (stream, buffer, ev);

		if (ev->_major != CORBA_NO_EXCEPTION) {
			CORBA_free (buffer);
			return;
		}

		pos += bytes_written;
	}

	CORBA_free (buffer);
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
