#ifndef _SIMTK_TEXTVIEW_H
#define _SIMTK_TEXTVIEW_H

struct simtk_textview_properties
{
  SDL_mutex *lock;

  /* cpi_disp_font_from_display (display_from_simtk_widget (widget)) */
  struct cpi_disp_font *font;
  
  int render_x, render_y;
  int rows, cols;

  char *text;
  uint32_t *fore;
  uint32_t *back;

  void *opaque;
};

void simtk_textview_set_opaque (struct simtk_widget *, void *);
void *simtk_textview_get_opaque (const struct simtk_widget *);
void simtk_textview_set_text (struct simtk_widget *, int, int, uint32_t, uint32_t, const void *, size_t);
struct simtk_widget *simtk_textview_new (struct simtk_container *, int, int, int, int);
struct simtk_textview_properties *simtk_textview_get_properties (const struct simtk_widget *);

#endif /* _SIMTK_TEXTVIEW_H */
