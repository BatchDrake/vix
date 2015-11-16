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

#include <SDL/SDL_thread.h>

#include "widget.h"

#include "primitives.h"

extern struct simtk_container *root;

SDL_cond   *root_redraw_condition;
SDL_mutex  *root_redraw_mutex;
SDL_Thread *redraw_thread;

volatile int         root_redraw_requested;
volatile int         quit_flag;
volatile int         should_refresh;

int
simtk_should_refresh (void)
{
  int result;

  SDL_mutexP (root_redraw_mutex);
  
  result = should_refresh;

  should_refresh = 0;

  SDL_mutexV (root_redraw_mutex);
  
  return result;
}

int
simtk_container_clear_all (struct simtk_container *cont)
{
  int cleared = 0;
  
  if (cont->background_dirty)
  {
    if (cont->background != NULL)
      draw_to_display (cont->disp, cont->background, 0, 0, 0xff);
    else
      fbox (cont->disp, cont->x, cont->y, cont->x + cont->width - 1, cont->y + cont->height - 1, OPAQUE (0));

    cont->background_dirty = 0;

    cleared = 1;
  }

  return cleared;
}

void
simtk_widget_draw_border (const simtk_widget_t *widget, uint32_t color)
{
  struct simtk_container *cont = widget->parent;
  int x1, y1, x2, y2;

  x1 = cont->screen_offset_x + cont->x + widget->x - 1;
  x2 = cont->screen_offset_x + cont->x + widget->x + widget->width;

  y1 = cont->screen_offset_y + cont->y + widget->y - 1;
  y2 = cont->screen_offset_y + cont->y + widget->y + widget->height;

  if (x1 < cont->screen_start_x)
    x1 = cont->screen_start_x;
  if (x1 >= cont->screen_end_x)
    x1 = cont->screen_end_x - 1;

  if (x2 < cont->screen_start_x)
    x2 = cont->screen_start_x;
  if (x2 >= cont->screen_end_x)
    x2 = cont->screen_end_x - 1;

  if (y1 < cont->screen_start_y)
    y1 = cont->screen_start_y;
  if (y1 >= cont->screen_end_y)
    y1 = cont->screen_end_y - 1;

  if (y2 < cont->screen_start_y)
    y2 = cont->screen_start_y;
  if (y2 >= cont->screen_end_y)
    y2 = cont->screen_end_y - 1;
  
  line (cont->disp, x1, y1, x1, y2, color);

  line (cont->disp, x1, y1, x2, y1, color);

  line (cont->disp, x2, y2, x1, y2, color);

  line (cont->disp, x2, y2 + 1, x2, y1, color);
}

void
simtk_widget_dump_to_screen (simtk_widget_t *widget)
{
  int i, j;
  int xstart, xend, scr_x_off, scr_y_off;
  int idx;
  display_t *disp;
  
  simtk_widget_lock (widget);

  disp = display_from_simtk_widget (widget);
  
  xstart = widget->x < 0 ? -widget->x : 0;
  xend   = (widget->x + widget->width >= widget->parent->width ?
	    widget->parent->width - widget->x : widget->width);

  scr_x_off = simtk_widget_to_screen_x (widget);
  scr_y_off = simtk_widget_to_screen_y (widget);
  
  for (j = widget->y < 0 ? -widget->y : 0; j < widget->height; ++j)
  {
    if (j >= widget->parent->height)
      break;

    idx = j * widget->width + xstart;

#ifdef SIMTK_BLENDING
    memcpy (&((Uint32 *) disp->screen->pixels) [x + y * disp->width], widget->buffers[!widget->current_buff][idx++], (xemd - xstart) * sizeof (Uint32));
#else
    for (i = xstart; i < xend; ++i)
      pset_abs (disp,
		scr_x_off + i, scr_y_off + j,
		widget->buffers[!widget->current_buff][idx++]);
#endif
  }

  simtk_widget_unlock (widget);
}


void
simtk_redraw_from (simtk_widget_t *root, int force)
{
  simtk_widget_t *this = root;
  struct simtk_event event = {0, 0, 0};

  int dirty_found = force;
  
  while (this != NULL)
  {
    event.button = dirty_found;
	      
    if (simtk_widget_is_dirty (this) || dirty_found)
    {
      simtk_widget_lock (this);

      if (this->switched || dirty_found)
      {
        simtk_widget_draw_border (this, this == this->parent->current_widget ? this->focused_border_color : this->blurred_border_color);
        simtk_widget_dump_to_screen (this);

	this->switched = 0;
      }
      
      trigger_hook (this->event_hooks, SIMTK_EVENT_REDRAW, &event);

      this->dirty = 0;

      dirty_found = 1;
      
      simtk_widget_unlock (this);
    }
    
    this = this->next;
  }
}

void
simtk_redraw_container (struct simtk_container *cont, int force)
{
  int i, j;

  int min_x = cont->width, min_y = cont->height, max_x = 0, max_y = 0;

  /* Add this information to container */
  for (i = 0; i < cont->widget_count; ++i)
    if (cont->widget_list[i] != NULL)
    {
      if (min_x > cont->widget_list[i]->x)
	min_x = cont->widget_list[i]->x;

      if (min_y > cont->widget_list[i]->y)
	min_y = cont->widget_list[i]->y;

      if (max_x < cont->widget_list[i]->x + cont->widget_list[i]->width)
	max_x = cont->widget_list[i]->x + cont->widget_list[i]->width;
      
      if (max_y < cont->widget_list[i]->y + cont->widget_list[i]->height)
	max_y = cont->widget_list[i]->y + cont->widget_list[i]->height;    
    }

  /* TODO: define container background */
  if (min_x < max_x && min_y < max_y)
  {
    simtk_redraw_from (cont->widget_list[0], force);

    /* XXX: this MUST be protected with mutexes!! */
    cont->dirty = 0;
  }
}

int
simtk_redraw_thread_SDL (void *arg)
{
  int force;
  
  while (!quit_flag)
  {
    SDL_mutexP (root_redraw_mutex);

    while (!root_redraw_requested && !quit_flag)
      SDL_CondWaitTimeout (root_redraw_condition, root_redraw_mutex, 10000);
    
    SDL_mutexV (root_redraw_mutex);

    if (quit_flag)
      break;
    
    simtk_container_lock (root);
    
    force = simtk_container_clear_all (root);

    simtk_redraw_container (root, force);

    simtk_container_unlock (root);

    SDL_mutexP (root_redraw_mutex);
    
    root_redraw_requested = 0;

    should_refresh = 1;

    SDL_mutexV (root_redraw_mutex);
  }

  printf ("Redraw thread: quit\n");

  return 0;
}

int
simtk_is_drawing (void)
{
  int val;
  
  SDL_mutexP (root_redraw_mutex);

  val = !should_refresh && root_redraw_requested;
  
  SDL_mutexV (root_redraw_mutex);

  return val;
}

void
simtk_redraw_thread_quit (void)
{
  int status;
  
  SDL_mutexP (root_redraw_mutex);
  
  quit_flag = 1;

  SDL_mutexV (root_redraw_mutex);

  SDL_CondSignal (root_redraw_condition);

  SDL_WaitThread (redraw_thread, &status);
}

void
simtk_set_redraw_pending (void)
{
  simtk_sort_widgets (root);
  
  SDL_mutexP (root_redraw_mutex);
  
  root_redraw_requested = 1;

  SDL_mutexV (root_redraw_mutex);

  SDL_CondSignal (root_redraw_condition);
}

int
simtk_init_threads_SDL (void)
{
  if ((root_redraw_mutex = SDL_CreateMutex ()) == NULL)
  {
    ERROR ("cannot create redraw SDL mutex\n");
    return -1;
  }

  if ((root_redraw_condition = SDL_CreateCond ()) == NULL)
  {
    ERROR ("cannot create redraw condition variable\n");

    SDL_DestroyMutex (root_redraw_mutex);
    
    return -1;
  }

  simtk_redraw_container (root, 1);

#ifdef SDL2_ENABLED
  if ((redraw_thread = SDL_CreateThread (simtk_redraw_thread_SDL, "redraw-thread", NULL)) == NULL)
#else
  if ((redraw_thread = SDL_CreateThread (simtk_redraw_thread_SDL, NULL)) == NULL)
#endif
  {
    ERROR ("cannot create redraw thread\n");

    SDL_DestroyCond (root_redraw_condition);

    SDL_DestroyMutex (root_redraw_mutex);

    return -1;
  }

  return 0;
  
}
