#include <draw.h>

#include "event.h"
#include "widget.h"

/* Find it within container */
struct simtk_widget *
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
  struct simtk_widget *widget = NULL;
  struct simtk_widget *focused_widget = NULL;
  struct simtk_widget *blurred_widget = NULL;
  
  struct simtk_event simtk_event;

  int prevent_exec_focus = 0;
  int ign_event = 0;
  
  blurred_widget = cont->current_widget;
  
  switch (type)
  {    
  case SIMTK_EVENT_MOUSEUP:
  case SIMTK_EVENT_MOUSEDOWN:
  case SIMTK_EVENT_MOUSEMOVE:
    
    if ((widget = simtk_find_widget (cont, event->x, event->y)) != NULL)
    {
      if (type == SIMTK_EVENT_MOUSEDOWN)
      {
	cont->current_widget = widget;
        widget->z = HUGE_Z;
	simtk_set_redraw_pending ();
      }
      
      simtk_event.x = event->x - widget->x;
      simtk_event.y = event->y - widget->y;
    }

    break;

  case SIMTK_EVENT_KEYDOWN:
  case SIMTK_EVENT_KEYUP:
    widget = cont->current_widget;
    
    if ((simtk_event.button = event->button) == '\t' &&
	(type == SIMTK_EVENT_KEYDOWN))
    { 
      cont->current_widget = widget->next == NULL ? cont->widget_list[0] : widget->next;
      
      simtk_set_redraw_pending ();

      ++ign_event;
      
      break;
    }

    if (type == SIMTK_EVENT_KEYDOWN && widget->next != NULL)
    {
      widget->z = HUGE_Z;

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

  struct simtk_widget *focused_widget = NULL;
  struct simtk_widget *blurred_widget = NULL;
  
  struct simtk_widget *widget = NULL;
  
  int x, y;
  int prevent_exec_focus = 0;
  int ign_event = 0;
  
  blurred_widget = cont->current_widget;
  
  /* Corrent x and y to container */

  switch (sdl_event->type)
  {
  case SDL_QUIT:
    simtk_redraw_thread_quit ();
    
    simtk_container_destroy (cont);

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
	cont->current_widget = widget;
	cont->motion_widget  = widget;
        widget->z = HUGE_Z;

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
    
    if ((simtk_event.button = sdl_event->key.keysym.sym) == '\t' &&
	(type == SIMTK_EVENT_KEYDOWN))
    {
      cont->current_widget = widget->next == NULL ? cont->widget_list[0] : widget->next;
      
      simtk_set_redraw_pending ();

      ++ign_event;
      
      break;
    }

    if (type == SIMTK_EVENT_KEYDOWN && widget->next != NULL)
    {
      widget->z = HUGE_Z;
      
      simtk_set_redraw_pending ();
    }
    
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
simtk_widget_trigger_create (struct simtk_widget *widget)
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

  SDL_UpdateRect (disp->screen, 0, 0, disp->width, disp->height);

  simtk_container_unlock (cont);

  for (;;)
  { 
    if (simtk_should_refresh ())
    {
      simtk_container_lock (cont);
      
      SDL_UpdateRect (disp->screen, 
		      disp->min_x, disp->min_y,
		      disp->max_x - disp->min_x + 1,
		      disp->max_y - disp->min_y + 1);
      
      cont->dirty = 0;

      simtk_container_unlock (cont);
    }
    
    SDL_WaitEvent (&event);

    simtk_parse_event_SDL (cont, &event);
  }
}
