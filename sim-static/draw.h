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
    
#ifndef _DRAW_H
#define _DRAW_H

#include <SDL.h>
#include <math.h>
#include <complex.h>

#include "wbmp.h"
#include "cpi.h"

#include "hook.h"

#define EVENT_TYPE_KEYBOARD 0
#define EVENT_TYPE_MOUSE    1


struct display_info
{
  int dirty;
  
  int width, height;
  
  int min_x, min_y;
  int max_x, max_y;
  
  int cpi_selected;
  
  int     grid_step;
  double zoom;
  double offset_x, offset_y; /* offsets x and y relative to screen coords */
  
  SDL_Surface *screen;
  struct cpi_disp_font *selected_font;
  struct hook_bucket *kbd_hooks;
  struct text_area *whole_screen;
  struct area_info *areas;
  
  cpi_handle_t cpi_handle;
};

struct event_info;


typedef void (*mouse_handler_t) (struct display_info *, struct event_info *, void *);
typedef int (*generic_handler_t) (int, struct display_info *, struct event_info *);

typedef generic_handler_t kbd_handler_t;

struct area_info
{
  int x, y;
  int width, height;
  void *data;
  mouse_handler_t handler;
  struct area_info *next;
};

struct event_info
{
  int type;
  int code;
  int state;
  int x, y;
  
  struct area_info *area;
};



struct text_area
{
  struct cpi_disp_font *selected_font;
  
  cpi_handle_t cpi_handle;
  
  int cursor_x, cursor_y;
  int cpi_width, cpi_height;
  
  int pos_x, pos_y;
  int color, bgcolor;
  
  int autorefresh;
  struct display_info *display;
};


typedef struct display_info display_t;
typedef struct text_area    textarea_t;
typedef struct event_info   event_t;

static inline int
pt2px_x (display_t *disp, double x)
{
  return (x + disp->offset_x) * disp->zoom + (disp->width >> 1);
}

static inline int
pt2px_y (display_t *disp, double y)
{
  return (disp->height >> 1) - (y + disp->offset_y) * disp->zoom;
}

static inline double
px2pt_x (display_t *disp, int x)
{
  return  ((double) x - (disp->width >> 1)) / disp->zoom - disp->offset_x;
}

static inline double
px2pt_y (display_t *disp, int y)
{
  return ((disp->height >> 1) - (double) y) / disp->zoom - disp->offset_y;
}

#include "pixel.h"

display_t *display_new (int, int);
void display_refresh (display_t *);
struct draw *display_to_draw (display_t *);
void draw_to_display (display_t *, struct draw *, int, int, int);
int  display_dump (const char *, display_t *);
int  display_put_bmp (display_t *, const char *, int, int, int);
int  display_select_cpi (display_t *, const char *);
int  display_select_font (display_t *, int, int);

void display_puts (display_t *, int, int, int, int, const char *);
void display_printf (display_t *, int, int, int, int, const char *, ...);
void display_poll_events (display_t *); 
void display_wait_events (display_t *);
void display_break_wait (display_t *);
int  display_area_register (display_t *, int, int, int, int, mouse_handler_t, void *);
void display_end (display_t *);

textarea_t *display_textarea_new 
  (display_t *, int, int, int, int, const char *, int, int);
void cputs (textarea_t *, const char *); /* Why C?? */
void cprintf (textarea_t *, const char *, ...);
int  textarea_gotoxy (textarea_t *, int, int);

#define disputs(disp, str) cputs (disp->whole_screen, str)
#define disprintf(disp, fmt, arg...) cprintf (disp->whole_screen, fmt, ##arg)
#define disgotoxy(disp, x, y) textarea_gotoxy (disp->whole_screen, x, y)

void textarea_set_fore_color  (textarea_t *, int);
void textarea_set_back_color  (textarea_t *, int);
void textarea_set_autorefresh (textarea_t *, int);

void axis_set_zoom_level (display_t *, double);
void axis_set_offset (display_t *, double, double);
void axis_draw_part (display_t *, int, int, int, int, Uint32);
void axis_draw (display_t *, Uint32);

#endif /* _DRAW_H */

