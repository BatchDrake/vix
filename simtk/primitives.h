#ifndef _SIMTK_PRIMITIVES_H
#define _SIMTK_PRIMITIVES_H

static inline display_t *
display_from_simtk_widget (simtk_widget_t *widget)
{
  return widget->parent->disp;
}

static inline struct cpi_disp_font *
cpi_disp_font_from_display (display_t *disp)
{
  return disp->selected_font;
}

static inline int
__simtk_widget_xy_to_buffidx (const simtk_widget_t *widget, int x, int y)
{
  return x + widget->width * y;
}

static inline void
__buffer_pset (uint32_t *buffer, int idx, uint32_t col)
{ 
  buffer[idx] = col;
}

static inline void
simtk_widget_pset (simtk_widget_t *widget, int x, int y, uint32_t col)
{
  uint32_t *thisbuffer = widget->buffers[widget->current_buff];
  int idx = __simtk_widget_xy_to_buffidx (widget, x, y);
  
  if (x < 0 || y < 0 || x >= widget->width || y >= widget->height)
    return;

  __buffer_pset (thisbuffer, idx, col);
}

static inline void
simtk_widget_fbox (simtk_widget_t *widget, int x1, int y1, int x2, int y2, uint32_t col)
{
  int i, j;
  int idx;
  uint32_t *this_buffer = widget->buffers[widget->current_buff];
  
  if (x1 < 0)
    x1 = 0;

  if (x2 < 0)
    x2 = 0;

  if (y1 < 0)
    y1 = 0;

  if (y2 < 0)
    y2 = 0;

  if (x1 >= widget->width)
    x1 = widget->width - 1;

  if (x2 >= widget->width)
    x2 = widget->width - 1;

  if (y1 >= widget->height)
    y1 = widget->height - 1;

  if (y2 >= widget->height)
    y2 = widget->height - 1;

  if (x1 > x2)
    swap (&x1, &x2);

  if (y1 > y2)
    swap (&y1, &y2);

  for (j = y1; j <= y2; ++j)
  {
    idx = j * widget->width + x1;
    
    for (i = x1; i <= x2; ++i)
      __buffer_pset (this_buffer, idx++, col);
  }   
}

void simtk_widget_render_string_cpi (simtk_widget_t *,
				     struct cpi_disp_font *,
				     int, int,
				     uint32_t, uint32_t,
				     const char *text);

void simtk_widget_render_char_cpi (simtk_widget_t  *,
				   struct cpi_disp_font *,
				   int, int,
				   uint32_t,
				   uint32_t,
				   char);

#endif /* _SIMTK_PRIMITIVES_H */
