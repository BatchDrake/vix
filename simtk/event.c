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

#include "widget.h"

static int event_loop_running;

int
simtk_in_event_loop (void)
{
  return event_loop_running;
}

/* Find it within container */
simtk_widget_t *
simtk_find_widget (struct simtk_container *cont, int x, int y)
{
  int i;

  for (i = cont->widget_count - 1; i >= 0; --i)
    if (cont->widget_list[i] != NULL)
      if (cont->widget_list[i]->x <= x && x < cont->widget_list[i]->x + cont->widget_list[i]->width &&
	  cont->widget_list[i]->y <= y && y < cont->widget_list[i]->y + cont->widget_list[i]->height)
	return cont->widget_list[i];

  return NULL;
}

void
simtk_container_event_cascade (struct simtk_container *cont, enum simtk_event_type type, struct simtk_event *event) /* Container-relative event */
{
  simtk_widget_t *widget = NULL;
  simtk_widget_t *focused_widget = NULL;
  simtk_widget_t *blurred_widget = NULL;
  
  struct simtk_event simtk_event;

  int prevent_exec_focus = 0;
  int ign_event = 0;
  
  blurred_widget = cont->current_widget;
  simtk_event = *event;
  
  switch (type)
  {    
  case SIMTK_EVENT_MOUSEUP:
  case SIMTK_EVENT_MOUSEDOWN:
  case SIMTK_EVENT_MOUSEMOVE:
    
    if ((widget = simtk_find_widget (cont, event->x, event->y)) != NULL)
    {
      if (type == SIMTK_EVENT_MOUSEDOWN)
      {
        simtk_widget_bring_front (widget);

        simtk_widget_focus (widget);
        
	simtk_set_redraw_pending ();
      }
      
      simtk_event.x = event->x - widget->x;
      simtk_event.y = event->y - widget->y;
    }

    break;

  case SIMTK_EVENT_KEYDOWN:
  case SIMTK_EVENT_KEYUP:
    widget = cont->current_widget;
    
    if (simtk_event.button == '\t' && type == SIMTK_EVENT_KEYDOWN)
    {      
      simtk_widget_focus (widget->next == NULL ? cont->widget_list[0] : widget->next);

      simtk_widget_bring_front (widget->next == NULL ? cont->widget_list[0] : widget->next);

      simtk_set_redraw_pending ();

      ++ign_event;
      
      break;
    }
    
    if (type == SIMTK_EVENT_KEYDOWN && widget->next != NULL)
    {
      simtk_widget_bring_front (widget);

      simtk_widget_focus (widget);
      
      simtk_set_redraw_pending ();
    }

    break;

  default:
    ++prevent_exec_focus;
    widget = cont->current_widget;
  }

  focused_widget = cont->current_widget;
        
  if (widget != NULL && !ign_event)
    trigger_hook (widget->event_hooks, type, &simtk_event);

  if (blurred_widget != focused_widget && !prevent_exec_focus)
  { 
    if (blurred_widget != NULL)
      trigger_hook (blurred_widget->event_hooks, SIMTK_EVENT_BLUR, &simtk_event);

    if (focused_widget != NULL)
      trigger_hook (focused_widget->event_hooks, SIMTK_EVENT_FOCUS, &simtk_event);
  }
}

/* TODO: fix this */
void
simtk_parse_event_SDL (struct simtk_container *cont, SDL_Event *sdl_event)
{
  struct simtk_event simtk_event;
  enum simtk_event_type type;

  simtk_widget_t *focused_widget = NULL;
  simtk_widget_t *blurred_widget = NULL;
  
  simtk_widget_t *widget = NULL;
  
  int x, y;
  int prevent_exec_focus = 0;
  int ign_event = 0;
  
  blurred_widget = cont->current_widget;
  
  /* Corrent x and y to container */

  switch (sdl_event->type)
  {
  case SDL_QUIT:
    simtk_container_destroy (cont);

    /* This is the proper way to do it: redraw thread holds
       some mutexes here that must be released AFTER all
       widgets are destroyed */
    
    simtk_redraw_thread_quit ();
    
    exit (0);
    
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
  case SDL_MOUSEMOTION:
    
    type = sdl_event->type == SDL_MOUSEBUTTONDOWN ? SIMTK_EVENT_MOUSEDOWN : (sdl_event->type == SDL_MOUSEMOTION ? SIMTK_EVENT_MOUSEMOVE : SIMTK_EVENT_MOUSEUP);
    
    simtk_event.button = sdl_event->button.button;

    x = sdl_event->button.x - cont->x;
    y = sdl_event->button.y - cont->y;

    if ((widget = cont->motion_widget) == NULL)
      widget = simtk_find_widget (cont, x, y);
    
    if (widget != NULL)
    {
      if (type == SIMTK_EVENT_MOUSEDOWN)
      {
	cont->motion_widget  = widget;

	simtk_widget_bring_front (widget);

        simtk_widget_focus (widget);
        
	simtk_set_redraw_pending ();
      }
      else if (type == SIMTK_EVENT_MOUSEUP)
	cont->motion_widget = NULL;
      
      simtk_event.x = x - widget->x;
      simtk_event.y = y - widget->y;
    }

    break;

  case SDL_KEYDOWN:
  case SDL_KEYUP:
    widget = cont->current_widget;
    
    type = sdl_event->type == SDL_KEYDOWN ? SIMTK_EVENT_KEYDOWN : SIMTK_EVENT_KEYUP;
    simtk_event.button = sdl_event->key.keysym.sym;

    /* This keyboard shortcut must change */
    if (simtk_event.button == '\t' && sdl_event->key.keysym.mod & KMOD_CTRL && type == SIMTK_EVENT_KEYDOWN)
    { 
      simtk_widget_focus (widget->next == NULL ? cont->widget_list[0] : widget->next);

      simtk_widget_bring_front (widget->next == NULL ? cont->widget_list[0] : widget->next);
      
      simtk_set_redraw_pending ();

      ++ign_event;
      
      break;
    }
    
    simtk_event.mod = 0;

    if (sdl_event->key.keysym.mod & KMOD_CTRL)
      simtk_event.mod |= SIMTK_KBD_MOD_CTRL;

    if (sdl_event->key.keysym.mod & KMOD_ALT)
      simtk_event.mod |= SIMTK_KBD_MOD_ALT;

    if (sdl_event->key.keysym.mod & KMOD_SHIFT)
      simtk_event.mod |= SIMTK_KBD_MOD_SHIFT;

    if (sdl_event->key.keysym.mod & KMOD_CAPS)
      simtk_event.mod |= SIMTK_KBD_MOD_CAPSLOCK;

   
    
#ifdef SDL2_ENABLED
    simtk_event.character = sdl_event->key.keysym.sym;
#else
    simtk_event.character = sdl_event->key.keysym.unicode;
#endif
    
    if (type == SIMTK_EVENT_KEYDOWN && widget->next != NULL)
    {
      simtk_widget_bring_front (widget);

      simtk_widget_focus (widget);
      
      simtk_set_redraw_pending ();
    }
    
    break;

  case SDL_USEREVENT:
    type = SIMTK_EVENT_HEARTBEAT;
    widget = cont->current_widget;
    
    break;
    
  default:
    ++prevent_exec_focus;
    return;
  }

  focused_widget = cont->current_widget;
    
  if (widget != NULL && !ign_event)
    trigger_hook (widget->event_hooks, type, &simtk_event);

  if (blurred_widget != focused_widget && !prevent_exec_focus)
  { 
    if (blurred_widget != NULL)
      trigger_hook (blurred_widget->event_hooks, SIMTK_EVENT_BLUR, &simtk_event);

    if (focused_widget != NULL)
      trigger_hook (focused_widget->event_hooks, SIMTK_EVENT_FOCUS, &simtk_event);
  }
}

void
simtk_widget_trigger_create (simtk_widget_t *widget)
{
  struct simtk_event event = {0, 0, 0};
  
  trigger_hook (widget->event_hooks, SIMTK_EVENT_CREATE, &event);
}

void
simtk_container_trigger_create_all (struct simtk_container *cont)
{
  int i;

  simtk_container_lock (cont);
  
  for (i = 0; i < cont->widget_count; ++i)
    if (cont->widget_list[i] != NULL)
      simtk_widget_trigger_create (cont->widget_list[i]);

  simtk_container_unlock (cont);
}

Uint32
simtk_heartbeat_handler (Uint32 interval, void *param)
{
  SDL_Event event;
  SDL_UserEvent userevent;
  
  userevent.type = SDL_USEREVENT;
  userevent.code = 0;
  userevent.data1 = NULL;
  userevent.data2 = NULL;
  
  event.type = SDL_USEREVENT;
  event.user = userevent;
  
  SDL_PushEvent (&event);
  
  return SIMTK_HEARTBEAT_DELAY_MS - (SDL_GetTicks () % SIMTK_HEARTBEAT_DELAY_MS);
}

void
simtk_event_loop (struct simtk_container *cont)
{
  display_t *disp = cont->disp;
  SDL_Event event;
  int dirty;

  simtk_container_trigger_create_all (cont);
  
  if (simtk_init_threads_SDL () == -1)
    exit (EXIT_FAILURE);

  /* Instead of this, add wait for main thread */
  simtk_container_lock (cont);

#ifdef SDL2_ENABLED
  SDL_UpdateWindowSurface (disp->window);
#else
  SDL_UpdateRect (disp->screen, 0, 0, disp->width, disp->height);
#endif
  
  simtk_container_unlock (cont);

  SDL_AddTimer (SIMTK_HEARTBEAT_DELAY_MS, simtk_heartbeat_handler, NULL);

#ifndef SDL2_ENABLED
  SDL_EnableUNICODE (SDL_ENABLE);
#endif

  event_loop_running = 1;
  
  for (;;)
  { 
    if (simtk_should_refresh ())
    {
      simtk_container_lock (cont);

#ifdef SDL2_ENABLED
      SDL_UpdateWindowSurface (disp->window);
#else
      SDL_UpdateRect (disp->screen, 
		      disp->min_x, disp->min_y,
		      disp->max_x - disp->min_x + 1,
		      disp->max_y - disp->min_y + 1);	      
      __make_clean (disp);
#endif      
      simtk_container_unlock (cont);
    }
    
    SDL_WaitEvent (&event);

    simtk_parse_event_SDL (cont, &event);
  }

  event_loop_running = 0;
}
