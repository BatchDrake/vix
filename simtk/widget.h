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

#ifndef _SIMTK_WIDGET_H
#define _SIMTK_WIDGET_H

#include <draw.h>
#include <util.h>

#include "../config.h"
#include "event.h"

#define HUGE_Z (1 << 30)

#define DEFAULT_BLURD_BORDER OPAQUE (RGB (0x7f, 0x52, 0))
#define DEFAULT_FOCUS_BORDER OPAQUE (RGB (0xff, 0xa5, 0))

#define DEFAULT_BACKGROUND   ARGB (0x7f, 0, 0, 0)
#define DEFAULT_FOREGROUND   ARGB (0xff, 0xff, 0xa5, 0)

struct simtk_container
{
  SDL_mutex *lock;

  int screen_offset_x, screen_offset_y;

  int screen_start_x, screen_start_y;
  int screen_end_x, screen_end_y;
  
  int x, y;
  int width, height;

  int dirty;
  int background_dirty;
  
  PTR_LIST (struct simtk_widget, widget);

  struct simtk_widget *current_widget;
  struct simtk_widget *motion_widget;
  struct simtk_widget *container_widget;
  
  struct draw         *background;
  display_t           *disp;

  int                  current_zorder;
};

struct simtk_widget
{
  SDL_mutex *lock;
  
  struct simtk_container *parent;
  void *opaque;
  char *inheritance;
  
  int x;
  int y;

  int z;

  int dirty;

  /* When we deal with containers, there's no way to know
     whether they're dirty or not. We have to query explicitly
     to the widget to know if it needs to be redrawn */
  int (*child_is_dirty) (struct simtk_widget *);
  
  int width;
  int height;

  uint32_t background;
  uint32_t foreground;
  
  uint32_t blurred_border_color;
  uint32_t focused_border_color;
  
  struct hook_bucket *event_hooks;

  int        current_buff;
  uint32_t  *buffers[2];
  int        switched;
  
  PTR_LIST (struct simtk_container, container);

  struct simtk_widget *next;
};


static inline int
simtk_widget_to_screen_x (struct simtk_widget *widget)
{
  return widget->parent->x + widget->parent->screen_offset_x + widget->x;
}


static inline int
simtk_widget_to_screen_y (struct simtk_widget *widget)
{
  return widget->parent->y + widget->parent->screen_offset_y + widget->y;
}

static inline void
simtk_container_update_offset (struct simtk_container *container, const struct simtk_container *parent, int x, int y)
{
  container->x = x;
  container->y = y;
  
  container->screen_offset_x = parent->x + parent->screen_offset_x;
  container->screen_offset_y = parent->y + parent->screen_offset_y;

  if ((container->screen_start_x = container->screen_offset_x + container->x) < parent->screen_start_x)
    container->screen_start_x = parent->screen_start_x;
  if ((container->screen_start_y = container->screen_offset_y + container->y) < parent->screen_start_y)
    container->screen_start_y = parent->screen_start_y;
  
  if ((container->screen_end_x = container->screen_offset_x + container->width + container->x) >= parent->screen_end_x)
    container->screen_end_x = parent->screen_end_x;
  if ((container->screen_end_y = container->screen_offset_y + container->height + container->y) >= parent->screen_end_y)
    container->screen_end_y = parent->screen_end_y;
}


struct simtk_container *simtk_container_new (int, int, int, int);
void simtk_container_destroy (struct simtk_container *);
void __simtk_sort_widgets (struct simtk_container *);
void simtk_container_set_display (struct simtk_container *, display_t *);
display_t *simtk_container_get_display (struct simtk_container *);

int simtk_init_from_display (display_t *);
struct simtk_container *simtk_get_root_container (void);
void simtk_sort_widgets (struct simtk_container *);
void simtk_widget_draw_border (const struct simtk_widget *, uint32_t);
void simtk_redraw_from (struct simtk_widget *, int);
int  simtk_container_clear_all (struct simtk_container *);
void simtk_redraw_container (struct simtk_container *, int);
struct simtk_widget *simtk_widget_new (struct simtk_container *, int, int, int, int);
void simtk_event_connect (struct simtk_widget *, enum simtk_event_type, void *);
void simtk_widget_set_opaque (struct simtk_widget *, void *);
void *simtk_widget_get_opaque (const struct simtk_widget *);
void simtk_widget_set_redraw_query_function (struct simtk_widget *, int (*) (struct simtk_widget *));
int  simtk_widget_is_dirty (struct simtk_widget *);
void simtk_widget_get_absolute (struct simtk_widget *, int *, int *);
void simtk_widget_move (struct simtk_widget *, int, int);
int  simtk_widget_is_focused (struct simtk_widget *);
void simtk_widget_set_background (struct simtk_widget *, uint32_t);
void simtk_widget_set_foreground (struct simtk_widget *, uint32_t);
int  __simtk_widget_add_container (struct simtk_widget *, struct simtk_container *);
void simtk_widget_bring_front (struct simtk_widget *);
void simtk_widget_set_focus (struct simtk_widget *);

int simtk_widget_is_class (struct simtk_widget *, const char *);
int simtk_widget_inheritance_add (struct simtk_widget *, const char *);

void simtk_widget_destroy (struct simtk_widget *);
void __simtk_widget_destroy (struct simtk_widget *, int);

void simtk_set_redraw_pending (void);
void simtk_redraw_thread_quit (void);
int simtk_init_threads_SDL (void);

void simtk_container_lock (const struct simtk_container *);
void simtk_container_unlock (const struct simtk_container *);

void simtk_widget_lock (const struct simtk_widget *);
void simtk_widget_unlock (const struct simtk_widget *);
int simtk_should_refresh (void);
int simtk_is_drawing (void);
void simtk_widget_switch_buffers (struct simtk_widget *);
void simtk_container_set_background (struct simtk_container *, const char *);

void simtk_container_make_dirty (struct simtk_container *);
void simtk_widget_make_dirty (struct simtk_widget *);

#endif /* _SIMTK_WIDGET_H */
