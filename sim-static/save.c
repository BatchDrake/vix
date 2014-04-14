#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "layout.h"
#include <util.h>
#include "wbmp.h"

int 
draw_to_bmp (const char *file, struct draw *draw)
{
  FILE *fp;
  BITMAPFILEHEADER header;
  BITMAPINFOHEADER info;
  
  if ((fp = fopen (file, "wb")) == NULL)
  {
    ERROR ("Couldn't open %s for writing: %s\n", file, strerror (errno));
    
    return -1;
  }
  
  header.bfType = BM_MAGIC;
  header.bfSize = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) +
                  BITMAP_SIZE (draw->width, draw->height, 24);
                  
  header.bfReserved1 = header.bfReserved2 = 0;
  header.bfOffBits   = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER);
  
  if (fwrite (&header, sizeof (BITMAPFILEHEADER), 1, fp) < 1)
  {
    ERROR ("Couldn't store header in %s: %s\n", file, strerror (errno));
    fclose (fp);
    
    return -1;
  }
  
  info.biSize          = sizeof (BITMAPINFOHEADER);
  info.biWidth         = draw->width;
  info.biHeight        = draw->height;
  info.biPlanes        = 1; /* It's ONE fucking plane, no zero, you dumbass */
  info.biBitCount      = 24;
  info.biCompression   = 0;
  info.biSizeImage     = 0;
  info.biXPelsPerMeter = 0;
  info.biYPelsPerMeter = 0;
  info.biClrUsed       = 0;
  info.biClrImportant  = 0;
  
  if (fwrite (&info, sizeof (BITMAPINFOHEADER), 1, fp) < 1)
  {
    ERROR ("Couldn't store header in %s: %s\n", file, strerror (errno));
    fclose (fp);
    
    return -1;
  }
  
  if (fwrite (draw->pixels, draw->total_size, 1, fp) < 1)
  {
    ERROR ("Couldn't store data in %s: %s\n", file, strerror (errno));
    fclose (fp);
    
    return -1;
  }
  
  fclose (fp);
  
  return 0;  
}

