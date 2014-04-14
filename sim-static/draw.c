/*
 *    <one line to give the program's name and a brief idea of what it does.>
 *    Copyright (C) <year>  <name of author>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wbmp.h"


#include "draw.h"
#include "cpi.h"

#include <util.h>
#include "hook.h"

#define DEFAULT_FONT_SIZE 8
#define DEFAULT_CODEPAGE  850

SDL_mutex *eventLock;

static void clean_exit (void) __attribute__ ((destructor));

static void
clean_exit (void)
{
  SDL_Quit ();
}

#if 0
static void
enter_opengl_context (void)
{
  SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
}

static SDL_Surface *
try_sdl_opengl (int width, int height)
{
  enter_opengl_context ();
  
  return SDL_SetVideoMode (width, height, 32, 
    SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_OPENGL);
}
#endif

static SDL_Surface *
try_sdl_traditional (int width, int height)
{
  return SDL_SetVideoMode (width, height, 32, SDL_DOUBLEBUF | SDL_HWSURFACE);
}

display_t *
display_new (int width, int height)
{
  display_t *new;
  
  new = xmalloc (sizeof (display_t));
  
  memset (new, 0, sizeof (display_t));
  
  new->width = width;
  new->height = height;
  
  if (SDL_Init (SDL_INIT_VIDEO) != 0)
  {
    ERROR ("Unable to initialize SDL: %s\n", SDL_GetError ());
    free (new);
    
    return NULL;
  }
 
  atexit (SDL_Quit);
  
  /* if ((new->screen = try_sdl_opengl (width, height)) == NULL)
  {
    fprintf (stderr, "display_new: can't use OpenGL render, trying traditional\n");
  */
  
    if ((new->screen = try_sdl_traditional (width, height)) == NULL)
    {
      ERROR ("Unable to set video mode: %s\n", SDL_GetError ());
      free (new);
      
      return NULL;
    }
    
  /*
  }
  */
  
  if ((new->whole_screen = display_textarea_new (
    new, 
    0, 
    0, 
    width / 8, 
    height / DEFAULT_FONT_SIZE, 
    NULL, 
    850, 
    DEFAULT_FONT_SIZE)
  ) == NULL)
  {
    ERROR ("Unable to set-up whole screen textarea: %s\n", strerror (errno));
    free (new);
    
    return NULL;
  }
  
  textarea_set_autorefresh (new->whole_screen, 1);
  
  SDL_WM_SetCaption ("libsim: simulation window",
                     "libsim: simulation window");
  
  memset (new->screen->pixels, 0, width * height * sizeof (Uint32));
  
  new->kbd_hooks = hook_bucket_new (512);
  new->grid_step = 16;
  new->zoom = 1.0;
  
  if (display_select_cpi (new, NULL) != -1)
    (void) display_select_font (new, DEFAULT_CODEPAGE, DEFAULT_FONT_SIZE);

  if (eventLock == NULL)
    eventLock = SDL_CreateMutex ();    
  return new;
}

static struct area_info*
area_info_new (void)
{
  struct area_info *new;
  
  new = xmalloc (sizeof (struct area_info));
  
  memset (new, 0, sizeof (struct area_info));
  
  return new;
}

static void
area_info_register (display_t *display, struct area_info *area)
{
  area->next = display->areas;
  display->areas = area;
}

int
display_area_register (display_t *disp, int x, int y, int width, int height, mouse_handler_t handler, void *data)
{
  struct area_info *new;
  
  if (x < 0 || y < 0 || x >= disp->width || y >= disp->height)
    return -1;
    
  new = area_info_new ();
  
  new->x = x;
  new->y = y;
  
  new->height = height;
  new->width  = width;
  new->handler = handler;
  new->data = data;
  
  area_info_register (disp, new);
  
  return 0;
}

static inline void
__try_mouse_event (display_t *disp, event_t *event, struct area_info *area)
{
  if (event->x >= area->x && 
      event->y >= area->y && 
      event->x < (area->x + area->width) && 
      event->y < (area->y + area->height))
  {
    event->area = area;
    area->handler (disp, event, area->data);
  }
}

static inline void
__notify_mouse_event (display_t *disp, SDL_Event *event)
{
  static event_t event_info;
  struct area_info *this;
  
  if (disp->areas)
  {
    event_info.type = EVENT_TYPE_MOUSE;
    event_info.code = event->button.button;
    event_info.state = event->type == SDL_MOUSEBUTTONDOWN;
    event_info.x    = event->button.x;
    event_info.y    = event->button.y;
    
    for (this = disp->areas; this != NULL; this = this->next)
      __try_mouse_event (disp, &event_info, this);
  }
  
}


int
display_register_key_handler (display_t *disp, int key, kbd_handler_t handler)
{
  return hook_register (disp->kbd_hooks, key, 
    (int (*) (int, void *, void *)) handler, disp);
}

static inline void
__parse_event (display_t *display, SDL_Event *event)
{
  static event_t event_info;
  
  switch (event->type)
  {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    
      event_info.type  = EVENT_TYPE_KEYBOARD;
      event_info.code  = event->key.keysym.sym;
      event_info.state = event->type == SDL_KEYDOWN;
      
      trigger_hook (display->kbd_hooks, event->key.keysym.sym, &event_info);
      
      if (event->type == SDL_KEYUP && event->key.keysym.sym == SDLK_ESCAPE)
        exit (0);
      break;
      
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      __notify_mouse_event (display, event);
      break;
    
    case SDL_QUIT:
      exit (0);
      break;
  }
}

void
display_poll_events (display_t *display)
{
  SDL_Event event;

  while (SDL_PollEvent (&event))
    __parse_event (display, &event); 
}

void
display_wait_events (display_t *display)
{
  SDL_Event event;
  
  SDL_WaitEvent (&event);
  __parse_event (display, &event);
  
  display_poll_events (display);
}

void
display_break_wait (display_t *display)
{
  SDL_Event event;
  
  event.type = SDL_USEREVENT;
  event.user.code = 0;
  event.user.data1 = 0;
  event.user.data2 = 0;
  
  SDL_LockMutex (eventLock);
  SDL_PushEvent (&event);
  SDL_UnlockMutex (eventLock);
}

void
display_refresh (display_t *display)
{
  if (display->dirty)
  {
    SDL_UpdateRect (display->screen, 
      display->min_x, display->min_y,
      display->max_x - display->min_x + 1,
      display->max_y - display->min_y + 1);
      
    display->dirty = 0;
  }
  
  /* Add some if */
  
  display_poll_events (display);
}

/* Do the reverse thing */
struct draw *
display_to_draw (display_t *display)
{
  int i, j;
  
  struct draw *output;
  
  output = draw_new (display->width, display->height);
  
  for (j = 0; j < display->height; j++)
    for (i = 0; i < display->width; i++)
      draw_pset (output, i, j, pget (display, i, j));
      
  return output;
}

void
draw_to_display (display_t *display, struct draw *draw, int x, int y, int a)
{
  int offset_x = 0, offset_y = 0;
  int actual_width, actual_height;
  int i, j;
  
  if (x < 0)
  {
    offset_x = -x;
    x = 0;
  }
  
  if (y < 0)
  {
    offset_y = -y;
    y = 0;
  }
  
  actual_width  = draw->width - offset_x;
  actual_height = draw->height - offset_y;
  
  if (actual_width > 0 && actual_height > 0)
  {
    if (actual_width > display->width)
      actual_width = display->width;
      
    if (actual_height > display->height)
      actual_height = display->height;
      
    for (j = 0; j < actual_height; j++)
      for (i = 0; i < actual_width; i++)
        pset_abs (display, i + x, j + y, 
          ARGB(a, 0, 0, 0) | 
          (COLOR_MASK & draw_pget (draw, i + offset_x, j + offset_y)));
  }
}

int
display_dump (const char *file, display_t *display)
{
  int err;
  
  struct draw *output;
  
  output = display_to_draw (display);
  
  err = draw_to_bmp (file, output);
  
  draw_free (output);
  
  return err;
}

int
display_put_bmp (display_t *display, 
                 const char *file, 
                 int x, 
                 int y, 
                 int alpha)
{
  struct draw *draw;
  
  
  if ((draw = draw_from_bmp (file)) == NULL)
    return -1;
  
  draw_to_display (display, draw, x, y, alpha);
  
  draw_free (draw);
  
  return 0;
}

void
display_end (display_t *display)
{
  display_refresh (display);
  
  for (;;) 
    display_wait_events (display);
}


