/* $Id */
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

static int hello_object_stream_read (Bonobo_Stream stream,
				     Hello *obj);

/* Load data from a BonoboStream -- implementation */
static int
hello_object_stream_read (Bonobo_Stream stream, Hello *obj)
{
    Bonobo_Stream_iobuf *buffer;
    CORBA_long bytes_read;
    CORBA_Environment ev;
    gchar *charbuf = g_new0 (gchar, 0);
    size_t last_len = 0;
    
    CORBA_exception_init (&ev);
    
    /* We will read the data in chunks of the specified size */
#define READ_CHUNK_SIZE 65536
    do {
	bytes_read = Bonobo_Stream_read (stream, READ_CHUNK_SIZE, &buffer, &ev);
	
	charbuf = g_realloc (charbuf,
			     last_len + buffer->_length);
	memcpy (charbuf + last_len, buffer->_buffer, buffer->_length);
	last_len += buffer->_length;
	
	CORBA_free (buffer);
    } while (bytes_read > 0);
#undef READ_CHUNK_SIZE
    
    CORBA_exception_free (&ev);
    
    if (bytes_read < 0)
    {
	g_free (charbuf);
	return -1;
    }

    charbuf[last_len] = '\0';
    hello_model_set_text (obj, charbuf);
    g_free (charbuf);

    return 0;
}

/* This function implements the Bonobo::PersistStream:load method. */
int
hello_object_pstream_load (BonoboPersistStream *ps,
			   Bonobo_Stream stream,
			   void *data)
{
    Hello *obj = (Hello*) data;
    
    /* 1. Free the old data */
    hello_model_clear (obj);
    
    /* 2. Read the new text data. */
    if (hello_object_stream_read (stream, obj) < 0)
	return -1; /* This will raise an exception. */
    
    return 0;
}

/* This function implements the Bonobo::PersistStream:save method. */
int
hello_object_pstream_save (BonoboPersistStream *ps,
			   Bonobo_Stream stream,
			   void *data)
{
    Hello *obj = data;
    Bonobo_Stream_iobuf *buffer;
    size_t pos = 0, length = strlen (obj->text);
    CORBA_Environment ev;
    
    CORBA_exception_init (&ev);
    
    /* Write the text data into the stream. */
    buffer = Bonobo_Stream_iobuf__alloc ();
    
    buffer->_buffer = obj->text;
    buffer->_length = length;
    
    while (pos < length)
    {
	CORBA_long bytes_written;
	
	bytes_written = Bonobo_Stream_write (stream, buffer, &ev);
	
	if (ev._major != CORBA_NO_EXCEPTION)
	{
	    CORBA_free (buffer);
	    CORBA_exception_free (&ev);
	    return -1; /* Raise exception */
	}
	
	pos += bytes_written;
    }
    
    CORBA_free (buffer);
    CORBA_exception_free (&ev);
    
    return 0;
}

