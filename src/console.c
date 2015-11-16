/*
  
  Copyright (C) 2014 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <draw.h>
#include <util.h>
#include <stdarg.h>

#include <simtk/simtk.h>

#include "console.h"

void
simtk_console_properties_destroy (struct simtk_console_properties *prop)
{
  if (prop->lock != NULL)
    SDL_DestroyMutex (prop->lock);

  if (prop->buffer != NULL)
    free (prop->buffer);

  free (prop);
}

struct simtk_console_properties *
simtk_console_properties_new (int rows, int cols)
{
  struct simtk_console_properties *new;

  if ((new = calloc (1, sizeof (struct simtk_console_properties))) == NULL)
    goto fail;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
    goto fail;

  if ((new->buffer = calloc (1, rows * cols)) == NULL)
    goto fail;

  new->rows = rows;
  new->cols = cols;

  new->fgcolor = SIMTK_CONSOLE_DEFAULT_FGCOLOR;
  new->bgcolor = SIMTK_CONSOLE_DEFAULT_BGCOLOR;
  
  return new;

fail:
  if (new != NULL)
    simtk_console_properties_destroy (new);
  
  return NULL;
}

void
simtk_console_properties_lock (struct simtk_console_properties *prop)
{
  SDL_mutexP (prop->lock);
}

void
simtk_console_properties_unlock (struct simtk_console_properties *prop)
{
  SDL_mutexV (prop->lock);
}

struct simtk_console_properties *
simtk_console_get_properties (const simtk_widget_t *widget)
{
  return (struct simtk_console_properties *) simtk_textview_get_opaque (widget);
}

void
simtk_console_set_properties (simtk_widget_t *widget, struct simtk_console_properties *prop)
{
  simtk_textview_set_opaque (widget, prop);
}

void *
simtk_console_get_opaque (const simtk_widget_t *widget)
{
  struct simtk_console_properties *prop;

  prop = simtk_console_get_properties (widget);

  return prop->opaque;
}

void
simtk_console_set_opaque (simtk_widget_t *widget, void *opaque)
{
  struct simtk_console_properties *prop;

  prop = simtk_console_get_properties (widget);

  prop->opaque = opaque;
}

int
simtk_console_create (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_console_render (widget);

  return HOOK_LOCK_CHAIN;
}

int
simtk_console_destroy (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_console_properties_destroy (simtk_console_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

int
simtk_console_hearbeat (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  struct simtk_console_properties *cprop;

  cprop = simtk_console_get_properties (widget);
  
  simtk_console_properties_lock (cprop);

  cprop->blinkstat = !cprop->blinkstat;
  
  simtk_console_properties_unlock (cprop);

  simtk_console_render (widget); /* Maybe we can improve this by just painting the required square */

  return HOOK_RESUME_CHAIN;
}

void
simtk_console_render (simtk_widget_t *widget)
{
  struct simtk_textview_properties *tprop;
  struct simtk_console_properties  *cprop;
  uint32_t cur_bgcolor, cur_fgcolor;
  int j;

  int start;
  int off;
  int len;
  
  cprop = simtk_console_get_properties (widget);
  tprop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (tprop);
  simtk_console_properties_lock (cprop);

  for (j = 0; j < tprop->rows; ++j)
  {
    simtk_textview_properties_unlock (tprop);
    simtk_textview_repeat (widget, 0, j, cprop->fgcolor, cprop->bgcolor, ' ', tprop->cols);
    simtk_textview_properties_lock (tprop);
    
    if (j + cprop->off_y >= 0 && j + cprop->off_y < cprop->rows)
    {
      if (cprop->off_x < 0)
      {
        start = -cprop->off_x;
        off = 0;
      }
      else
      {
        start = 0;
        off = cprop->off_x;
      }

      len = cprop->cols - (off + start);
      simtk_textview_properties_unlock (tprop);
      simtk_textview_set_text (widget, start, j, cprop->fgcolor, cprop->bgcolor, cprop->buffer + (j + cprop->off_y) * cprop->cols + off, len);
      simtk_textview_properties_lock (tprop);
    }
  }

  if (cprop->cur_x >= cprop->off_x &&
      cprop->cur_x < tprop->cols + cprop->off_x &&
      cprop->cur_y >= cprop->off_y &&
      cprop->cur_y < tprop->rows + cprop->off_y)
  {
    cur_fgcolor = cprop->blinkstat ? cprop->bgcolor : cprop->fgcolor;
    cur_bgcolor = cprop->blinkstat ? cprop->fgcolor : cprop->bgcolor;

    simtk_textview_properties_unlock (tprop);
    simtk_textview_set_text (widget,
                             cprop->cur_x + cprop->off_x,
                             cprop->cur_y + cprop->off_y,
                             cur_fgcolor,
                             cur_bgcolor,
                             cprop->buffer + cprop->cur_y * cprop->rows + cprop->cur_x,
                             1);
    simtk_textview_properties_lock (tprop);
  }
  
  simtk_console_properties_unlock (cprop);
  simtk_textview_properties_unlock (tprop);

  simtk_textview_render_text (widget);
}

static void
__simtk_console_scroll (simtk_widget_t *widget)
{
  struct simtk_console_properties  *cprop;
 
  cprop = simtk_console_get_properties (widget);

  memmove (cprop->buffer, cprop->buffer + cprop->cols, cprop->cols * (cprop->rows - 1));

  memset (cprop->buffer + cprop->cols * (cprop->rows - 1), 0, cprop->cols);
}

void
simtk_console_puts (simtk_widget_t *widget, const char *string)
{
  struct simtk_console_properties  *cprop;
  struct simtk_textview_properties *tprop;
  int i;
  int len;
  
  cprop = simtk_console_get_properties (widget);
  tprop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (tprop);
  simtk_console_properties_lock (cprop);

  len = strlen (string);
  
  for (i = 0; i < len; ++i)
  {
    switch (string[i])
    {
    case '\b':
      if (cprop->cur_x > 0)
        --cprop->cur_x;
      break;
      
    case '\r':
      cprop->cur_x = 0;
      break;

    case '\n':
      cprop->cur_x = 0;
      ++cprop->cur_y;
      break;
      
    default:
      cprop->buffer[cprop->cur_x++ + cprop->cur_y * cprop->cols] = string[i];
      break;
    }

    if (cprop->cur_x >= cprop->cols)
    {
      cprop->cur_x = 0;
      ++cprop->cur_y;
    }
    
    if (cprop->cur_y >= cprop->rows - 1)
    {
      __simtk_console_scroll (widget);
      --cprop->cur_y;
    }
  }

  if (cprop->cur_y >= tprop->rows)
    cprop->off_y = cprop->cur_y - tprop->rows + 1;
  
  simtk_console_properties_unlock (cprop);
  simtk_textview_properties_unlock (tprop);
}

void
simtk_console_vprintf (simtk_widget_t *widget, const char *fmt, va_list ap)
{
  va_list copy;
  char *string;
  
  va_copy (copy, ap);

  if ((string = vstrbuild (fmt, ap)) != NULL)
  {
    simtk_console_puts (widget, string);
    
    free (string);
  }

  va_end (ap);
}

void
scputs (simtk_widget_t *widget, const char *string)
{
  simtk_console_puts (widget, string);

  simtk_console_render (widget);
}

void
vscprintf (simtk_widget_t *widget, const char *fmt, va_list ap)
{
  simtk_console_vprintf (widget, fmt, ap);

  simtk_console_render (widget);
}

void
scprintf (simtk_widget_t *widget, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  
  vscprintf (widget, fmt, ap);

  simtk_console_render (widget);

  va_end (ap);
}

void
simtk_console_printf (simtk_widget_t *widget, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  
  simtk_console_vprintf (widget, fmt, ap);

  va_end (ap);
}

simtk_widget_t *
simtk_console_new (struct simtk_container *cont, int x, int y, int rows, int cols, int bufrows, int bufcols)
{
  simtk_widget_t *new;
  struct simtk_console_properties *prop;

  if ((new = simtk_textview_new (cont, x, y, rows, cols)) == NULL)
    return NULL;

  if ((prop = simtk_console_properties_new (bufrows, bufcols)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "Console");

  simtk_textview_set_opaque (new, prop);

  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_console_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_console_create);

  simtk_event_connect (new, SIMTK_EVENT_HEARTBEAT, simtk_console_hearbeat);
  
  return new;
}
