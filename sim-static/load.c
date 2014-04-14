#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "layout.h"
#include <util.h>
#include "wbmp.h"

struct draw *draw_from_bmp (const char *file)
{
  FILE *fp;
  DWORD *pix;
  struct draw *draw;
  
  void *fdata;
  size_t size;
  int color_size;
  int i, j;
  DWORD color;
  
  BITMAPFILEHEADER *header;
  BITMAPINFOHEADER *info;
  RGBQUAD          *colors;
  
  if ((fp = fopen (file, "rb")) == NULL)
  {
    ERROR ("Couldn't open file \"%s\": %s\n", file, strerror (errno));
    
    return NULL;
  }
  
  fseek (fp, 0, SEEK_END);
  size = ftell (fp);
  fseek (fp, 0, SEEK_SET);
  
  if (size < sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER))
  {
    ERROR ("%s: broken header\n", file);
    fclose (fp);
    
    return NULL;
  }
  
  if ((fdata = mmap (NULL, size, PROT_READ, MAP_SHARED, fileno (fp), 0)) ==
    NULL)
  {
    ERROR ("%s: mmap failed: %s\n", file, strerror (errno));
    fclose (fp);
    
    return NULL;
  }
  
  header = fdata;
  info   = fdata + sizeof (BITMAPFILEHEADER);
    
  if (header->bfType != BM_MAGIC)
  {
    ERROR ("%s: not a BMP file\n", file);
    fclose (fp);
    munmap (fdata, size);
    
    return NULL;
  }
  
  if (header->bfSize > size)
  {
    ERROR ("%s: broken file\n", file);
    fclose (fp);
    munmap (fdata, size);
    
    return NULL;
  }
  
  if (header->bfOffBits >= size)
  {
    ERROR ("%s: incongruent field bfOffBits" 
           "(it says %d, size is %d bytes long)\n", 
           file, header->bfOffBits, (int) size);
    fclose (fp);
    munmap (fdata, size);
    
    return NULL;
  }
  
  if (info->biCompression != 0)
  {
    ERROR ("%s: the file is compressed\n", file);
    fclose (fp);
    munmap (fdata, size);
    
    return NULL;
  }
  
  colors = fdata + sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER);
  
  color_size = info->biBitCount >= 24 ? 0 : 
    (info->biClrUsed == 0 ? 1 << info->biBitCount : info->biClrUsed);
    
  if ((sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + 
      color_size * sizeof (RGBQUAD)) > header->bfOffBits)
  {
    ERROR ("%s: color table overlaps picture data\n", file);
    ERROR ("0x%lx > 0x%x\n", (sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + 
      color_size * sizeof (RGBQUAD)), header->bfOffBits);
    ERROR ("%d colors used\n", info->biClrUsed);
    
    fclose (fp);
    munmap (fdata, size);
    
    return NULL;
  }
  
  draw = draw_new (info->biWidth, info->biHeight);
  
  for (j = 0; j < info->biHeight; j++)
    for (i = 0; i < info->biWidth; i++)
    {
      pix = (DWORD *) (fdata + header->bfOffBits + 
        PIX_OFFSET (i, j, info->biWidth, info->biHeight, info->biBitCount));
      
      /*FIXME: check if pix is out of bounds */
      
      color = 0xffffff & *pix;
      if (color_size)
      {
        color = get_color (color, info->biBitCount,
          PIX_BIT_OFFSET (i, info->biBitCount));
          
        if (color >= color_size)
        {
          ERROR ("%s: unmapped color %x (%d colors)\n", file, color,
            color_size);
          fclose (fp);
          munmap (fdata, size);
          draw_free (draw);
          
          return NULL;  
        }
        
        color = RGB (colors[color].rgbRed,
                     colors[color].rgbGreen,
                     colors[color].rgbBlue);
      }
      
      draw_pset (draw, i, j, color);
    }
    
  fclose (fp);
  munmap (fdata, size);
  
  return draw;
}

