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

#include <stdint.h>

struct SDL_mutex {
	int recursive;
	uint32_t owner;
	void *sem;
};

#include <draw.h>

#include "widget.h"
#include "event.h"
#include "textview.h"

#include "primitives.h"

struct simtk_textview_properties *
simtk_textview_properties_new (int rows, int cols)
{
  struct simtk_textview_properties *new;

  if ((new = calloc (1, sizeof (struct simtk_textview_properties))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);

    return NULL;
  }

  new->rows = rows;
  new->cols = cols;

  if ((new->text = calloc (cols * rows, sizeof (char))) == NULL)
  {
    SDL_DestroyMutex (new->lock);

    free (new);

    return NULL;
  }

  if ((new->fore = calloc (cols * rows, sizeof (uint32_t))) == NULL)
  {
    free (new->text);

    SDL_DestroyMutex (new->lock);

    free (new);

    return NULL;
  }

  if ((new->back = calloc (cols * rows, sizeof (uint32_t))) == NULL)
  {
    free (new->fore);

    free (new->text);

    SDL_DestroyMutex (new->lock);

    free (new);

    return NULL;
  }

  return new;
}

void
simtk_textview_properties_lock (const struct simtk_textview_properties *prop)
{
  SDL_mutexP (prop->lock);
}


void
simtk_textview_properties_unlock (const struct simtk_textview_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_textview_properties_destroy (struct simtk_textview_properties *prop)
{
  simtk_textview_properties_lock (prop);

  free (prop->back);

  free (prop->fore);

  free (prop->text);

  simtk_textview_properties_unlock (prop);

  SDL_DestroyMutex (prop->lock);

  free (prop);
}

struct simtk_textview_properties *
simtk_textview_get_properties (const simtk_widget_t *widget)
{
  return (struct simtk_textview_properties *) simtk_widget_get_opaque (widget);
}


void *
simtk_textview_get_opaque (const simtk_widget_t *widget)
{
  struct simtk_textview_properties *prop;

  prop = simtk_textview_get_properties (widget);

  return prop->opaque;
}

void
simtk_textview_set_opaque (simtk_widget_t *widget, void *opaque)
{
  struct simtk_textview_properties *prop;

  prop = simtk_textview_get_properties (widget);
  
  simtk_textview_properties_lock (prop);

  prop->opaque = opaque;
  
  simtk_textview_properties_unlock (prop);
}


void
simtk_textview_render_text_noflip (simtk_widget_t *widget)
{
  struct simtk_textview_properties *prop;

  int i, j;
  
  int charwidth, charheight;
  int index = 0;
  
  prop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (prop);
  
  charwidth  = prop->font->cols;
  charheight = prop->font->rows;

  for (j = 0; j < prop->rows; ++j)
    for (i = 0; i < prop->cols; ++i)
    {
      simtk_widget_render_char_cpi (widget, prop->font, prop->render_x + i * charwidth, prop->render_y + j * charheight, prop->fore[index], prop->back[index], prop->text[index]);

      ++index;
    }
  
  simtk_textview_properties_unlock (prop);
}

void
simtk_textview_render_text (simtk_widget_t *widget)
{
  simtk_textview_render_text_noflip (widget);
  
  simtk_widget_switch_buffers (widget);
}

int
simtk_textview_create (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_textview_render_text (widget);

  return HOOK_RESUME_CHAIN;
}

/* Be careful: destroy hooks must be called in reverse order! */
int
simtk_textview_destroy (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_textview_properties_destroy (simtk_textview_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

void
simtk_textview_repeat (simtk_widget_t *widget, int x, int y, uint32_t fg, uint32_t bg, char c, size_t size)
{
  int idx, i;

  struct simtk_textview_properties *prop;

  prop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (prop);

  idx = x + y * prop->cols;

  if (idx < 0 || idx >= prop->cols * prop->rows)
  {
    simtk_textview_properties_unlock (prop);

    return;
  }

  if (idx + size > prop->cols * prop->rows)
    size = prop->cols * prop->rows - idx;

  while (size--)
  {
    prop->text[idx + size] = c;
    prop->fore[idx + size] = fg;
    prop->back[idx + size] = bg;
  }
  
  simtk_textview_properties_unlock (prop);
}


void
simtk_textview_set_text (simtk_widget_t *widget, int x, int y, uint32_t fg, uint32_t bg, const void *buf, size_t size)
{
  int idx;

  struct simtk_textview_properties *prop;

  prop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (prop);

  idx = x + y * prop->cols;

  if (idx < 0 || idx >= prop->cols * prop->rows)
  {
    simtk_textview_properties_unlock (prop);

    return;
  }

  if (idx + size > prop->cols * prop->rows)
    size = prop->cols * prop->rows - idx;
  
  memcpy (prop->text + idx, buf, size);

  while (size--)
  {
    prop->fore[idx + size] = fg;
    prop->back[idx + size] = bg;
  }
  
  simtk_textview_properties_unlock (prop);
}

/* Everything in here should be performed with a big lock on the whole widget */
simtk_widget_t *
simtk_textview_new (struct simtk_container *cont, int x, int y, int rows, int cols)
{
  simtk_widget_t *new;
  struct cpi_disp_font *font;
  struct simtk_textview_properties *prop;
  
  font = cpi_disp_font_from_display (simtk_container_get_display (cont));
  
  if ((new = simtk_widget_new (cont, x, y, cols * font->cols, rows * font->rows)) == NULL)
    return NULL;

  if ((prop = simtk_textview_properties_new (rows, cols)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "TextView");
  
  prop->font = font;
  
  simtk_widget_set_opaque (new, prop);
  
  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_textview_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_textview_create);

  return new;
}

