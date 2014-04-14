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

#include <util.h>
#include "layout.h"
#include "wbmp.h"

struct draw *
draw_new (int width, int height)
{
  struct draw *draw;
  
  draw = xmalloc (_ALIGN (sizeof (struct draw), 4));
  
  draw->width  = width;
  draw->height = height;
  draw->total_size = BITMAP_SIZE (width, height, 24);
  draw->pixels = xmalloc (draw->total_size);
  
  printf ("Total size: %d\n", draw->total_size);
  
  memset (draw->pixels, 0, draw->total_size);
  
  return draw;
}

void 
draw_free (struct draw *draw)
{
  free (draw->pixels);
  free (draw);
}

void 
draw_pset (struct draw *draw, int x, int y, DWORD color)
{
  DWORD *p;
  
  if (x < 0 || x >= draw->width || y < 0 || y >= draw->height)
  {
    WARNING ("Coords (%d, %d) out of bounds!\n", x, y);
    return;
  }
  
  p = (DWORD *) (draw->pixels + PIX_OFFSET (x, y, draw->width, draw->height, 24));
  
  *p &= 0xff000000;  
  *p |= (0xffffff & color);
}

DWORD 
draw_pget (struct draw *draw, int x, int y)
{
  DWORD *p;
  
  if (x < 0 || x >= draw->width || y < 0 || y >= draw->height)
  {
    WARNING ("Coords (%d, %d) out of bounds!\n", x, y);
    return 0;
  }
  
  p = (DWORD *) (draw->pixels + PIX_OFFSET (x, y, draw->width, draw->height, 24));
  
  return *p & 0xffffff;  
}


