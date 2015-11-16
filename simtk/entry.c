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

#include "widget.h"
#include "event.h"
#include "primitives.h"
#include "textview.h"
#include "entry.h"

#ifndef SDL2_ENABLED
static char *clipboard;
static int noxclipfound;

/* These are the clipboard hacks SimTK uses if it was built against SDL 1.2.X.

   SDL doesn't have clipboard support, so we have to trust into external
   applications to achieve this. It's dirty and slow, but it's a workaround and
   it works. And it will be like this until we switch to SDL 2.0 completely.
*/

int
SDL_SetClipboardText (const char *text)
{
  FILE *fp;

  if (!noxclipfound)
  {
    if ((fp = popen ("xclip", "w")) != NULL)
    {
      fwrite (text, strlen (text), 1, fp);
      
      pclose (fp);

      return 0;
    }
    
    WARNING ("xclip was not found on your system. Copy&paste won't work outside Vix.\n");
      
    noxclipfound = 1;
  }

  if (clipboard != NULL)
    free (clipboard);

  clipboard = strdup (text);

  return clipboard != NULL;
}

char *
SDL_GetClipboardText (void)
{
  FILE *fp;
  char *buff, *tmp;
  int len;
  int totallen;
  int required;
  int buflen;
  
  buff = NULL;
  totallen = 0;
  buflen = 0;
  
  if (!noxclipfound)
  {
    if ((fp = popen ("xclip -o", "r")) != NULL)
    {
      while (!feof (fp) && !ferror (fp))
      {
        required = 4096 + 1;

        if (totallen + required > buflen)
        {
          if ((tmp = realloc (buff, buflen + 2 * required)) == NULL)
          {
            pclose (fp);
            
            if (buff != NULL)
              free (buff);
            
            return NULL;
          }

          buff = tmp;

          buflen += 2 * required;
        }
        
        if ((len = fread (buff + totallen, 1, required - 1, fp)) > 0)
          totallen += len;
      }

      if (buff != NULL) {
	      buff[totallen] = '\0';
      }
      
      pclose (fp);

      if (clipboard != NULL)
      {
        free (clipboard);
        
        clipboard = NULL;
      }
      
      clipboard = buff;
    }
    else
    {
      WARNING ("xclip was not found on your system. Copy&paste won't work outside Vix.\n");
      
      noxclipfound = 1;
    }
  }

  if (clipboard == NULL)
    return "";

  return clipboard;
}

#endif

struct simtk_entry_properties *
simtk_entry_properties_new (int buffer_length, const char *text)
{
  struct simtk_entry_properties *new;
  
  if ((new = calloc (1, sizeof (struct simtk_entry_properties))) == NULL)
    goto fail;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
    goto fail;
  
  if (text != NULL)
    new->text_length = strlen (text);

  if (buffer_length < 1)
  {
    new->autogrow = 1;
    new->buffer_length = new->text_length + 1;
  }
  else
  {
    new->buffer_length = buffer_length + 1;
    
    if (new->text_length > buffer_length)
      new->text_length = buffer_length;
  }

  if ((new->buffer = malloc (new->buffer_length)) == NULL)
    goto fail;

  memcpy (new->buffer, text, new->text_length);
  
  new->buffer[new->text_length] = '\0';

  new->bgcolor = SIMTK_ENTRY_DEFAULT_BGCOLOR;
  new->fgcolor = SIMTK_ENTRY_DEFAULT_FGCOLOR;

  new->sel_bgcolor = SIMTK_ENTRY_DEFAULT_SEL_BGCOLOR;
  new->sel_fgcolor = SIMTK_ENTRY_DEFAULT_SEL_FGCOLOR;

  new->cur_bgcolor = SIMTK_ENTRY_DEFAULT_CUR_BGCOLOR;
  new->cur_fgcolor = SIMTK_ENTRY_DEFAULT_CUR_FGCOLOR;
  
  new->bordercolor = SIMTK_ENTRY_DEFAULT_BORDERCOLOR;

  new->blinking = 1;

  new->mark = -1;
  
  return new;

fail:
  if (new != NULL)
    simtk_entry_properties_destroy (new);
  
  return NULL;
}

void
simtk_entry_properties_destroy (struct simtk_entry_properties *prop)
{
  if (prop->buffer != NULL)
    free (prop->buffer);

  if (prop->lock != NULL)
    SDL_DestroyMutex (prop->lock);

  free (prop);
}

void
simtk_entry_properties_lock (const struct simtk_entry_properties *prop)
{
  SDL_mutexP (prop->lock);
}

void
simtk_entry_properties_unlock (const struct simtk_entry_properties *prop)
{
  SDL_mutexV (prop->lock);
}

struct simtk_entry_properties *
simtk_entry_get_properties (const simtk_widget_t *entry)
{
  return (struct simtk_entry_properties *) simtk_textview_get_opaque (entry);
}

void *
simtk_entry_get_opaque (const simtk_widget_t *widget)
{
  struct simtk_entry_properties *prop = simtk_entry_get_properties (widget);

  return prop->opaque;
}

void
simtk_entry_set_opaque (simtk_widget_t *widget, void *opaque)
{
  struct simtk_entry_properties *prop = simtk_entry_get_properties (widget);

  prop->opaque = opaque;
}

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

int
simtk_entry_create (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_entry_render (widget);

  return HOOK_LOCK_CHAIN;
}

int
simtk_entry_destroy (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_entry_properties_destroy (simtk_entry_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

int
simtk_entry_hearbeat (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  struct simtk_entry_properties *eprop;

  eprop = simtk_entry_get_properties (widget);
  
  simtk_entry_properties_lock (eprop);

  eprop->blinkstat = !eprop->blinkstat || !eprop->blinking;
  
  simtk_entry_properties_unlock (eprop);

  simtk_entry_render (widget); /* Maybe we can improve this by just painting the required square */

  return HOOK_RESUME_CHAIN;
}

void
simtk_entry_disable_blinking (simtk_widget_t *widget)
{
  struct simtk_entry_properties *eprop;

  eprop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (eprop);

  eprop->blinking = 0;

  simtk_entry_properties_unlock (eprop);
}

void
simtk_entry_enable_blinking (simtk_widget_t *widget)
{
  struct simtk_entry_properties *eprop;

  eprop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (eprop);

  eprop->blinking = 1;

  simtk_entry_properties_unlock (eprop);
}


void
simtk_entry_copy_selected (simtk_widget_t *widget)
{
  struct simtk_entry_properties *eprop;
  char save;
  
  eprop = simtk_entry_get_properties (widget);

  if (eprop->sel_length)
  {
    simtk_entry_properties_lock (eprop);
    
    /* THOSE DIRTY TRICKS IN EARLY VERSIONS */
    save = eprop->buffer[eprop->sel_start + eprop->sel_length];
    eprop->buffer[eprop->sel_start + eprop->sel_length] = '\0';
    
    SDL_SetClipboardText (eprop->buffer + eprop->sel_start);
    
    /* THOSE TRICKS */
    eprop->buffer[eprop->sel_start + eprop->sel_length] = save;
    
    simtk_entry_properties_unlock (eprop);
  }
}

int
isarrowkey (int code)
{
  return code == SDLK_LEFT || code == SDLK_RIGHT || code == SDLK_HOME || code == SDLK_END;
}

/* Funny stuff starts here */
int
simtk_entry_keyup (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_entry_enable_blinking (widget);

  return HOOK_RESUME_CHAIN;
}

int
simtk_entry_keydown (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  int cursor;
  int shift;
  struct simtk_entry_properties *eprop;
  char *text;
  
  eprop = simtk_entry_get_properties (widget);

  cursor = simtk_entry_get_cursor (widget);
  shift = event->mod & SIMTK_KBD_MOD_SHIFT;
    
  simtk_entry_disable_blinking (widget);

  if (event->mod & SIMTK_KBD_MOD_CTRL)
  {
    switch (event->button)
    {
    case 'c':
      simtk_entry_copy_selected (widget);
      break;
        
    case 'v':
      if ((text = SDL_GetClipboardText ()) != NULL)
        simtk_entry_insert_text (widget, text);
      break;
      
    case 'x':
      simtk_entry_copy_selected (widget);
      simtk_entry_remove_selected (widget);
      break;
    }
  }
  else
  {
    if (isarrowkey (event->button))
    {
      if (shift)
      {
        if (eprop->mark == -1)
          eprop->mark = cursor;
      }
      else
      {
        eprop->mark = -1;
        simtk_entry_select (widget, 0, 0);
      }
    }
    
    if (isprint (event->character))
      simtk_entry_insert_char (widget, event->character);
    else switch (event->button)
    {
    case '\n':
    case '\r':
      trigger_hook (widget->event_hooks, SIMTK_EVENT_SUBMIT, event);
      break;
      
    case '\b':
      if (eprop->mark != -1)
      {
        simtk_entry_remove_selected (widget);
        eprop->mark = -1;
      }
      else if (cursor > 0)
      {
        simtk_entry_select (widget, cursor - 1, 1);
        simtk_entry_remove_selected (widget);
      }
      
      break;

    case '\x7f':
      if (eprop->mark != -1)
      {
        simtk_entry_remove_selected (widget);
        eprop->mark = -1;
      }
      else
      {
        simtk_entry_select (widget, cursor, 1);
        simtk_entry_remove_selected (widget);
      }
      
      break;
      
    case SDLK_RIGHT:
      simtk_entry_set_cursor (widget, ++cursor);
      
      if (shift)
      {
        if (eprop->mark < cursor)
          simtk_entry_select (widget, eprop->mark, cursor - eprop->mark);
        else
          simtk_entry_select (widget, cursor, eprop->mark - cursor);
      }
      
      break;

    case SDLK_LEFT:
      if (cursor > 0)
      {
        simtk_entry_set_cursor (widget, --cursor);

        if (shift)
        {
          if (eprop->mark < cursor)
            simtk_entry_select (widget, eprop->mark, cursor - eprop->mark);
          else
            simtk_entry_select (widget, cursor, eprop->mark - cursor);
        }
      }
      break;
      
    case SDLK_HOME:
      simtk_entry_set_cursor (widget, cursor = 0);
      
      if (shift)
      {
        if (eprop->mark < cursor)
          simtk_entry_select (widget, eprop->mark, cursor - eprop->mark);
        else
          simtk_entry_select (widget, cursor, eprop->mark - cursor);
      }
        
      break;

    case SDLK_END:
      simtk_entry_set_cursor (widget, cursor = eprop->text_length);
      
      if (shift)
      {
        if (eprop->mark < cursor)
          simtk_entry_select (widget, eprop->mark, cursor - eprop->mark);
        else
          simtk_entry_select (widget, cursor, eprop->mark - cursor);
      }
        
      break;
      
        
    }
  }
  
  simtk_entry_render (widget);

  return HOOK_LOCK_CHAIN;
}


void
simtk_entry_render (simtk_widget_t *widget)
{
  struct simtk_textview_properties *tprop;
  struct simtk_entry_properties *eprop;
  uint32_t curr_bgcolor, curr_fgcolor;
  int i;
  
  eprop = simtk_entry_get_properties (widget);
  tprop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (tprop);
  simtk_entry_properties_lock (eprop);

  for (i = eprop->text_offset; i < MIN (eprop->text_length + 1, eprop->text_offset + tprop->cols); ++i)
  {
    if (i >= eprop->sel_start && i < eprop->sel_start + eprop->sel_length)
    {
      curr_bgcolor = eprop->sel_bgcolor;
      curr_fgcolor = eprop->sel_fgcolor;
    }
    else
    {
      curr_bgcolor = eprop->bgcolor;
      curr_fgcolor = eprop->fgcolor;
    }

    if (i == eprop->cursor && eprop->blinkstat)
    {
      curr_bgcolor = eprop->blinkstat ? eprop->cur_bgcolor : eprop->cur_fgcolor;
      curr_fgcolor = eprop->blinkstat ? eprop->cur_fgcolor : eprop->cur_bgcolor;
    }

    simtk_textview_properties_unlock (tprop);
    simtk_textview_set_text (widget, i - eprop->text_offset, 0, curr_fgcolor, curr_bgcolor, eprop->buffer + i, 1);
    simtk_textview_properties_lock (tprop);
  }

  if (i - eprop->text_offset < tprop->cols)
  {
    simtk_textview_properties_unlock (tprop);
    simtk_textview_repeat (widget, i - eprop->text_offset, 0, eprop->fgcolor, eprop->bgcolor, ' ', tprop->cols - (i - eprop->text_offset));
    simtk_textview_properties_lock (tprop);
  }
  
  simtk_entry_properties_unlock (eprop);
  simtk_textview_properties_unlock (tprop);
  
  simtk_textview_render_text (widget);
}


void
simtk_entry_select (simtk_widget_t *widget, int start, int length)
{
  struct simtk_entry_properties *prop;

  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  if (start >= 0 && start < prop->text_length)
  {
    prop->sel_start = start;
    prop->sel_length = length >= (prop->text_length - start) ? prop->text_length - start : length;
  }
  else
    prop->sel_start = prop->sel_length = 0;
  
  simtk_entry_properties_unlock (prop);

  /* TODO: call redraw */
}

void
__simtk_entry_set_cursor (simtk_widget_t *widget, int cur)
{
  struct simtk_entry_properties *prop;
  struct simtk_textview_properties *tprop;
  
  prop = simtk_entry_get_properties (widget);

  tprop = simtk_textview_get_properties (widget);

  simtk_textview_properties_lock (tprop); /* Won't deadlock */
  
  if (cur < 0)
    cur = 0;
  else if (cur > prop->text_length)
    cur = prop->text_length;
  
  prop->cursor = cur;

  /* Adjust text offset so the text cursor fits always within entry limits */
  if (prop->cursor < prop->text_offset)
    prop->text_offset = prop->cursor;
  else if (prop->cursor > (prop->text_offset + tprop->cols - 1))
    prop->text_offset = prop->cursor - tprop->cols + 1;

  simtk_textview_properties_unlock (tprop);
}

void
simtk_entry_set_cursor (simtk_widget_t *widget, int cur)
{
  struct simtk_entry_properties *prop;

  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  __simtk_entry_set_cursor (widget, cur);
  
  simtk_entry_properties_unlock (prop);
}

int
simtk_entry_get_cursor (simtk_widget_t *widget)
{
  struct simtk_entry_properties *prop;
  int result;
  
  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  result = prop->cursor;
  
  simtk_entry_properties_unlock (prop);

  return result;
}

char *
simtk_entry_get_text (simtk_widget_t *widget)
{
  struct simtk_entry_properties *prop;
  char *result;
  
  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  result = prop->buffer;
  
  simtk_entry_properties_unlock (prop);

  return result;
}

void
simtk_entry_clear (simtk_widget_t *widget)
{
  struct simtk_entry_properties *prop;

  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  prop->text_length = 0;
  prop->buffer[0] = 0;
  prop->sel_start = 0;
  prop->sel_length = 0;
  prop->cursor = 0;
  
  simtk_entry_properties_unlock (prop);

  return;
}

void
simtk_entry_remove_selected (simtk_widget_t *widget)
{
  struct simtk_entry_properties *prop;

  prop = simtk_entry_get_properties (widget);
  
  simtk_entry_properties_lock (prop);

  if (prop->sel_start >= 0 && prop->sel_start < prop->text_length && prop->sel_length > 0)
  {
    memmove (prop->buffer + prop->sel_start,
             prop->buffer + prop->sel_start + prop->sel_length,
             prop->text_length - (prop->sel_start + prop->sel_length) + 1); /* Move null char aswell */

    prop->text_length -= prop->sel_length;

    /* TODO: buffer shrinking? */

    __simtk_entry_set_cursor (widget, prop->sel_start);

    prop->sel_start = prop->sel_length = 0;
  }
  
  simtk_entry_properties_unlock (prop);
}

void
simtk_entry_set_textfilter (simtk_widget_t *widget, char *(*filter) (const char *))
{
  struct simtk_entry_properties *prop;

  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  prop->textfilter = filter;
  
  simtk_entry_properties_unlock (prop);
}

int
simtk_entry_insert_text (simtk_widget_t *widget, const char *input)
{
  struct simtk_entry_properties *prop;
  int required_length;
  int buffer_length;
  int i, textlen;
  char *newbuf;
  const char *text;
  char *copy;
  
  simtk_entry_remove_selected (widget);

  prop = simtk_entry_get_properties (widget);

  simtk_entry_properties_lock (prop);

  if (prop->textfilter != NULL)
    text = copy = (prop->textfilter) (input);
  else
    text = input;
  
  if (text == NULL)
  {
    simtk_entry_properties_unlock (prop);
    return -1;
  }
  
  textlen = strlen (text);
  
  required_length = textlen + prop->text_length + 1;

  if ((buffer_length = prop->buffer_length) == 0)
    buffer_length = 1;

  while (buffer_length < required_length)
    buffer_length <<= 1;
  
  if (buffer_length != prop->buffer_length)
  {
    if (!prop->autogrow) /* Buffer cannot grow automatically, discarding */
    {
      simtk_entry_properties_unlock (prop);

      /* Or shall we cut the text? */
      
      return 0;
    }
    
    if ((newbuf = realloc (prop->buffer, buffer_length)) == NULL)
    {
      simtk_entry_properties_unlock (prop);

      return -1;
    }

    prop->buffer_length = buffer_length;
    prop->buffer = newbuf;
  }

  memmove (prop->buffer + prop->cursor + textlen, prop->buffer + prop->cursor, prop->text_length - prop->cursor + 1);
  
  memcpy (prop->buffer + prop->cursor, text, textlen);

  prop->text_length += textlen;

  __simtk_entry_set_cursor (widget, prop->cursor + textlen);

  prop->sel_start = prop->sel_length = 0;

  prop->mark = -1;

  if (prop->textfilter != NULL)
    free (copy);
  
  simtk_entry_properties_unlock (prop);

  return 0;
}

int
simtk_entry_insert_char (simtk_widget_t *widget, char c)
{
  char string[] = {c, '\0'};

  return simtk_entry_insert_text (widget, string);
}

simtk_widget_t *
simtk_entry_new (struct simtk_container *cont, int x, int y, int cols)
{
  simtk_widget_t *new;
  struct simtk_entry_properties *prop;

  if ((new = simtk_textview_new (cont, x, y, 1, cols)) == NULL)
    return NULL;

  if ((prop = simtk_entry_properties_new (0, NULL)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "Entry");

  simtk_textview_set_opaque (new, prop);

  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_entry_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_entry_create);

  simtk_event_connect (new, SIMTK_EVENT_HEARTBEAT, simtk_entry_hearbeat);

  simtk_event_connect (new, SIMTK_EVENT_KEYDOWN, simtk_entry_keydown);
  
  simtk_event_connect (new, SIMTK_EVENT_KEYUP, simtk_entry_keyup);

  return new;
}
