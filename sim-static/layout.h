#ifndef _BMP_LAYOUT_H
#define _BMP_LAYOUT_H

#include <stdint.h>

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
 
#ifndef __GNUC__
 #define __attribute__
#endif

#define _PACK __attribute__ ((packed))

#define BM_MAGIC 19778

typedef struct BITMAPFILEHEADER
{
  WORD  bfType _PACK; 
  DWORD bfSize _PACK;
  WORD  bfReserved1 _PACK;
  WORD  bfReserved2 _PACK;
  DWORD bfOffBits _PACK;
}
BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER
{
  DWORD biSize _PACK;
  DWORD biWidth _PACK;
  DWORD biHeight _PACK;
  WORD  biPlanes _PACK;
  WORD  biBitCount _PACK;
  DWORD biCompression _PACK;
  DWORD biSizeImage _PACK;
  DWORD biXPelsPerMeter _PACK;
  DWORD biYPelsPerMeter _PACK;
  DWORD biClrUsed _PACK;
  DWORD biClrImportant _PACK;
}
BITMAPINFOHEADER;

typedef struct RGBQUAD
{
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
}
RGBQUAD;

#define _ALIGN(x, size) ((((x) + (size) - 1) / (size)) * (size))
#define BITMAP_ROW_WIDTH(nominal) _ALIGN ((nominal), 8)
#define BITMAP_ROW_BYTES(nominal) _ALIGN(BITMAP_ROW_WIDTH (nominal) >> 3, 4)
#define BYTES_PER_PIX(bpp) (_ALIGN ((bpp), 8) >> 3)
#define PIX_OFFSET(x, y, w, h, bpp) (   \
  (((x) * (bpp)) >> 3) +                \
    ((h) - (y) - 1) *                   \
    BITMAP_ROW_BYTES (((w) * (bpp)))    \
  )

#define PIX_BIT_OFFSET(x, bpp) (((x) * (bpp)) & 7)

#define BITMAP_SIZE(w, h, bpp) ((h) * BITMAP_ROW_BYTES (((w) * (bpp)))) 
#define RGB(r, g, b) (((b) & 0xff) | ((g) & 0xff) << 8 | ((r) & 0xff) << 16)

static inline int get_color (int color, int bpp, int bit_off)
{
  int i;
  int result;
  
  for (result = 0, i = 0; i < bpp; i++)
    result |= (!!(color & (1 << (i + bit_off)))) << i;
    
  return result;
}

#endif /* _BMP_LAYOUT_H */

