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

void
axis_set_zoom_level (display_t *disp, double zoom)
{
  disp->zoom = zoom;
}

void
axis_set_offset (display_t *disp, double x, double y)
{
  disp->offset_x = x;
  disp->offset_y = y;
}

void
axis_draw_part (display_t *disp, int x, int y, int width, int height, Uint32 color)
{
  int line_x, line_y;
  int i;
  
  /* We have to draw two axis in the square ranging from (x, y) to
     (x + width - 1, y + height - 1). We can find the (0.0, 0.0) at
     (x + width / 2, y + height / 2). So, if we have an offset of
     (ox, oy), this means an offset of (ox * ZOOM, -oy * ZOOM) on the
     screen. So, the center of our axis will be at:
     
     (x + width / 2 + ox * ZOOM, y + height / 2 - oy * ZOOM)
     
     */
     
     
  line_x = x + width  / 2 + disp->offset_x * disp->zoom;
  line_y = y + height / 2 - disp->offset_y * disp->zoom;
  
  /* Draw the Y axis */
  if (line_x >= x && line_x < (x + width))
    line (disp, line_x, y, line_x, y + height - 1, color);
    
  /* Draw the X axis */
  if (line_y >= y && line_y < (y + height))
    line (disp, x, line_y, x + width - 1, line_y, color);
    
  for (i = (line_x - x) % disp->grid_step; i < width; i += disp->grid_step)
    line (disp, x + i, y, x + i, y + height - 1, (0xffffff & color) | ARGB (0x5f, 0, 0, 0));
    
  for (i = (line_y - y) % disp->grid_step; i < height; i += disp->grid_step)
    line (disp, x, y + i, x + width - 1, y + i, (0xffffff & color) | ARGB (0x5f, 0, 0, 0));  
    
  line (disp, x, y, x + width - 1, y, color);
  line (disp, x, y, x, y + height - 1, color);
  
  line (disp, x + width - 1, y + height - 1, x + width - 1, y, color);
  line (disp, x + width - 1, y + height - 1, x, y + height - 1, color);
  
  
}

void
axis_draw (display_t *disp, Uint32 color)
{
  axis_draw_part (disp, 0, 0, disp->width, disp->height, color);
}

