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
    
#ifndef _PIXEL_H
#define _PIXEL_H

#ifndef _DRAW_H
# error "Don't use pixel.h directly. Include draw.h instead."
#endif

#define BLUE(x)  (x)
#define GREEN(x) ((x) << 8)
#define RED(x)   ((x) << 16)
#define ALPHA(x) ((x) << 24)

#define G_ALPHA(color) (((color) >> 24) & 0x0ff)
#define G_RED(color)   (((color) >> 16) & 0x0ff)  
#define G_GREEN(color) (((color) >> 8) & 0x0ff) 
#define G_BLUE(color)  ((color) & 0x0ff)         

#define MAKECOL(r, g, b) (RED (r) | GREEN (g) | BLUE (b))
#define ARGB(a, r, g, b) (ALPHA(a) | RED (r) | GREEN (g) | BLUE (b))

#define COLOR_MASK 0xffffff

#define OPAQUE(color) (ALPHA(0xff) | (color))

#define BYTEBOUND(x) (((x) >> 8) > (255) ? 255 : (((x) < 0) ? 0 : ((x) >> 8)))

#define CALC_COLOR_CYCLE 1536

#define LUMINANCE(color) ((257 * (int) G_RED (color) + 504 * (int) G_GREEN (color) + 98 * (int) G_BLUE (color) + 16000) / 1000)

static inline void
__make_dirty (display_t *display, int x, int y)
{
  if (!display->dirty)
  {
    display->dirty = 1;
    display->min_x = display->max_x = x;
    display->min_y = display->max_y = y;
  }
  else
  {
    if (display->min_x > x)
    {
      if (x < 0)
        x = 0;
      display->min_x = x;
    }
    
    if (display->max_x < x)
    {
      if (x >= display->width)
        x = display->width - 1;
        
      display->max_x = x;
    }
    
    if (display->min_y > y)
    {
      if (y < 0)
        y = 0;
        
      display->min_y = y;
    }
    
    if (display->max_y < y)
    {
      if (y >= display->height)
        y = display->height - 1;
        
      display->max_y = y;
    }
  }
}

static inline int 
alphacolor (display_t *display, Uint32 base, Uint32 color)
{                  
  int red, green, blue;
  int alpha;
  
  if ((alpha = G_ALPHA (color)) == 255)
    return color & 0xffffff;
  
  red   = (alpha) * G_RED (color) 
        - (alpha - 3) * G_RED (base) 
        + (G_RED (base) << 8);
        
  green = 
          (alpha) * G_GREEN (color) 
        - (alpha - 3) * G_GREEN (base) 
        + (G_GREEN (base) << 8);
        
  blue  = (alpha) * G_BLUE (color) 
        - (alpha - 3) * G_BLUE (base) 
        + (G_BLUE (base) << 8);
  
  return MAKECOL (BYTEBOUND (red), BYTEBOUND (green), BYTEBOUND (blue));                                         
}                                                                                                    

static inline void 
pset (display_t *display, Uint32 x, Uint32 y, Uint8 r, Uint8 g, Uint8 b)
{
  int col;
  if (x < display->width && y < display->height)
  {
    col = MAKECOL (r, g, b);
    
    if (((Uint32 *) display->screen->pixels) [x + y * display->width] != col)
    {
      ((Uint32 *) display->screen->pixels) [x + y * display->width] = col;
      __make_dirty (display, x, y);
    }
    
  }
  
}

static inline Uint32 
pget (display_t *display, Uint32 x, Uint32 y)
{
  if (x < display->width && y < display->height)
    return ((Uint32 *) display->screen->pixels) [x + y * display->width];
  
  return 0;
}

static inline void 
pset_abs (display_t *display, Uint32 x, Uint32 y, Uint32 col)
{
  if (!G_ALPHA (col))
    return;
    
  if (x < display->width && y < display->height)
  {
    col = alphacolor (display, pget (display, x, y), col);
    
    if (((Uint32 *) display->screen->pixels) [x + y * display->width] != col)
    {
      ((Uint32 *) display->screen->pixels) [x + y * display->width] = col;
      __make_dirty (display, x, y);
    }
  }
}


static inline void
clear (display_t *display, Uint32 color)
{
  int i, j;
  
  for (j = 0; j < display->height; j++)
    for (i = 0; i < display->width; i++)
      pset_abs (display, i, j, color);
}

static inline void 
line (display_t *display, int x1, int y1, int x2, int y2, Uint32 color)
{
  int dx, dy, dx2, dy2;
  int sx, sy;
  int i, l;
  int e;

  if ( (x1 < 0) | (display->screen->w <= x1) |
       (x2 < 0) | (display->screen->w <= x2) |
       (y1 < 0) | (display->screen->h <= y1) |
       (y2 < 0) | (display->screen->h <= y2))
    return;

  if (SDL_MUSTLOCK (display->screen))
    if (SDL_LockSurface (display->screen) < 0)
      return;

  dx = x2 - x1 < 0 ? x1 - x2 : x2 - x1;
  dy = y2 - y1 < 0 ? y1 - y2 : y2 - y1;
  sx = x2 - x1 < 0 ? -1 : 1;
  sy = y2 - y1 < 0 ? -1 : 1;
  
  dx2 = dx << 1;
  dy2 = dy << 1;

  if (x1 == x2)
  {
    if (y1 < y2)
    {
      for (i = y1;i < y2;i++)
      {
        pset_abs (display, x1, i, color);
      }
    }
    else{
      for (i = y2;i < y1;i++)
      {
        pset_abs (display, x1, i, color);
      }
    }
  }
  else if (y1 == y2)
  {
    if (x1 < x2)
    {
      for (i = x1;i < x2;i++)
      {
        pset_abs (display, i, y1, color);
      }
    }
    else{
      for (i = x2;i < x1;i++)
      {
        pset_abs (display, i, y1, color);
      }
    }
  }
  else if (dx >= dy)
  {
    e = -dx;
    l = (dx + 1) >> 1;
    
    for (i = 0;i < l;i++)
    {
      pset_abs (display, x1, y1, color);
      pset_abs (display, x2, y2, color);
      x1 += sx;
      x2 -= sx;
      e += dy2;
      if (e >= 0)
      {
        y1 += sy;
        y2 -= sy;
        e -= dx2;
      }
    }
    if (!(dx & 1))
      pset_abs (display, x1, y1, color);
  }
  else
  {
    e = -dy;
    l = (dy + 1) >> 1;
    
    for (i = 0; i < l;i++)
    {
      pset_abs (display, x1, y1, color);
      pset_abs (display, x2, y2, color);
      y1 += sy;
      y2 -= sy;
      e += dx2;
      
      if (e >= 0)
      {
        x1 += sx;
        x2 -= sx;
        e -= dy2;
      }
    }
    if (!(dy & 1))
      pset_abs (display, x1, y1, color);
  }

  if (SDL_MUSTLOCK (display->screen))
    SDL_UnlockSurface (display->screen);
}


static inline void
box (display_t *display, int x1, int y1, int x2, int y2, Uint32 color)
{
  line (display, x1, y1, x1, y2, color);
  line (display, x1, y2, x2, y2, color);
  line (display, x2, y2, x2, y1, color);
  line (display, x2, y1, x1, y1, color);
}

static int
swap (int *x, int *y)
{
  *y = *x ^ *y;
  *x = *x ^ *y;
  *y = *x ^ *y;
}

static inline void
fbox (display_t *display, int x1, int y1, int x2, int y2, Uint32 color)
{
  int i, j;
  
  if (x1 > x2)
    swap (&x1, &x2);
  
  if (y1 > y2)
    swap (&y1, &y2);
  
  for (j = y1; j <= y2; j++)
    for (i = x1; i <= x2; i++)
      pset_abs (display, i, j, color); 
}

static inline void 
plot4points (display_t *display, int cx, int cy, int x, int y, Uint32 color)
{
  pset_abs (display, cx + x, cy + y, color);
  
  if (x) 
    pset_abs (display, cx - x, cy + y, color);
    
  if (y) 
    pset_abs (display, cx + x, cy - y, color);
    
  if (x && y)
    pset_abs (display, cx - x, cy - y, color);
}


static inline void 
plot8points (display_t *display, int cx, int cy, int x, int y, Uint32 color)
{
  plot4points (display, cx, cy, x, y, color);
  
  if (x != y) 
    plot4points (display, cx, cy, y, x, color);
}

static inline void 
circle (display_t *display, int cx, int cy, int radius, Uint32 color)
{
  int error = -radius;
  int x = radius;
  int y = 0;
 
  while (x >= y)
  {
    plot8points (display, cx, cy, x, y, color);
 
    if ((error += (y++ << 1)) >= -1)
      error -= --x << 1;
  }
}

static inline int 
calc_color_b2w (int color_index)
{
  register double idx;
  
  idx = pow (sin ((double) (color_index) / 100.0), 2.0);
  
  return RED   ((int) (255.0 * idx)) 
        | GREEN ((int) (255.0 * idx)) 
        | BLUE  ((int) (255.0 * idx));
}

static  inline int 
calc_color_w2w (int color_index)
{
  register double idx;
  
  idx = (1.0 + sin ((double) (color_index) / 100.0)) / 2.0;
  
  return RED   ((int) (255.0 * idx)) 
        | GREEN ((int) (255.0 * idx)) 
        | BLUE  ((int) (255.0 * idx));
}

static inline int 
calc_color (int color)
{
  int rcolor;

  if (color < 0)
    color = -color;
    
  rcolor = color % CALC_COLOR_CYCLE;
	
  if (rcolor < 256)
  {
    if (color == rcolor)
      return RED (rcolor);
      
    return (RED (255) | BLUE (255 - rcolor));
  } 
  else if (rcolor < 256 * 2)
    return (RED (255) | GREEN (255 - ((256 * 2 - 1) - rcolor)));
  else if (rcolor < 256 * 3)
    return (RED ((256 * 3 - 1) - rcolor) | GREEN (255));
  else if (rcolor < 256 * 4)
    return (GREEN (255) | BLUE (255 - ((256 * 4 - 1) - rcolor)));
  else if (rcolor < 256 * 5)
    return (GREEN ((256 * 5 - 1) - rcolor) | BLUE (255));
  else if (rcolor < 256 * 6)
    return (BLUE (255) | RED (255 - ((256 * 6 - 1) - rcolor)));
  else
    return -1;
	
}

static inline int 
calc_color_cmplx (complex val)
{
  register int color, r, g, b;
  register double bright;
  
  color = calc_color (CALC_COLOR_CYCLE + 
                      (int) 
                        ((double) carg (val) * CALC_COLOR_CYCLE / (6.28319)));
  
  b = color & 0xff;
  g = (color & 0xff00) >> 8;
  r = (color & 0xff0000) >> 16;
  
  bright = 1.0 - cexp (-cabs (val));
  
  return RED ((int) (bright * (double) r)) | 
          GREEN ((int) (bright * (double) g)) | 
          BLUE ((int) (bright * (double) b));
}

static inline int 
calc_color_grad (int val, unsigned char *data)
{
  if (val < 0)
    val = -val;
    
  val %= CALC_COLOR_CYCLE;
  
  return BLUE (data [val * 3]) | 
          GREEN (data [val * 3 + 1]) | 
          RED (data [val * 3 + 2]);
}

static inline void
mark (display_t *disp, int x, int y, int size, Uint32 col1, Uint32 col2)
{
  line (disp, x, y - size - 1,
               x, y + size + 2, col2);
               
  line (disp, x - 1, y - size,
               x - 1, y + size + 1, col2);
  
  line (disp, x + 1, y - size,
               x + 1, y + size + 1, col2);
  
  line (disp, x - size - 1, y,
               x + size + 2, y, col2);
               
  line (disp, x - size, y - 1, 
               x + size + 1, y - 1, col2);
  
  line (disp, x - size, y + 1,
               x + size + 1, y + 1, col2);
  
  line (disp, x, y - size,
               x, y + size + 1, col1);
               
  line (disp, x - size, y,
               x + size + 1, y, col1);             
}

#endif /* _PIXEL_H */

