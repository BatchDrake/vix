#ifndef _WBMP_H
#define _WBMP_H

#include "layout.h"

struct draw
{
  int width;
  int height;
  int total_size;
  
  BYTE *pixels;
};

struct draw *draw_new (int, int);
void draw_free (struct draw *);
struct draw *draw_from_bmp (const char *);
int draw_to_bmp (const char *, struct draw *);

void  draw_pset (struct draw *, int, int, DWORD);
DWORD draw_pget (struct draw *, int, int);

#endif /* _WBMP_H */

