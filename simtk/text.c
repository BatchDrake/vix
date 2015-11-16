#include <draw.h>

#include "widget.h"
#include "primitives.h"

void
simtk_widget_render_string_cpi (
  simtk_widget_t  *widget,
  struct cpi_disp_font *font,
  int x, int y,
  uint32_t foreground,
  uint32_t background,
  const char *text)
{
  int i, j;
  int n;
  int len;
  int charlength;
  int p;
    
  struct glyph *glyph;
  
  if (font->cols != 8)
  {
    ERROR ("cpi_puts: weird font type (cols != 8)\n");
    return;
  }
  
  len = strlen (text);
  
  if (font->cols * len + x > widget->width)
    len = (widget->width - x) / font->cols + !!((widget->width - x) % font->cols);
  
  charlength = font->rows;
  
  if (charlength + y > widget->height)
    charlength = widget->height - y;
  
  for (n = 0; n < len; n++)
  {
    if ((glyph = cpi_get_glyph (font, (unsigned char) text[n])) == NULL)
      continue;
        
    for (j = 0; j < charlength; j++)
      for (i = 0; i < 8; i++)
	simtk_widget_pset (widget,
			   x + i + (n << 3),
			   y + j,
			   (glyph->bits[j] & (1 << (7 - i))) ?
			   foreground : background);
    
  }
}

void
simtk_widget_render_char_cpi (
  simtk_widget_t  *widget,
  struct cpi_disp_font *font,
  int x, int y,
  uint32_t foreground,
  uint32_t background,
  char c)
{
  int i, j;
  int charlength;
  
  struct glyph *glyph;
  
  if ((glyph = cpi_get_glyph (font, (unsigned char) c)) == NULL)
    return;

  charlength = font->rows;
  
  for (j = 0; j < charlength; j++)
    for (i = 0; i < 8; i++)
      simtk_widget_pset (widget, x + i, y + j,
			 (glyph->bits[j] & (1 << (7 - i))) ?
			 foreground : background);
}
