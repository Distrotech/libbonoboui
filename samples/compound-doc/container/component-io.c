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

#include "component.h"

void
component_load (Component *component, Bonobo_Stream stream)
{
    Bonobo_PersistStream persist;
    Bonobo_Stream corba_stream = stream;
    CORBA_Environment ev;

    /* Get the PersistStream interface of our component */
    persist = bonobo_object_client_query_interface (component->server,
						    "IDL:Bonobo/PersistStream:1.0",
						    NULL);
    if (persist == CORBA_OBJECT_NIL)
    {
	printf ("noimplement\n");
	/* This component doesn't implement load/save features */
	return;
    }

    CORBA_exception_init (&ev);

    Bonobo_PersistStream_load (persist, corba_stream, &ev);

    /* See if we had any problems */
    if (ev._major != CORBA_NO_EXCEPTION)
	gnome_warning_dialog (_("An exception occured while trying "
				"to load data into the component with "
				"PersistStorage"));
    Bonobo_Unknown_unref (persist, &ev);
    CORBA_Object_release (persist, &ev);
/*    bonobo_object_unref (BONOBO_OBJECT (stream));*/
    
    CORBA_exception_free (&ev);
}

void
component_save (Component *component, Bonobo_Stream stream)
{
    Bonobo_PersistStream persist;
    Bonobo_Stream corba_stream = stream;
    CORBA_Environment ev;

    /* Get the PersistStream interface of our component */
    persist = bonobo_object_client_query_interface (component->server,
						    "IDL:Bonobo/PersistStream:1.0",
						    NULL);
    if (persist == CORBA_OBJECT_NIL)
    {
	printf ("noimplement\n");
	/* This component doesn't implement load/save features */
	return;
    }

    CORBA_exception_init (&ev);

    Bonobo_PersistStream_save (persist, corba_stream, &ev);

    /* See if we had any problems */
    if (ev._major != CORBA_NO_EXCEPTION) {
	gnome_warning_dialog (_("An exception occured while trying "
				"to save data from the component with "
				"PersistStorage"));
    } else {
	Bonobo_Unknown_unref (persist, &ev);
	
	CORBA_Object_release (persist, &ev);
    }
    
    CORBA_exception_free (&ev);
}

void
component_save_id (Component *component, Bonobo_Stream stream)
{
    Bonobo_Stream_iobuf *buffer;
    Bonobo_Stream corba_stream = stream;
    size_t pos = 0, length = strlen (component->goad_id);
    CORBA_Environment ev;
    
    CORBA_exception_init (&ev);

    buffer = Bonobo_Stream_iobuf__alloc ();
    buffer->_length = length;
    buffer->_buffer = component->goad_id;

    while (pos < length)
    {
	CORBA_long bytes_written;
	
	bytes_written = Bonobo_Stream_write (corba_stream, buffer, &ev);
	
	if (ev._major != CORBA_NO_EXCEPTION)
	{
	    CORBA_free (buffer);
	    CORBA_exception_free (&ev);
	    return;
	}
	
	pos += bytes_written;
    }

    CORBA_free (buffer);
    CORBA_exception_free (&ev);
}
