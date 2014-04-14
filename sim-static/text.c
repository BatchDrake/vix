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

int
have_font_selected (display_t *display)
{
  return display->cpi_selected && display->selected_font;
}

void 
display_puts (display_t *display, 
             int x, int y, int color, int bgcolor, const char *text)
{
  if (!have_font_selected (display))
  {
    ERROR ("display_puts: no font selected\n");
    return;
  }

  if (x < 0 || y < 0 || x >= display->width || y >= display->height)
    return;
    
  cpi_puts (display->selected_font, display->width, display->height, x, y, 
      display->screen->pixels, 4, !(bgcolor & 0xff000000), color, bgcolor, text);
  __make_dirty (display, x, y);
  __make_dirty (display, 
    x + 8 * strlen (text) - 1, y + display->selected_font->rows - 1);
}

void 
display_printf (display_t *display,
               int x, int y, int color, int bgcolor, const char *fmt, ...)
{
  va_list ap;
  char *text;
  
  if (!have_font_selected (display))
  {
    ERROR ("display_printf: no font selected\n");
    return;
  }
  
  
  va_start (ap, fmt);
  
  text = vstrbuild (fmt, ap);
  display_puts (display, x, y, color, bgcolor, text);
  
  free (text);
  
  va_end (ap);
}

int
display_select_cpi (display_t *display, const char *path)
{
  /* What if there's another CPI file selected */
  if (cpi_map_codepage (&display->cpi_handle, path) == -1)
    return -1;
  display->cpi_selected = 1;
  
  return 0;
}

int
display_select_font (display_t *display, int cp, int height)
{
  struct cpi_entry *page;
  
  if (!display->cpi_selected)
  {
    ERROR ("display_select_font: no CPI file selected\n");
    return -1;
  }
  
  if ((page = cpi_get_page (&display->cpi_handle, cp)) == NULL)
  {
    ERROR ("display_select_font: codepage `%d' doesn't exit\n", cp);
    return -1;
  }
  
  if ((display->selected_font = 
       cpi_get_disp_font (&display->cpi_handle, page, height, 8)) == NULL)
  {
    ERROR ("display_select_font: font of %dx%d doesn't exit\n", height, 8);
    return -1;
  }
  
  return 0;
}


void 
scroll_up (textarea_t *area)
{
  int i, j;
  
  for (i = area->selected_font->rows; 
       i < area->cpi_height * area->selected_font->rows; i++)
    memcpy (
      area->display->screen->pixels +
        sizeof (Uint32) * (area->pos_x + 
          (area->pos_y + i - area->selected_font->rows) 
          * area->display->width),
          
      area->display->screen->pixels +
        sizeof (Uint32) * (area->pos_x +
          (area->pos_y + i) * area->display->width),
            
      (area->cpi_width * 8) * sizeof (Uint32));
  
  
  for (j = 0; j < area->selected_font->rows; j++)
    memset (
      area->display->screen->pixels +
        sizeof (Uint32) * (area->pos_x +
          (area->pos_y + j + (area->cpi_height - 1) * area->selected_font->rows) 
        * area->display->width),
      0,
      (area->cpi_width * 8) * sizeof (Uint32));
            
  __make_dirty (area->display, area->pos_x, area->pos_y);
  __make_dirty (area->display, 
    area->pos_x + area->cpi_width  * 8 - 1, 
    area->pos_y + area->cpi_height * area->selected_font->rows - 1);
  
}

static inline void 
cputchar (textarea_t *area, char c)
{
  char cbuf [2];
  
  cbuf[0] = c;
  cbuf[1] = '\0';
  
  if (c == '\n')
  {
    area->cursor_x = 0;
    
    if (area->cursor_y++ == (area->cpi_height - 1))
    {
      area->cursor_y--;
      scroll_up (area);
    }
  }
  else if (c == '\b')
  {
    if (area->cursor_x > 0)
      area->cursor_x--;
  }
  else
  {
      if (area->cursor_x == area->cpi_width)
      {
        area->cursor_x = 0;
        
        if (area->cursor_y++ == (area->cpi_height - 1))
        {
          area->cursor_y--;
          scroll_up (area);
        }
      }
      
    cpi_puts (area->selected_font, 
      area->display->width, 
      area->display->height, 
      area->pos_x + area->cursor_x * 8, 
      area->pos_y + area->selected_font->rows * (area->cursor_y), 
      area->display->screen->pixels, 4, !(area->bgcolor & 0xff000000), area->color, area->bgcolor, cbuf);
  
    
    __make_dirty (area->display, 
      area->pos_x + area->cursor_x * 8, 
      area->pos_y + area->selected_font->rows * (area->cursor_y));
    
    __make_dirty (area->display, 
      area->pos_x + area->cursor_x * 8 + 7, 
      area->pos_y + area->selected_font->rows * (area->cursor_y + 1) - 1);
      
      area->cursor_x++;
  }
}

void 
cputs (textarea_t *area, const char *text)
{
  int i;
  
  for (i = 0; i < strlen (text); i++)
    cputchar (area, text[i]);
    
  if (area->autorefresh)
    display_refresh (area->display);
}


void 
cprintf (textarea_t *area, const char *fmt, ...)
{
  va_list ap;
  char *text;
  
  va_start (ap, fmt);
  
  text = vstrbuild (fmt, ap);
  
  cputs (area, text);
  
  free (text);
  
  va_end (ap);
}

/* FIXME: deallocate this */
textarea_t *
display_textarea_new (display_t *disp, int x, int y, int cols, int rows,
  const char *path, int cp, int height)
{
  textarea_t *new;
  struct cpi_entry *page;
  
  new = xmalloc (sizeof (textarea_t));
  
  memset (new, 0, sizeof (textarea_t));
  
  new->display = disp;
  
  /* What if there's another CPI file selected */
  if (cpi_map_codepage (&new->cpi_handle, path) == -1)
  {
    ERROR ("display_textarea_new: cpi_map_codepage failed\n");
    free (new);
    return NULL;
  }
  
  if ((page = cpi_get_page (&new->cpi_handle, cp)) == NULL)
  {
    ERROR ("display_textarea_new: cpi_get_page failed (no codepage %d)\n", cp);
    free (new);
    cpi_unmap (&new->cpi_handle);
    return NULL;
  }
  
  if ((new->selected_font = 
       cpi_get_disp_font (&new->cpi_handle, page, height, 8)) == NULL)
  {
    ERROR ("display_textarea_new: cpi_get_disp_font failed (no font of %dx%d)\n", height, 8);
    free (new);
    cpi_unmap (&new->cpi_handle);
    return NULL;
  }
  
  new->pos_x = x;
  new->pos_y = y;
  
  new->cpi_width  = cols;
  new->cpi_height = rows;
  
  new->cursor_x = 0;
  new->cursor_y = 0;
  
  new->color = ARGB (0xff, 0xff, 0x7f, 0x00);
  new->bgcolor = 0x000000;
  
  return new;
}

void
textarea_set_fore_color (textarea_t *area, int color)
{
  area->color = color;
}

void
textarea_set_back_color (textarea_t *area, int color)
{
  area->bgcolor = color;
}

void
textarea_set_autorefresh (textarea_t *area, int value)
{
  area->autorefresh = value;
}

int
textarea_gotoxy (textarea_t *area, int x, int y)
{
  if (x < 0 || x >= area->cpi_width || y < 0 || y >= area->cpi_height)
    return -1;
    
  area->cursor_x = x;
  area->cursor_y = y;
  
  return 0;
}

