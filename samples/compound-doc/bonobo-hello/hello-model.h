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

#ifndef BONOBO_HELLO_MODEL_H
#define BONOBO_HELLO_MODEL_H

#include <bonobo.h>

/* The struct holding the Model (the data structure) */
typedef struct {
    BonoboEmbeddable *bonobo_object;
    
    gchar *text;
} Hello;

void hello_model_init (Hello *hello, BonoboEmbeddable *bonobo_object);
void hello_model_set_text (Hello *hello, gchar *text);
void hello_model_clear (Hello *hello);
void hello_model_destroy (Hello *obj);

#endif
